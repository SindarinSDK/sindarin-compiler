#!/usr/bin/env python3
"""Sindarin callers consume independently built native packages with generated adapters."""
import os
from pathlib import Path
import subprocess
import shutil
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
COMPILER=Path(os.environ.get('SN_COMPILER',ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))).resolve()

class NativeImports(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='sn-native-import-')
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name).resolve()

    def write(self,path,text):
        file=self.root/path
        file.parent.mkdir(parents=True,exist_ok=True)
        file.write_text(text)

    def package(self,name,language,source,result='value',failure='abort'):
        suffix={'C':'c','RS':'rs','GO':'go'}[language]
        self.write(f'.sn/{name}/src/api.sn', 'native fn provide(): '+('str' if result=='owned' else 'int')+'\n')
        self.write(f'.sn/{name}/src/impl.{suffix}',source)
        manifest=f'name: {name}\nruntime: {language}\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n    - name: backing\n      language: {language}\n      sources: [src/impl.{suffix}]\n'
        if language=='RS':manifest+='      entry: src/impl.rs\n'
        if language=='GO':
            manifest+='      module: src\n'
            self.write(f'.sn/{name}/src/go.mod','module sindarin.test/'+name+'\n\ngo 1.26.0\n')
        manifest+=f'  bindings:\n    - declaration: src/api.sn::provide\n      build: backing\n      symbol: native_{name}\n      convention: C\n      failure: {failure}\n      ownership: {{parameters: {{}}, result: {result}}}\n'
        self.write(f'.sn/{name}/sn.yaml',manifest)

    def execute(self,target,expected):
        executable=self.root/'main.exe'
        built=subprocess.run([str(COMPILER),'main.sn','--target',target,'--no-install','-o',str(executable)],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        run=subprocess.run([str(executable)],cwd=self.root,capture_output=True,timeout=15)
        self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))
        self.assertEqual(run.stdout,expected.replace(b'\n',b'\r\n') if os.name=='nt' else expected)

    def test_mixed_backing_and_portable_sindarin(self):
        self.package('cdep','C','#include <stdint.h>\nint64_t native_cdep(void) {return 11;}\n')
        self.package('rsdep','RS','#[no_mangle] pub extern "C" fn native_rsdep()->i64 {22}\n')
        self.package('godep','GO','package main\n/* #include <stdint.h> */\nimport "C"\n//export native_godep\nfunc native_godep() C.int64_t {return 33}\nfunc main() {}\n')
        self.write('.sn/portable/sn.yaml','name: portable\n')
        self.write('.sn/portable/src/api.sn','fn portable(value: int): int =>\n  return value + 1\n')
        self.write('main.sn','import "cdep/src/api" as CDep\nimport "rsdep/src/api" as RSDep\nimport "godep/src/api" as GoDep\nimport "portable/src/api"\nfn main(): void =>\n  println(portable(CDep.provide() + RSDep.provide() + GoDep.provide()))\n')
        for target in ('c','rust'):
            with self.subTest(target=target):self.execute(target,b'67\n')

    def test_c_links_dependency_graph_larger_than_path_limit(self):
        imports, calls = [], []
        for i in range(20):
            name=f'dep{i}'
            self.package(name,'C',f'#include <stdint.h>\nint64_t native_{name}(void) {{ return {i}; }}\n')
            imports.append(f'import "{name}/src/api" as Dep{i}')
            calls.append(f'Dep{i}.provide()')
        self.write('main.sn','\n'.join(imports)+'\nfn main(): void =>\n  println('+ ' + '.join(calls)+')\n')
        self.execute('c',b'190\n')

    def test_linker_options_preserve_framework_argument_pairs(self):
        import sys
        sys.path.insert(0,str(ROOT/'scripts'))
        from prepare_native_imports import native_link_options
        self.assertEqual(native_link_options(['-lc','-framework','CoreFoundation','-framework','Security','-L','a path','-l','pthread']),
                         ['-lc','-Wl,-framework,CoreFoundation','-Wl,-framework,Security','-La path','-lpthread'])
        with self.assertRaisesRegex(ValueError,'missing its argument'):
            native_link_options(['-framework'])

    def test_rust_mixed_app_retains_c_sdk_resources(self):
        sdk=ROOT/'.sn/sdk-native-integration'
        if not (sdk/'src/io/textfile.sn').is_file():
            self.fail('pinned SDK integration checkout is required')
        shutil.copytree(sdk/'src',self.root/'.sn/sindarin-pkg-sdk/src')
        shutil.copyfile(sdk/'sn.yaml',self.root/'.sn/sindarin-pkg-sdk/sn.yaml')
        self.package('cdep','C','#include <stdint.h>\nint64_t native_cdep(void) {return 11;}\n')
        self.package('rsdep','RS','#[no_mangle] pub extern "C" fn native_rsdep()->i64 {22}\n')
        self.package('godep','GO','package main\n/* #include <stdint.h> */\nimport "C"\n//export native_godep\nfunc native_godep() C.int64_t {return 33}\nfunc main() {}\n')
        self.write('.sn/http/sn.yaml','name: http\n')
        self.write('.sn/http/src/request.sn','fn requestLine(): str =>\n  return "GET / HTTP/1.1"\n')
        self.write('main.sn','import "cdep/src/api" as CDep\nimport "rsdep/src/api" as RSDep\nimport "godep/src/api" as GoDep\nimport "http/src/request"\nimport "sindarin-pkg-sdk/src/io/textfile"\nfn main(): void =>\n  TextFile.writeAll("mixed-sdk.txt", requestLine() + "\\n" + "second")\n  var file: TextFile = TextFile.open("mixed-sdk.txt")\n  var alias: TextFile = file\n  var lines: str[] = file.readLines()\n  alias.dispose()\n  println(lines[0])\n  println(lines[1])\n  println(file._is_open)\n  println(CDep.provide() + RSDep.provide() + GoDep.provide())\n  TextFile.delete("mixed-sdk.txt")\n')
        self.execute('rust',b'GET / HTTP/1.1\nsecond\n0\n66\n')

    def test_owned_shared_runtime_string(self):
        self.package('text','C','#include "sn_abi.h"\nuint32_t native_text(SnAbiValue **out) {return sn_abi_v1_string_copy("package text",out);}\n',result='owned',failure='status')
        self.write('main.sn','import "text/src/api"\nfn main(): void =>\n  println(provide())\n')
        for target in ('c','rust'):
            with self.subTest(target=target):self.execute(target,b'package text\n')

    def test_ownership_mismatch_is_rejected_before_linking(self):
        self.package('text','C','#include "sn_abi.h"\nuint32_t native_text(SnAbiValue **out) {return 0;}\n',result='owned',failure='status')
        path=self.root/'.sn/text/sn.yaml'
        path.write_text(path.read_text().replace('result: owned','result: value'))
        self.write('main.sn','import "text/src/api"\nfn main(): void =>\n  println(provide())\n')
        output=self.root/'main.exe'
        built=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o',str(output)],cwd=self.root,capture_output=True,timeout=60)
        self.assertNotEqual(built.returncode,0)
        self.assertIn(b'result ownership',built.stderr)
        self.assertFalse(output.exists())

    def test_borrowed_input_and_owned_result_lifetimes(self):
        self.package('text','C','#include "sn_abi.h"\nuint32_t native_text(SnAbiValue *input, SnAbiValue **out) { SnAbiBytes b; uint32_t s=sn_abi_v1_bytes(input,&b); if(s) return s; return sn_abi_v1_string_copy((const char *)b.data,out); }\n',result='owned',failure='status')
        self.write('.sn/text/src/api.sn','native fn provide(input: str): str\n')
        path=self.root/'.sn/text/sn.yaml'
        path.write_text(path.read_text().replace('parameters: {}','parameters: {input: borrowed}'))
        self.write('main.sn','import "text/src/api"\nfn main(): void =>\n  var original: str = "lifetime"\n  var result: str = provide(original)\n  original = "changed"\n  println(result)\n  println(provide("temporary"))\n')
        for target in ('c','rust'):
            with self.subTest(target=target):self.execute(target,b'lifetime\ntemporary\n')

    def test_native_status_error_has_same_terminal_contract(self):
        self.package('failed','C','#include <stdint.h>\nuint32_t native_failed(int64_t *out) { return 5; }\n',failure='status')
        self.write('main.sn','import "failed/src/api"\nfn main(): void =>\n  println(provide())\n')
        for target in ('c','rust'):
            with self.subTest(target=target):
                output=self.root/'main.exe'
                built=subprocess.run([str(COMPILER),'main.sn','--target',target,'--no-install','-o',str(output)],cwd=self.root,capture_output=True,timeout=60)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                run=subprocess.run([str(output)],cwd=self.root,capture_output=True,timeout=15)
                self.assertEqual(run.returncode,1)
                self.assertEqual(run.stdout,b'')
                expected=b"native package '"+(self.root/'.sn/failed/sn.yaml').as_posix().encode()+b"' export 'native_failed' failed: runtime ABI index or length out of range\n"
                if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                self.assertEqual(run.stderr,expected)

if __name__=='__main__':unittest.main(verbosity=2)
