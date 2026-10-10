#!/usr/bin/env python3
"""Compile original C/Rust/Go backing into archives and consume them independently."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('SN_COMPILER', ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))).resolve()


class NativeArtifactTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='sn-native-artifacts-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'package with spaces'
        self.root.mkdir()
        self.output = self.root / '.sn/artifacts'
        self.write('src/api.sn', 'native fn fromC(): int\nnative fn fromRust(): int\nnative fn fromGo(): int\nnative fn goText(): str\n')
        self.write('native/value.h', '#define NATIVE_VALUE 11\n')
        self.write('native/value.c', '#include "value.h"\nlong long from_c(void) { return NATIVE_VALUE; }\n')
        self.write('native/value.rs', '#[no_mangle]\npub extern "C" fn from_rs() -> i64 { 22 }\n')
        self.write('native/go/go.mod', 'module sindarin.test/native-package\n\ngo 1.26.0\n')
        self.write('native/go/main.go', 'package main\n\n/* #include <stdint.h>\n#include <stdlib.h>\n#include "sn_abi.h" */\nimport "C"\nimport "unsafe"\n'
                   '//export from_go\nfunc from_go() C.int64_t { return 33 }\n'
                   '//export from_go_string\nfunc from_go_string(out **C.SnAbiValue) C.uint32_t {\n'
                   ' text := C.CString("native Go backing")\n status := C.sn_abi_v1_string_copy(text, out)\n'
                   ' C.free(unsafe.Pointer(text))\n return status\n}\nfunc main() {}\n')
        self.manifest(['C', 'RS', 'GO'])

    def write(self, path, text):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def manifest(self, languages):
        text = 'name: native-package\nversion: 1.0.0\nruntime: RS\nnative:\n  abi: 1.0\n'
        text += '  declarations: [src/api.sn]\n  builds:\n'
        for lang in languages:
            stem, source = {'C': ('c', 'native/value.c'), 'RS': ('rs', 'native/value.rs'),
                            'GO': ('go', 'native/go/main.go')}[lang]
            text += f'    - name: {stem}\n      language: {lang}\n      sources: [{source}]\n'
            if lang == 'RS': text += f'      entry: {source}\n'
            if lang == 'GO': text += '      module: native/go\n'
        text += '  bindings:\n'
        for lang in languages:
            stem = {'C': 'c', 'RS': 'rs', 'GO': 'go'}[lang]
            text += (f'    - declaration: src/api.sn::{dict(C="fromC", RS="fromRust", GO="fromGo")[lang]}\n      build: {stem}\n'
                     f'      symbol: from_{stem}\n      convention: C\n      failure: abort\n'
                     '      ownership: {parameters: {}, result: value}\n')
        if 'GO' in languages:
            text += ('    - declaration: src/api.sn::goText\n      build: go\n'
                     '      symbol: from_go_string\n      convention: C\n      failure: status\n'
                     '      ownership: {parameters: {}, result: owned}\n')
        self.write('sn.yaml', text)

    def build(self, success=True, extra=()):
        command = [str(COMPILER), '--build-native', str(self.root / 'sn.yaml'),
                   '--target', 'rust', '-o', str(self.output)] + list(extra)
        result = subprocess.run(command, capture_output=True, timeout=300)
        if not success:
            self.assertNotEqual(result.returncode, 0)
            return result
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))
        summary = json.loads(result.stdout)
        metadata = json.loads(Path(summary['assembly']).read_text())
        return summary, metadata

    def test_original_three_language_archives_link_in_c_and_rust(self):
        summary, metadata = self.build()
        self.assertFalse(metadata['complete_package'])
        self.assertTrue(metadata['requires_generated_adapters'])
        self.assertFalse(metadata['cache_reusable'])
        self.assertEqual({u['language'] for u in metadata['units']}, {'C', 'RS', 'GO'})
        self.assertEqual(metadata['package']['runtime'], 'RS')
        base = Path(summary['assembly']).parent
        archives = [base / u['archive'] for u in metadata['units']]
        cc = shlex.split(os.environ.get('SN_CC', 'clang' if os.name == 'nt' or os.sys.platform == 'darwin' else 'gcc'))
        executable = self.root / 'consumer.exe'
        self.write('consumer.c', '#include <stdio.h>\n#include <string.h>\n#include "sn_abi.h"\n'
                   'SnAbiStatus from_go_string(SnAbiValue **out);\nlong long from_c(void); long long from_rs(void); '
                   'long long from_go(void);\nint main(void) { SnAbiValue *value = NULL; SnAbiBytes bytes;\n'
                   ' if (from_go_string(&value) || sn_abi_v1_bytes(value, &bytes) || bytes.length != 17 || '
                   'memcmp(bytes.data, "native Go backing", 17)) return 1;\n sn_abi_v1_release(value);\n'
                   ' printf("%lld,%lld,%lld\\n", from_c(), from_rs(), from_go()); }\n')
        flags = [flag for unit in metadata['units'] for flag in unit['native_link_flags']]
        if os.name != 'nt': flags.append('-pthread')
        build = subprocess.run(cc + ['-I', str(COMPILER.parent / 'include/runtime'), str(self.root / 'consumer.c')] +
                               [str(p) for p in archives] + [metadata['shared_runtime']['archive']] + flags + ['-o', str(executable)], capture_output=True, timeout=120)
        self.assertEqual(build.returncode, 0, build.stderr.decode(errors='replace'))
        run = subprocess.run([str(executable)], capture_output=True, timeout=10)
        self.assertEqual(run.returncode, 0, run.stderr.decode(errors='replace'))
        self.assertEqual(run.stdout.splitlines(), [b'11,22,33'])
        links = '#[link(name="sn_runtime_min", kind="static")] extern "C" {}\n' + '\n'.join(f'#[link(name="{p.stem.removeprefix("lib")}", kind="static")] extern "C" {{}}' for p in archives)
        self.write('consumer.rs', links + '\nextern "C" { fn from_c()->i64; fn from_rs()->i64; fn from_go()->i64; }\n'
                   '#[repr(C)] struct V { opaque: [u8; 0] }\n#[repr(C)] struct Bytes { data: *const u8, len: u64 }\n'
                   'extern "C" { fn from_go_string(out: *mut *mut V)->u32; fn sn_abi_v1_bytes(v:*const V,b:*mut Bytes)->u32; fn sn_abi_v1_release(v:*mut V); }\n'
                   'fn main() { unsafe { let mut v = std::ptr::null_mut(); let mut b = Bytes { data: std::ptr::null(), len: 0 }; '
                   'assert_eq!(from_go_string(&mut v), 0); assert_eq!(sn_abi_v1_bytes(v, &mut b), 0); '
                   'assert_eq!(std::slice::from_raw_parts(b.data,b.len as usize), b"native Go backing"); sn_abi_v1_release(v); '
                   'println!("{},{},{}", from_c(), from_rs(), from_go()); } }\n')
        rustc = shlex.split(os.environ.get('SN_RUSTC', 'rustc'))
        rustflags = shlex.split(os.environ.get('SN_RUSTFLAGS', ''))
        build = subprocess.run(rustc + ['--edition=2021', str(self.root / 'consumer.rs'), '-L', str(base), '-L', str(Path(metadata['shared_runtime']['archive']).parent),
                                      '-o', str(executable)] + rustflags +
                               (['-C', 'link-arg=-pthread'] if os.name != 'nt' else []),
                               capture_output=True, timeout=120)
        self.assertEqual(build.returncode, 0, build.stderr.decode(errors='replace'))
        run = subprocess.run([str(executable)], capture_output=True, timeout=10)
        self.assertEqual(run.returncode, 0, run.stderr.decode(errors='replace'))
        self.assertEqual(run.stdout.splitlines(), [b'11,22,33'])

    def test_cache_validates_headers_sources_and_artifact_bytes(self):
        self.manifest(['C', 'RS'])
        first, original = self.build()
        again, _ = self.build()
        self.assertTrue(again['cache_hit'])
        self.assertEqual(first['assembly'], again['assembly'])
        base = Path(first['assembly']).parent
        archive = base / original['units'][0]['archive']
        archive.write_bytes(b'corrupt')
        repaired, _ = self.build()
        self.assertFalse(repaired['cache_hit'])
        rebuilt = json.loads(Path(repaired['assembly']).read_text())
        rebuilt_archive = Path(repaired['assembly']).parent / rebuilt['units'][0]['archive']
        self.assertNotEqual(rebuilt_archive.read_bytes(), b'corrupt')
        self.assertEqual(archive.read_bytes(), b'corrupt')  # older generations are never overwritten
        self.assertNotEqual(repaired['assembly'], first['assembly'])
        corrupted_metadata = Path(repaired['assembly'])
        corrupt = json.loads(corrupted_metadata.read_text())
        corrupt['bindings'][0]['symbol'] = 'wrong_symbol'
        corrupted_metadata.write_text(json.dumps(corrupt))
        fixed, _ = self.build()
        self.assertFalse(fixed['cache_hit'])
        self.write('native/value.h', '#define NATIVE_VALUE 12\n')
        changed, _ = self.build()
        self.assertFalse(changed['cache_hit'])
        self.assertNotEqual(changed['assembly'], first['assembly'])
        mode, _ = self.build(extra=('-O0', '--unchecked'))
        self.assertNotEqual(mode['assembly'], changed['assembly'])

    def test_generated_exports_are_independent_typed_abi_artifacts(self):
        sources = {
            'C': '#include <stdlib.h>\n#include <string.h>\n#include <stdint.h>\n#include <stdbool.h>\n'
                 'uint32_t echo(char *v, char **out) { *out = v ? strdup(v) : NULL; return 0; }\n'
                 'uint32_t fail(long long *out) { return 5; }\nbool flip(bool v) { return !v; }\n',
            'RS': 'pub fn echo(v: Option<&[u8]>) -> Result<Option<Vec<u8>>,u32> { Ok(v.map(|v| v.to_vec())) }\n'
                  'pub fn fail() -> Result<i64,u32> { Err(5) }\npub fn flip(v:bool)->bool { !v }\n',
            'GO': 'package backing\nimport helper "sindarin.test/helper"\n'
                  'func Echo(v *string) (*string,uint32) { if v == nil { return nil,0 }; text := helper.Copy(*v); return &text,0 }\n'
                  'func Fail() (int64,uint32) { return 99,5 }\nfunc Flip(v bool) bool { return !v }\n'}
        self.write('native/helper/go.mod','module sindarin.test/helper\n\ngo 1.26.0\n')
        self.write('native/helper/helper.go','package helper\nfunc Copy(v string) string { return v }\n')
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or os.sys.platform=='darwin' else 'gcc'))
        for language,source in sources.items():
            with self.subTest(language=language):
                path={'C':'native/value.c','RS':'native/value.rs','GO':'native/go/main.go'}[language]
                self.write(path,source)
                self.manifest([language])
                if language=='GO':
                    self.write('native/go/go.mod','module sindarin.test/native-package\n\ngo 1.26.0\n'
                               'require sindarin.test/helper v0.0.0\nreplace sindarin.test/helper => ../helper\n')
                self.write('src/api.sn','native fn echo(text: str): str\nnative fn fail(): int\nnative fn flip(flag: bool): bool\n')
                manifest=self.root/'sn.yaml'
                text=manifest.read_text().split('  bindings:')[0]+'  bindings:\n'
                build={'C':'c','RS':'rs','GO':'go'}[language]
                for name,result,parameters,failure in (('echo','owned','{text: borrowed}','status'),('fail','value','{}','status'),('flip','value','{flag: value}','abort')):
                    function=name.capitalize() if language=='GO' else name
                    text+=(f'    - declaration: src/api.sn::{name}\n      build: {build}\n'
                           f'      symbol: provider_{name}\n      function: {function}\n      convention: C\n'
                           f'      failure: {failure}\n      ownership: {{parameters: {parameters}, result: {result}}}\n')
                manifest.write_text(text)
                original={p:p.read_bytes() for p in (manifest,self.root/path,self.root/'native/go/go.mod')}
                summary,metadata=self.build()
                self.assertEqual(metadata['units'][0]['generated_provider_exports'],['provider_echo','provider_fail','provider_flip'])
                self.assertEqual(len(metadata['provider_signatures']),3)
                for p,expected in original.items(): self.assertEqual(p.read_bytes(),expected)
                base=Path(summary['assembly']).parent
                self.write('provider-client.c', '#include <assert.h>\n#include <string.h>\n#include "sn_abi.h"\n'
                           'uint32_t provider_echo(SnAbiValue*,SnAbiValue**);\nuint32_t provider_fail(int64_t*);\nuint8_t provider_flip(uint8_t);\n'
                           'int main(void) { SnAbiValue *text=NULL,*out=NULL,*buffer=NULL; SnAbiBytes bytes;\n'
                           ' char data[]={65,(char)255,0,66,0};\n'
                           ' assert(sn_abi_v1_string_copy(data,&text)==0); assert(provider_echo(text,&out)==0);\n'
                           ' sn_abi_v1_release(text); assert(sn_abi_v1_bytes(out,&bytes)==0);\n'
                           ' assert(bytes.length==2 && bytes.data[0]==65 && bytes.data[1]==255); sn_abi_v1_release(out);\n'
                           ' assert(provider_echo(NULL,&out)==0 && out==NULL);\n'
                           ' assert(sn_abi_v1_string_copy("",&text)==0); assert(provider_echo(text,&out)==0 && out!=NULL);\n'
                           ' assert(sn_abi_v1_bytes(out,&bytes)==0 && bytes.length==0 && bytes.data!=NULL); sn_abi_v1_release(out);\n'
                           ' assert(sn_abi_v1_buffer_copy((const uint8_t*)"abc",3,&buffer)==0); out=text;\n'
                           ' assert(provider_echo(buffer,&out)==SN_ABI_WRONG_KIND && out==text);\n'
                           ' assert(provider_echo(text,NULL)==SN_ABI_INVALID_ARGUMENT);\n'
                           ' int64_t preserved=123; assert(provider_fail(&preserved)==5 && preserved==123);\n'
                           ' assert(provider_fail(NULL)==SN_ABI_INVALID_ARGUMENT);\n'
                           ' assert(provider_flip(0)==1 && provider_flip(1)==0);\n'
                           ' sn_abi_v1_release(buffer); sn_abi_v1_release(text); return 0; }\n')
                unit=metadata['units'][0]
                executable=self.root/'provider-client.exe'
                built=subprocess.run(cc+['-I',str(COMPILER.parent/'include/runtime'),str(self.root/'provider-client.c'),
                                         str(base/unit['archive']),metadata['shared_runtime']['archive']]+unit['native_link_flags']+
                                     ['-o',str(executable)],capture_output=True,timeout=120)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                run=subprocess.run([str(executable)],capture_output=True,timeout=15)
                self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))

    def test_multiple_go_build_units_produce_one_independent_archive(self):
        self.write('src/api.sn','native fn first(): int\nnative fn second(): int\n')
        builds,bindings=[],[]
        for name,value in (('first',11),('second',22)):
            self.write(f'native/{name}/go.mod',f'module sindarin.test/{name}\n\ngo 1.26.0\n')
            self.write(f'native/{name}/impl.go',f'package backing\nfunc Provide() int64 {{ return {value} }}\n')
            builds.append(f'    - name: {name}\n      language: GO\n      sources: [native/{name}/impl.go]\n      module: native/{name}\n')
            bindings.append(f'    - declaration: src/api.sn::{name}\n      build: {name}\n      symbol: native_{name}\n'
                            '      function: Provide\n      convention: C\n      failure: abort\n      ownership: {parameters: {}, result: value}\n')
        self.write('sn.yaml','name: multiple-go\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n'+''.join(builds)+'  bindings:\n'+''.join(bindings))
        summary,metadata=self.build()
        self.assertEqual(len(metadata['units']),2)
        self.assertEqual(len({u['archive'] for u in metadata['units']}),1)
        self.assertFalse(metadata['cache_reusable'])
        base=Path(summary['assembly']).parent
        self.write('aggregate-client.c','#include <assert.h>\n#include <stdint.h>\nint64_t native_first(void); int64_t native_second(void);\n'
                   'int main(void) { assert(native_first()==11); assert(native_second()==22); return 0; }\n')
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or os.sys.platform=='darwin' else 'gcc'))
        executable=self.root/'aggregate-client.exe'
        built=subprocess.run(cc+[str(self.root/'aggregate-client.c'),str(base/metadata['units'][0]['archive']),
                                 metadata['shared_runtime']['archive']]+metadata['units'][0]['native_link_flags']+
                             ['-o',str(executable)],capture_output=True,timeout=120)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        run=subprocess.run([str(executable)],capture_output=True,timeout=15)
        self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))

    def test_generated_owned_string_arrays_have_wire_lifetime_contracts(self):
        sources={
            'C':'#include "sn_array.h"\n#include <stdlib.h>\n#include <string.h>\n'
                'static void release(void *p) { free(*(char**)p); }\n'
                'SnArray *make(void) { SnArray *a=sn_array_new(sizeof(char*),4); a->elem_release=release; '
                'char *one=strdup("one"), *nil=NULL, *empty=strdup(""); '
                'sn_array_push(a,&one); sn_array_push(a,&nil); sn_array_push(a,&empty); return a; }\n'
                'uint32_t checked(long long mode, SnArray **out) { if (mode<0) return 5; '
                '*out=mode==0 ? NULL : mode==1 ? sn_array_new(sizeof(char*),4) : make(); return 0; }\n',
            'RS':'pub fn make()->Option<Vec<Option<Vec<u8>>>> { Some(vec![Some(b"one".to_vec()),None,Some(vec![])]) }\n'
                 'pub fn checked(mode:i64)->Result<Option<Vec<Option<Vec<u8>>>>,u32> { '
                 'if mode<0 { Err(5) } else { Ok(if mode==0 { None } else if mode==1 { Some(vec![]) } else { make() }) } }\n',
            'GO':'package backing\nfunc Make() []*string { one:="one"; empty:=""; return []*string{&one,nil,&empty} }\n'
                 'func Checked(mode int64) ([]*string,uint32) { if mode<0 { return nil,5 }; '
                 'if mode==0 { return nil,0 }; if mode==1 { return []*string{},0 }; return Make(),0 }\n'}
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or os.sys.platform=='darwin' else 'gcc'))
        for language,source in sources.items():
            with self.subTest(language=language):
                path={'C':'native/value.c','RS':'native/value.rs','GO':'native/go/main.go'}[language]
                self.write(path,source); self.manifest([language])
                self.write('src/api.sn','native fn items(): str[]\nnative fn checked(mode: int): str[]\n')
                manifest=self.root/'sn.yaml'
                text=manifest.read_text().split('  bindings:')[0].replace('abi: 1.0','abi: 1.1')
                build={'C':'c','RS':'rs','GO':'go'}[language]
                text+=f'  bindings:\n    - declaration: src/api.sn::items\n      build: {build}\n      symbol: provider_items\n'
                text+=f'      function: {"Make" if language=="GO" else "make"}\n      convention: C\n      failure: abort\n      ownership: {{parameters: {{}}, result: owned}}\n'
                text+=f'    - declaration: src/api.sn::checked\n      build: {build}\n      symbol: provider_checked\n'
                text+=f'      function: {"Checked" if language=="GO" else "checked"}\n      convention: C\n      failure: status\n      ownership: {{parameters: {{mode: value}}, result: owned}}\n'
                manifest.write_text(text)
                summary,metadata=self.build(); base=Path(summary['assembly']).parent; unit=metadata['units'][0]
                self.write('array-client.c','#include <assert.h>\n#include <string.h>\n#include "sn_abi.h"\n'
                           'SnAbiValue *provider_items(void);\nuint32_t provider_checked(int64_t,SnAbiValue**);\n'
                           'int main(void) { SnAbiValue *array=provider_items(),*one=NULL,*nil=NULL,*empty=NULL; uint64_t length; SnAbiBytes bytes;\n'
                           'assert(sn_abi_v1_value_array_length(array,&length)==0 && length==3);\n'
                           'assert(sn_abi_v1_value_array_get(array,0,&one)==0); assert(sn_abi_v1_value_array_get(array,1,&nil)==0 && nil==NULL);\n'
                           'assert(sn_abi_v1_value_array_get(array,2,&empty)==0 && empty!=NULL); sn_abi_v1_release(array);\n'
                           'assert(sn_abi_v1_string_bytes(one,&bytes)==0 && bytes.length==3 && memcmp(bytes.data,"one",3)==0);\n'
                           'assert(sn_abi_v1_string_bytes(empty,&bytes)==0 && bytes.length==0 && bytes.data!=NULL);\n'
                           'SnAbiValue *output=one; assert(provider_checked(-1,&output)==5 && output==one);\n'
                           'assert(provider_checked(2,NULL)==SN_ABI_INVALID_ARGUMENT);\n'
                           'assert(provider_checked(0,&output)==0 && output==NULL);\n'
                           'assert(provider_checked(1,&output)==0 && output!=NULL);\n'
                           'assert(sn_abi_v1_value_array_length(output,&length)==0 && length==0); sn_abi_v1_release(output);\n'
                           'assert(provider_checked(2,&output)==0 && output!=NULL);\n'
                           'assert(sn_abi_v1_value_array_length(output,&length)==0 && length==3); sn_abi_v1_release(output);\n'
                           'sn_abi_v1_release(one);sn_abi_v1_release(empty); return 0; }\n')
                executable=self.root/'array-client.exe'
                built=subprocess.run(cc+['-I',str(COMPILER.parent/'include/runtime'),str(self.root/'array-client.c'),
                                         str(base/unit['archive']),metadata['shared_runtime']['archive']]+unit['native_link_flags']+
                                     ['-o',str(executable)],capture_output=True,timeout=120)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                run=subprocess.run([str(executable)],capture_output=True,timeout=15)
                self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))

    def test_borrowed_string_array_provider_errors_aliases_and_lifetimes(self):
        sources={
            'C':'uint32_t echo(SnArray *a,SnArray *b,bool ok,SnArray **out) { '
                'if (!ok) return 5; if (a!=b) return 6; *out=a?sn_array_copy(a):NULL; return 0; }\n',
            'RS':'pub fn echo(a:Option<&[Option<&[u8]>]>,b:Option<&[Option<&[u8]>]>,ok:bool)'
                 '->Result<Option<Vec<Option<Vec<u8>>>>,u32> { if !ok { return Err(5) }; '
                 'if let (Some(a),Some(b))=(a,b) { if !std::ptr::eq(a,b) { return Err(6) } }; '
                 'Ok(a.map(|a|a.iter().map(|s|s.map(|s|s.to_vec())).collect())) }\n',
            'GO':'package backing\nimport "unsafe"\nfunc Echo(a,b []*string,ok bool) ([]*string,uint32) { '
                 'if !ok { return nil,5 }; if unsafe.SliceData(a)!=unsafe.SliceData(b) { return nil,6 }; '
                 'if a==nil { return nil,0 }; out:=make([]*string,len(a)); '
                 'for i,s:=range a { if s!=nil { text:=*s; out[i]=&text } }; return out,0 }\n'}
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or os.sys.platform=='darwin' else 'gcc'))
        for language,source in sources.items():
            with self.subTest(language=language):
                path={'C':'native/value.c','RS':'native/value.rs','GO':'native/go/main.go'}[language]
                self.write(path,source);self.manifest([language])
                self.write('src/api.sn','native fn echo(a: str[], b: str[], ok: bool): str[]\n')
                manifest=self.root/'sn.yaml';build={'C':'c','RS':'rs','GO':'go'}[language]
                text=manifest.read_text().split('  bindings:')[0].replace('abi: 1.0','abi: 1.1')
                text+=f'  bindings:\n    - declaration: src/api.sn::echo\n      build: {build}\n      symbol: provider_echo\n'
                text+=f'      function: {"Echo" if language=="GO" else "echo"}\n      convention: C\n      failure: status\n'
                text+='      ownership: {parameters: {a: borrowed, b: borrowed, ok: value}, result: owned}\n'
                manifest.write_text(text)
                summary,metadata=self.build();base=Path(summary['assembly']).parent;unit=metadata['units'][0]
                self.write('borrow-client.c','#include <assert.h>\n#include <string.h>\n#include "sn_abi.h"\n'
                    'uint32_t provider_echo(SnAbiValue*,SnAbiValue*,uint8_t,SnAbiValue**);\n'
                    'int main(void) { SnAbiValue *array=NULL,*text=NULL,*empty=NULL,*out=NULL,*buffer=NULL,*other=NULL; SnAbiBytes bytes;\n'
                    'assert(sn_abi_v1_value_array_new(&array)==0); assert(sn_abi_v1_string_copy("",&empty)==0);\n'
                    'char raw[]={65,(char)255,0,66}; assert(sn_abi_v1_string_copy(raw,&text)==0);\n'
                    'assert(sn_abi_v1_value_array_push(array,text)==0); assert(sn_abi_v1_value_array_push(array,NULL)==0);\n'
                    'assert(sn_abi_v1_value_array_push(array,empty)==0); assert(provider_echo(array,array,1,&out)==0);\n'
                    'sn_abi_v1_release(array); array=NULL; SnAbiValue *item=NULL;\n'
                    'assert(sn_abi_v1_value_array_get(out,0,&item)==0); assert(sn_abi_v1_string_bytes(item,&bytes)==0 && bytes.length==2 && bytes.data[1]==255);\n'
                    'sn_abi_v1_release(item); assert(sn_abi_v1_value_array_get(out,1,&item)==0 && item==NULL);\n'
                    'assert(sn_abi_v1_value_array_get(out,2,&item)==0); assert(sn_abi_v1_string_bytes(item,&bytes)==0 && bytes.length==0 && bytes.data!=NULL); sn_abi_v1_release(item);\n'
                    'SnAbiValue *preserved=out; assert(provider_echo(out,text,1,&out)==SN_ABI_WRONG_KIND && out==preserved);\n'
                    'assert(provider_echo(out,out,2,&out)==SN_ABI_INVALID_ARGUMENT && out==preserved);\n'
                    'assert(provider_echo(out,out,0,&out)==5 && out==preserved); assert(provider_echo(out,out,1,NULL)==SN_ABI_INVALID_ARGUMENT);\n'
                    'assert(sn_abi_v1_buffer_copy((const uint8_t*)"x",1,&buffer)==0); assert(sn_abi_v1_value_array_new(&array)==0);\n'
                    'assert(sn_abi_v1_value_array_push(array,text)==0); assert(sn_abi_v1_value_array_push(array,buffer)==0);\n'
                    'assert(provider_echo(array,array,1,&out)==SN_ABI_WRONG_KIND && out==preserved); sn_abi_v1_release(array);\n'
                    'assert(sn_abi_v1_value_array_new(&array)==0); assert(sn_abi_v1_value_array_new(&other)==0);\n'
                    'assert(provider_echo(array,other,1,&out)==6 && out==preserved); sn_abi_v1_release(other);\n'
                    'assert(provider_echo(array,array,1,&out)==0 && out!=NULL); sn_abi_v1_release(out);\n'
                    'assert(provider_echo(NULL,NULL,1,&out)==0 && out==NULL);\n'
                    'sn_abi_v1_release(array); sn_abi_v1_release(preserved); sn_abi_v1_release(text); sn_abi_v1_release(empty); sn_abi_v1_release(buffer); return 0; }\n')
                executable=self.root/'borrow-client.exe'
                built=subprocess.run(cc+['-I',str(COMPILER.parent/'include/runtime'),str(self.root/'borrow-client.c'),
                    str(base/unit['archive']),metadata['shared_runtime']['archive']]+unit['native_link_flags']+['-o',str(executable)],capture_output=True,timeout=120)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                run=subprocess.run([str(executable)],capture_output=True,timeout=15)
                self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))

    def test_plan_inheritance_and_command_diagnostics(self):
        self.manifest(['C'])
        manifest = self.root / 'sn.yaml'
        manifest.write_text(manifest.read_text().replace('runtime: RS\n', ''))
        result = subprocess.run([str(COMPILER), '--native-plan', str(manifest), '--target', 'rust'],
                                capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        plan = json.loads(result.stdout)
        self.assertEqual(plan['package']['runtime'], 'RS')
        original = manifest.read_bytes()
        bad = subprocess.run([str(COMPILER), '--build-native', str(manifest), '--emit-rust'],
                             capture_output=True, timeout=30)
        self.assertNotEqual(bad.returncode, 0)
        self.assertIn(b'cannot combine', bad.stderr)
        self.assertEqual(manifest.read_bytes(), original)
        self.assertFalse(self.output.exists())
        bad = subprocess.run([str(COMPILER), '--build-native', str(manifest), '--clean'],
                             capture_output=True, timeout=30)
        self.assertNotEqual(bad.returncode, 0)
        self.assertIn(b'cannot combine', bad.stderr)
        self.assertEqual(manifest.read_bytes(), original)

    def test_external_headers_invalidate_cached_generation(self):
        self.manifest(['C'])
        external = Path(self.temp.name) / 'external'
        external.mkdir()
        header = external / 'outside.h'
        header.write_text('#define OUTSIDE_VALUE 17\n')
        self.write('native/value.c', '#include "../../external/outside.h"\n'
                   'long long from_c(void) { return OUTSIDE_VALUE; }\n')
        first, original = self.build()
        self.assertIn(str(header.resolve()), original['dependency_sha256'])
        again, _ = self.build()
        self.assertTrue(again['cache_hit'])
        header.write_text('#define OUTSIDE_VALUE 18\n')
        changed, _ = self.build()
        self.assertFalse(changed['cache_hit'])
        self.assertNotEqual(first['assembly'], changed['assembly'])

    def test_missing_toolchain_is_diagnosed_without_publication(self):
        self.manifest(['C'])
        env = os.environ.copy()
        env['SN_CC'] = 'sindarin-toolchain-that-does-not-exist'
        result = subprocess.run([str(COMPILER), '--build-native', str(self.root / 'sn.yaml'),
                                 '-o', str(self.output)], capture_output=True, timeout=30, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b'native toolchain unavailable', result.stderr)
        self.assertFalse(self.output.exists())

    def test_c_backing_preserves_posix_string_declarations(self):
        self.manifest(['C'])
        self.write('native/value.c', '#include <stdlib.h>\n#include <string.h>\n'
                   'long long from_c(void) { char *s = strdup("native"); '
                   'long long size = (long long)strlen(s); free(s); return size; }\n')
        summary, metadata = self.build()
        archive = Path(summary['assembly']).parent / metadata['units'][0]['archive']
        self.write('posix.c', 'long long from_c(void); int main(void) { return from_c() == 6 ? 0 : 1; }\n')
        cc = shlex.split(os.environ.get('SN_CC') or ('clang' if os.name == 'nt' or os.sys.platform == 'darwin' else 'gcc'))
        executable = self.root / 'posix.exe'
        built = subprocess.run(cc + [str(self.root / 'posix.c'), str(archive), '-o', str(executable)],
                               capture_output=True, timeout=120)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        run = subprocess.run([str(executable)], capture_output=True, timeout=10)
        self.assertEqual(run.returncode, 0, run.stderr.decode(errors='replace'))

    def test_missing_input_export_and_tool_failure_do_not_publish(self):
        self.manifest(['C'])
        self.write('native/value.c', 'long long wrong_export(void) { return 0; }\n')
        failure = self.build(success=False)
        self.assertIn(b'native export missing', failure.stderr)
        self.assertFalse(list(self.output.rglob('assembly.json')))
        self.write('native/value.c', 'this is not valid C\n')
        failure = self.build(success=False)
        self.assertIn(b'native tool failed', failure.stderr)
        self.assertFalse(list(self.output.rglob('assembly.json')))
        (self.root / 'native/value.c').unlink()
        failure = self.build(success=False)
        self.assertIn(b'native input is missing', failure.stderr)
        self.assertFalse(list(self.output.rglob('assembly.json')))


if __name__ == '__main__':
    unittest.main(verbosity=2)
