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

    def package(self,name,language,source,result='value',failure='abort',function=None):
        suffix={'C':'c','RS':'rs','GO':'go'}[language]
        self.write(f'.sn/{name}/src/api.sn', 'native fn provide(): '+('str' if result=='owned' else 'int')+'\n')
        self.write(f'.sn/{name}/src/impl.{suffix}',source)
        manifest=f'name: {name}\nruntime: {language}\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n    - name: backing\n      language: {language}\n      sources: [src/impl.{suffix}]\n'
        if language=='RS':manifest+='      entry: src/impl.rs\n'
        if language=='GO':
            manifest+='      module: src\n'
            self.write(f'.sn/{name}/src/go.mod','module sindarin.test/'+name+'\n\ngo 1.26.0\n')
        manifest+=f'  bindings:\n    - declaration: src/api.sn::provide\n      build: backing\n      symbol: native_{name}\n      convention: C\n      failure: {failure}\n      ownership: {{parameters: {{}}, result: {result}}}\n'
        if function: manifest += f'      function: {function}\n'
        self.write(f'.sn/{name}/sn.yaml',manifest)

    def test_independent_sindarin_function_libraries_select_package_runtime(self):
        import hashlib,json
        for runtime in ('C','RS',None):
            name='body'+(runtime.lower() if runtime else 'inherited')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(text: str): str\nnative fn calculate(value: int): int\n')
            self.write(f'.sn/{name}/src/body.sn','import "./helper"\nfn echo(text: str): str =>\n  return text\n'
                       'fn calculate(value: int): int =>\n  return increment(value) + value\n')
            self.write(f'.sn/{name}/src/helper.sn','fn increment(value: int): int =>\n  return value + 1\n')
            text='name: '+name+'\n'+('runtime: '+runtime+'\n' if runtime else '')
            text+='native:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn, src/helper.sn]\n  bindings:\n'
            for declaration,function,symbol,parameter,result in (
                    ('provide','echo','native_'+name,'text: borrowed','owned'),
                    ('calculate','calculate','calculate_'+name,'value: value','value')):
                failure='status' if declaration=='calculate' else 'abort'
                text+=f'    - declaration: src/api.sn::{declaration}\n      function: {function}\n      symbol: {symbol}\n      build: body\n      convention: C\n      failure: {failure}\n      ownership: {{parameters: {{{parameter}}}, result: {result}}}\n'
            self.write(f'.sn/{name}/sn.yaml',text)
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  var input: str = "body lifetime"\n  var output: str = provide(input)\n'
                       '  input = "changed"\n  println(output)\n  println(provide(nil) == nil)\n'
                       '  println(provide("") == nil)\n  println(calculate(20))\n')
            for target in ('c','rust'):
                with self.subTest(runtime=runtime,target=target):self.execute(target,b'body lifetime\ntrue\nfalse\n41\n')
            if runtime:
                package=self.root/'.sn'/name;manifest=package/'sn.yaml'
                built=subprocess.run([str(COMPILER),'--build-package',str(manifest),'--target','rust','-o',str(package/'.sn/published')],capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                descriptor=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(descriptor.read_text())
                self.assertEqual(metadata['units'][0]['implementation_runtime'],runtime)
                destination=package/'dist';shutil.copytree(descriptor.parent,destination)
                digest=hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
                manifest.write_text(text.replace('native:\n','native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
                (package/'src/body.sn').unlink();(package/'src/helper.sn').unlink();shutil.rmtree(package/'.sn')
                for target in ('c','rust'):
                    with self.subTest(prebuilt=runtime,target=target):self.execute(target,b'body lifetime\ntrue\nfalse\n41\n')

    def test_sindarin_library_contracts_and_go_backend_are_not_silently_replaced(self):
        self.write('.sn/body/src/api.sn','native fn provide(): int\n')
        self.write('main.sn','import "body/src/api"\nfn main(): void =>\n  println(provide())\n')
        template='name: body\nruntime: {runtime}\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n  bindings:\n    - declaration: src/api.sn::provide\n      function: provide\n      symbol: body_provide\n      build: body\n      convention: C\n      failure: abort\n      ownership: {{parameters: {{}}, result: value}}\n'
        for runtime,source,message in (
                ('GO','fn provide(): int =>\n  return 11\n',b'Sindarin Go backend is not implemented'),
                ('C','fn provide(): str =>\n  return "wrong type"\n',b'implementation type/ownership differs'),
                ('C','var state: int = 11\nfn provide(): int =>\n  return state\n',b'library global/storage initialization'),
                ('C','fn main(): void =>\n  println("not a library")\nfn provide(): int =>\n  return 11\n',b'cannot contain an application main')):
            with self.subTest(runtime=runtime,source=source):
                self.write('.sn/body/sn.yaml',template.format(runtime=runtime))
                self.write('.sn/body/src/body.sn',source)
                result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','rejected.exe'],cwd=self.root,capture_output=True,timeout=90)
                self.assertNotEqual(result.returncode,0)
                self.assertIn(message,result.stderr)
                self.assertFalse((self.root/'rejected.exe').exists())

    def test_generated_providers_keep_ordinary_backing_functions(self):
        self.package('cdep','C','long long provide_impl(void) {return 11;}\n',function='provide_impl')
        self.package('rsdep','RS','fn internal() -> i64 { 22 }\npub fn provide_impl() -> i64 { crate::internal() }\n',function='provide_impl')
        self.package('godep','GO','package backing\nfunc Provide() int64 { return 33 }\n',function='Provide')
        self.write('main.sn','import "cdep/src/api" as CDep\nimport "rsdep/src/api" as RSDep\nimport "godep/src/api" as GoDep\nfn main(): void =>\n  println(CDep.provide() + RSDep.provide() + GoDep.provide())\n')
        for target in ('c','rust'):
            with self.subTest(target=target): self.execute(target,b'66\n')
        # Independently rebuilding the package reuses the typed contract/cache.
        result=subprocess.run([str(COMPILER),'--build-native',str(self.root/'.sn/rsdep/sn.yaml'),
                               '--target','rust','-o',str(self.root/'.sn/build/native-imports/artifacts')],capture_output=True,timeout=90)
        self.assertEqual(result.returncode,0,result.stderr.decode(errors='replace'))
        import json
        self.assertTrue(json.loads(result.stdout)['cache_hit'])

    def test_prebuilt_archives_consume_without_backing_sources_or_foreign_tools(self):
        import hashlib,json
        for language,source,function in (
                ('C','#include <stdlib.h>\n#include <string.h>\nchar *echo(char *text) { return text ? strdup(text) : NULL; }\n','echo'),
                ('RS','pub fn echo(text:Option<&[u8]>)->Option<Vec<u8>> { text.map(|s|s.to_vec()) }\n','echo'),
                ('GO','package backing\nfunc Echo(text *string) *string { if text==nil { return nil }; copy:=*text; return &copy }\n','Echo')):
            with self.subTest(language=language):
                name='prebuilt'+language.lower()
                self.package(name,language,source,result='owned',function=function)
                package=self.root/'.sn'/name
                self.write(f'.sn/{name}/src/api.sn','native fn provide(text: str): str\n')
                manifest=package/'sn.yaml';text=manifest.read_text().replace('parameters: {}','parameters: {text: borrowed}')
                manifest.write_text(text)
                built=subprocess.run([str(COMPILER),'--build-native',str(manifest),'--target','rust','-o',str(package/'.sn/artifacts')],
                                     capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                descriptor=Path(json.loads(built.stdout)['assembly'])
                destination=package/'dist';shutil.copytree(descriptor.parent,destination)
                # Producer locations are provenance, not consumer link locations.
                metadata=json.loads((destination/'assembly.json').read_text())
                metadata['shared_runtime']['archive']='/missing/producer/runtime/libsn_runtime_min.a'
                (destination/'assembly.json').write_text(json.dumps(metadata,indent=2)+'\n')
                digest=hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
                manifest.write_text(text.replace('native:\n','native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
                for path in list((package/'src').iterdir()):
                    if path.suffix!='.sn':path.unlink()
                shutil.rmtree(package/'.sn')
                self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                           '  var source: str = "owned lifetime"\n  var result: str = provide(source)\n'
                           '  source = "changed"\n  println(result)\n  println(provide(nil) == nil)\n  println(provide("") == nil)\n')
                previous={key:os.environ.get(key) for key in ('SN_GO','SN_AR','SN_NM')}
                try:
                    for key in previous:os.environ[key]=str(self.root/'missing-foreign-tool')
                    rustc=os.environ.get('SN_RUSTC')
                    os.environ['SN_RUSTC']=str(self.root/'missing-backing-rustc')
                    try:
                        selected=subprocess.run([str(COMPILER),'--build-native',str(manifest),'--target','c','-o',str(package/'.sn/unneeded')],capture_output=True,timeout=90)
                        self.assertEqual(selected.returncode,0,selected.stderr.decode(errors='replace'))
                        selection=json.loads(selected.stdout)
                        self.assertTrue(selection['prebuilt'] and selection['cache_hit'])
                        self.assertTrue(Path(selection['shared_runtime']['archive']).is_file())
                        self.execute('c',b'owned lifetime\ntrue\nfalse\n')
                    finally:
                        if rustc is None:os.environ.pop('SN_RUSTC',None)
                        else:os.environ['SN_RUSTC']=rustc
                    self.execute('rust',b'owned lifetime\ntrue\nfalse\n')
                finally:
                    for key,value in previous.items():
                        if value is None:os.environ.pop(key,None)
                        else:os.environ[key]=value
                # No backing build cache is recreated when consuming the artifact.
                self.assertFalse((package/'.sn').exists())

    def test_prebuilt_artifact_rejects_corruption_and_incompatible_contracts(self):
        import copy,hashlib,json
        self.package('sealed','C','long long value(void) { return 11; }\n',function='value')
        package=self.root/'.sn/sealed';manifest=package/'sn.yaml';original=manifest.read_text()
        built=subprocess.run([str(COMPILER),'--build-native',str(manifest),'--target','rust','-o',str(package/'.sn/artifacts')],
                             capture_output=True,timeout=120)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        descriptor=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(descriptor.read_text())
        self.write('main.sn','import "sealed/src/api"\nfn main(): void =>\n  println(provide())\n')
        def change(mapping,path,value):
            for key in path[:-1]:mapping=mapping[key]
            mapping[path[-1]]=value
        cases=[(('schema',),2,b'unsupported prebuilt native assembly schema'),
               (('package','version'),'incompatible',b'package identity/version/runtime differs'),
               (('abi',),'1.1',b'assembly ABI differs'),
               (('compatibility','pointer_bits'),8,b'platform/architecture/pointer width differs'),
               (('bindings',0,'ownership','result'),'owned',b'binding/ownership contract differs'),
               (('provider_signatures',0,'return_type','kind'),'string',b'resolved provider type contract differs'),
               (('declarations',0,'sha256'),'0'*64,b'public declaration bytes differ'),
               (('units',0,'symbols'),[],b'build/export/initialization contract differs'),
               (('units',0,'initialization'),'unsupported',b'build/export/initialization contract differs'),
               (('units',0,'archive_sha256'),'0'*64,b'archive is missing, outside the generation or corrupt'),
               (('units',0,'archive'),'../escape.a',b'archive is missing, outside the generation or corrupt'),
               (('units',),None,b'malformed prebuilt native assembly descriptor')]
        for path,value,message in cases:
            with self.subTest(path=path):
                corrupt=copy.deepcopy(metadata);change(corrupt,path,value)
                descriptor.write_text(json.dumps(corrupt)+'\n')
                digest=hashlib.sha256(descriptor.read_bytes()).hexdigest()
                manifest.write_text(original.replace('native:\n','native:\n  assembly: {path: '+descriptor.as_posix()+', sha256: '+digest+'}\n'))
                product=self.root/'rejected.exe'
                result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o',str(product)],
                                      cwd=self.root,capture_output=True,timeout=90)
                self.assertNotEqual(result.returncode,0)
                self.assertIn(message,result.stderr)
                self.assertFalse(product.exists())
        descriptor.write_text(json.dumps(metadata)+'\n')
        manifest.write_text(original.replace('native:\n','native:\n  assembly: {path: '+descriptor.as_posix()+', sha256: '+'0'*64+'}\n'))
        result=subprocess.run([str(COMPILER),'main.sn','--target','c','--no-install','-o','rejected.exe'],cwd=self.root,capture_output=True,timeout=90)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'assembly descriptor is missing or its sha256 differs',result.stderr)
        self.assertFalse((self.root/'rejected.exe').exists())

    def test_cross_runtime_package_methods_and_globals_require_package_compilation(self):
        self.package('fixed','C','long long value(void) { return 11; }\n',function='value')
        self.write('main.sn','import "fixed/src/api"\nfn main(): void =>\n  println(provide())\n')
        for body in ('struct Helper =>\n  fn result(): int =>\n    return 42\n',
                     'var packageState: int = 42\n'):
            with self.subTest(body=body):
                self.write('.sn/fixed/src/api.sn','native fn provide(): int\n'+body)
                result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','rejected.exe'],cwd=self.root,capture_output=True,timeout=90)
                self.assertNotEqual(result.returncode,0)
                self.assertIn(b'Sindarin package bodies require independent C compilation',result.stderr)
                self.assertFalse((self.root/'rejected.exe').exists())
                self.execute('c',b'11\n')

    def test_prebuilt_go_archives_require_one_runtime_graph(self):
        import hashlib,json
        for name,value in (('gofirst',11),('gosecond',22)):
            self.package(name,'GO',f'package backing\nfunc Provide() int64 {{ return {value} }}\n',function='Provide')
            package=self.root/'.sn'/name;manifest=package/'sn.yaml';text=manifest.read_text()
            result=subprocess.run([str(COMPILER),'--build-native',str(manifest),'--target','rust','-o',str(package/'.sn/artifacts')],capture_output=True,timeout=180)
            self.assertEqual(result.returncode,0,result.stderr.decode(errors='replace'))
            descriptor=Path(json.loads(result.stdout)['assembly']);digest=hashlib.sha256(descriptor.read_bytes()).hexdigest()
            manifest.write_text(text.replace('native:\n','native:\n  assembly: {path: '+descriptor.as_posix()+', sha256: '+digest+'}\n'))
        self.write('main.sn','import "gofirst/src/api" as First\nimport "gosecond/src/api" as Second\nfn main(): void =>\n  println(First.provide()+Second.provide())\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','rejected.exe'],cwd=self.root,capture_output=True,timeout=90)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'prebuilt Go runtime archives cannot be combined',result.stderr)
        self.assertFalse((self.root/'rejected.exe').exists())

    def test_generated_string_array_results_keep_nil_elements_and_mutation(self):
        sources={
            'C': '#include "sn_array.h"\n#include <stdlib.h>\n#include <string.h>\n'
                 'static void release(void *p) { free(*(char **)p); }\n'
                 'SnArray *make(long long mode) { if (!mode) return NULL; SnArray *a=sn_array_new(sizeof(char*),4); '
                 'a->elem_release=release; a->elem_tag=SN_TAG_STRING; if(mode==1) return a; '
                 'char *one=strdup("one"), *nil=NULL, *empty=strdup(""); '
                 'sn_array_push(a,&one); sn_array_push(a,&nil); sn_array_push(a,&empty); return a; }\n',
            'RS': 'pub fn make(mode:i64)->Option<Vec<Option<Vec<u8>>>> { if mode==0 { None } '
                  'else if mode==1 { Some(vec![]) } else { Some(vec![Some(b"one".to_vec()),None,Some(vec![])]) } }\n',
            'GO': 'package backing\nfunc Make(mode int64) []*string { if mode==0 { return nil }; '
                  'if mode==1 { return []*string{} }; one:="one"; empty:=""; return []*string{&one,nil,&empty} }\n'}
        for language,source in sources.items():
            name='arrays'+language.lower()
            self.package(name,language,source,result='owned',function='Make' if language=='GO' else 'make')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(mode: int): str[]\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            manifest.write_text(manifest.read_text().replace('abi: 1.0','abi: 1.1').replace('parameters: {}','parameters: {mode: value}'))
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  println(provide(0) == nil)\n  println(provide(1) == nil)\n'
                       '  var items: str[] = provide(2)\n  println(items.length)\n  println(items[0])\n'
                       '  println(items[1] == nil)\n  println(items[2] == nil)\n'
                       '  items[0] = "changed"\n  println(items[0])\n')
            for target in ('c','rust'):
                with self.subTest(language=language,target=target): self.execute(target,b'true\nfalse\n3\none\ntrue\nfalse\nchanged\n')

    def test_managed_array_result_requires_abi_1_1(self):
        self.package('oldabi','C','long long unused(void) { return 0; }\n',result='owned')
        self.write('.sn/oldabi/src/api.sn','native fn provide(): str[]\n')
        self.write('main.sn','import "oldabi/src/api"\nfn main(): void =>\n  var items: str[] = provide()\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                              cwd=self.root,capture_output=True,timeout=60)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'managed string-array results require native ABI 1.1',result.stderr)
        self.assertFalse((self.root/'main.exe').exists())

    def test_borrowed_string_arrays_preserve_nil_aliases_and_owned_results(self):
        sources={
            'C':'#include <stdlib.h>\nSnArray *echo(SnArray *a, SnArray *b) { if (a!=b) abort(); return a ? sn_array_copy(a) : NULL; }\n',
            'RS':'pub fn echo(a:Option<&[Option<&[u8]>]>,b:Option<&[Option<&[u8]>]>)->Option<Vec<Option<Vec<u8>>>> { '
                 'if let (Some(a),Some(b))=(a,b) { assert!(std::ptr::eq(a,b)); } '
                 'a.map(|v|v.iter().map(|s|s.map(|s|s.to_vec())).collect()) }\n',
            'GO':'package backing\nfunc Echo(a,b []*string) []*string { '
                 'if len(a)>0 && &a[0]!=&b[0] { panic("alias") }; if a==nil { return nil }; '
                 'out:=make([]*string,len(a)); for i,s:=range a { if s!=nil { text:=*s; out[i]=&text } }; return out }\n'}
        for language,source in sources.items():
            name='borrowarrays'+language.lower()
            self.package(name,language,source,result='owned',function='Echo' if language=='GO' else 'echo')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(a: str[], b: str[]): str[]\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            manifest.write_text(manifest.read_text().replace('abi: 1.0','abi: 1.1').replace('parameters: {}','parameters: {a: borrowed, b: borrowed}'))
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  var missing: str[] = nil\n  println(provide(missing, missing) == nil)\n'
                       '  var empty: str[] = {}\n  println(provide(empty, empty) == nil)\n'
                       '  var missingElement: str = nil\n  var original: str[] = {"one", missingElement, ""}\n'
                       '  var result: str[] = provide(original, original)\n  original[0] = "changed"\n'
                       '  println(result.length)\n  println(result[0])\n'
                       '  println(result[1] == nil)\n  println(result[2] == nil)\n')
            for target in ('c','rust'):
                with self.subTest(language=language,target=target):
                    self.execute(target,b'true\nfalse\n3\none\ntrue\nfalse\n')

    def test_borrowed_arrays_require_current_abi_and_readonly_ownership(self):
        self.package('badarray','C','long long unused(void) { return 0; }\n')
        self.write('.sn/badarray/src/api.sn','native fn provide(items: str[]): int\n')
        manifest=self.root/'.sn/badarray/sn.yaml';original=manifest.read_text()
        self.write('main.sn','import "badarray/src/api"\nfn main(): void =>\n  var items: str[] = {}\n  println(provide(items))\n')
        for abi,ownership,message in (('1.0','borrowed',b'managed string-array inputs require native ABI 1.1'),
                                      ('1.1','value',b'package input ownership requires')):
            with self.subTest(abi=abi,ownership=ownership):
                manifest.write_text(original.replace('abi: 1.0','abi: '+abi).replace('parameters: {}','parameters: {items: '+ownership+'}'))
                result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                                      cwd=self.root,capture_output=True,timeout=60)
                self.assertNotEqual(result.returncode,0)
                self.assertIn(message,result.stderr)
                self.assertFalse((self.root/'main.exe').exists())

    def test_generated_string_array_status_results_and_errors(self):
        sources={
            'C':'#include "sn_array.h"\nuint32_t make(long long mode, SnArray **out) { '
                'if (mode<0) return 5; *out=mode ? sn_array_new(sizeof(char*),4) : NULL; return 0; }\n',
            'RS':'pub fn make(mode:i64)->Result<Option<Vec<Option<Vec<u8>>>>,u32> { '
                 'if mode<0 { Err(5) } else { Ok(if mode==0 { None } else { Some(vec![]) }) } }\n',
            'GO':'package backing\nfunc Make(mode int64) ([]*string,uint32) { '
                 'if mode<0 { return nil,5 }; if mode==0 { return nil,0 }; return []*string{},0 }\n'}
        for language,source in sources.items():
            name='statusarrays'+language.lower()
            self.package(name,language,source,result='owned',failure='status',function='Make' if language=='GO' else 'make')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(mode: int): str[]\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            manifest.write_text(manifest.read_text().replace('abi: 1.0','abi: 1.1').replace('parameters: {}','parameters: {mode: value}'))
            for target in ('c','rust'):
                with self.subTest(language=language,target=target):
                    self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                               '  println(provide(0) == nil)\n  println(provide(1) == nil)\n')
                    self.execute(target,b'true\nfalse\n')
                    self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                               '  var items: str[] = provide(-1)\n  println("unreachable")\n')
                    output=self.root/'failed.exe'
                    built=subprocess.run([str(COMPILER),'main.sn','--target',target,'--no-install','-o',str(output)],
                                         cwd=self.root,capture_output=True,timeout=90)
                    self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                    run=subprocess.run([str(output)],cwd=self.root,capture_output=True,timeout=15)
                    self.assertEqual(run.returncode,1)
                    self.assertEqual(run.stdout,b'')
                    self.assertIn(b'failed: runtime ABI index or length out of range',run.stderr)

    def test_generated_string_providers_preserve_nil_empty_bytes_and_lifetime(self):
        sources={
            'C': '#include <stdlib.h>\n#include <string.h>\n#include <stdint.h>\nuint32_t echo(char *text, char **out) { *out = text ? strdup(text) : NULL; return 0; }\n',
            'RS': 'pub fn echo(text: Option<&[u8]>) -> Result<Option<Vec<u8>>, u32> { Ok(text.map(|v| v.to_vec())) }\n',
            'GO': 'package backing\nfunc Echo(text *string) (*string, uint32) { if text == nil { return nil, 0 }; value := *text; return &value, 0 }\n'}
        for language,source in sources.items():
            name='text'+language.lower()
            self.package(name,language,source,result='owned',failure='status',function='Echo' if language=='GO' else 'echo')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(text: str): str\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            manifest.write_text(manifest.read_text().replace('parameters: {}','parameters: {text: borrowed}'))
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n  var original: str = "lifetime"\n  var result: str = provide(original)\n  original = "changed"\n  println(result)\n  println(provide("temporary"))\n  println(provide(nil) == nil)\n  println(provide("") == nil)\n  println(provide(""))\n')
            for target in ('c','rust'):
                with self.subTest(language=language,target=target): self.execute(target,b'lifetime\ntemporary\ntrue\nfalse\n\n')

    def test_generated_status_provider_contains_foreign_panics(self):
        for language,source in (
                ('RS','pub fn fail() -> Result<i64,u32> { panic!("provider test panic") }\n'),
                ('GO','package backing\nfunc Fail() (int64,uint32) { panic(nil) }\n')):
            name='panic'+language.lower()
            self.package(name,language,source,failure='status',function='fail' if language=='RS' else 'Fail')
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n  println(provide())\n')
            for target in ('c','rust'):
                with self.subTest(language=language,target=target):
                    executable=self.root/'main.exe'
                    built=subprocess.run([str(COMPILER),'main.sn','--target',target,'--no-install','-o',str(executable)],cwd=self.root,capture_output=True,timeout=120)
                    self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                    run=subprocess.run([str(executable)],cwd=self.root,capture_output=True,timeout=15,
                                       env=dict(os.environ,GODEBUG='panicnil=1'))
                    self.assertEqual(run.returncode,1,run.stderr.decode(errors='replace'))
                    self.assertEqual(run.stdout,b'')
                    self.assertIn(b"failed: native backing function panicked",run.stderr)

    def test_generated_c_provider_rejects_backing_signature_mismatch(self):
        self.package('bad','C','int provide_impl(void) { return -1; }\n',function='provide_impl')
        self.write('main.sn','import "bad/src/api"\nfn main(): void =>\n  println(provide())\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                              cwd=self.root,capture_output=True,timeout=60)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'provide_impl',result.stderr)
        self.assertFalse((self.root/'main.exe').exists())

    def test_generated_providers_isolate_same_named_backing_functions(self):
        for name,language,value in (('firstc','C',11),('secondc','C',22),('firstrs','RS',33),('secondrs','RS',44)):
            source=(f'long long provide_impl(void) {{ return {value}; }}\n' if language=='C'
                    else f'pub fn provide_impl() -> i64 {{ {value} }}\n')
            self.package(name,language,source,function='provide_impl')
        self.write('main.sn','import "firstc/src/api" as FirstC\nimport "secondc/src/api" as SecondC\n'
                   'import "firstrs/src/api" as FirstRS\nimport "secondrs/src/api" as SecondRS\n'
                   'fn main(): void =>\n  println(FirstC.provide())\n  println(SecondC.provide())\n'
                   '  println(FirstRS.provide())\n  println(SecondRS.provide())\n')
        for target in ('c','rust'):
            with self.subTest(target=target): self.execute(target,b'11\n22\n33\n44\n')

    def test_two_go_packages_share_one_runtime_and_module_instance(self):
        self.write('shared/go.mod','module sindarin.test/shared\n\ngo 1.26.0\n')
        self.write('shared/counter.go','package shared\nvar count int64\nfunc Next() int64 { count++; return count }\n')
        originals={}
        for name,base in (('firstgo',100),('secondgo',200)):
            self.package(name,'GO',f'package backing\nimport "sindarin.test/shared"\n'
                         f'func Provide() int64 {{ return {base}+shared.Next() }}\n',function='Provide')
            module=self.root/f'.sn/{name}/src/go.mod'
            module.write_text(module.read_text()+'require sindarin.test/shared v0.0.0\n'
                              'replace sindarin.test/shared => ../../../shared\n')
            originals[module]=module.read_bytes()
        self.write('main.sn','import "firstgo/src/api" as First\nimport "secondgo/src/api" as Second\n'
                   'fn main(): void =>\n  println(First.provide())\n  println(Second.provide())\n  println(First.provide())\n')
        for target in ('c','rust'):
            with self.subTest(target=target): self.execute(target,b'101\n202\n103\n')
        for path,expected in originals.items(): self.assertEqual(path.read_bytes(),expected)
        import json
        responses=list((self.root/'.sn/build/native-imports').glob('response-*.json'))
        self.assertEqual(len(responses),2)
        for response in responses:
            links=json.loads(response.read_text())['links']
            go_archives=[p for p in links if p.endswith('libsn_go_graph.a')]
            self.assertEqual(len(go_archives),1,links)
            self.assertFalse(any('artifacts/' in p and p.endswith('.a') for p in links),links)

    def test_go_graph_rejects_conflicting_module_replacements(self):
        for name,value in (('firstgo',1),('secondgo',2)):
            self.package(name,'GO',f'package backing\nfunc Provide() int64 {{ return {value} }}\n',function='Provide')
            self.write(f'{name}/go.mod','module sindarin.test/shared\n\ngo 1.26.0\n')
            self.write(f'{name}/helper.go','package shared\n')
            module=self.root/f'.sn/{name}/src/go.mod'
            module.write_text(module.read_text()+f'replace sindarin.test/shared => ../../../{name}\n')
        self.write('main.sn','import "firstgo/src/api" as First\nimport "secondgo/src/api" as Second\n'
                   'fn main(): void =>\n  println(First.provide()+Second.provide())\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                              cwd=self.root,capture_output=True,timeout=90)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'conflicting Go module replacements',result.stderr)
        self.assertFalse((self.root/'main.exe').exists())

    def test_go_graph_owned_strings_keep_nil_and_call_lifetimes(self):
        for name,prefix in (('firstgo','one:'),('secondgo','two:')):
            self.package(name,'GO',f'package backing\nfunc Echo(v *string) (*string,uint32) {{ '
                         f'if v==nil {{ return nil,0 }}; text:="{prefix}"+*v; return &text,0 }}\n',
                         result='owned',failure='status',function='Echo')
            self.write(f'.sn/{name}/src/api.sn','native fn provide(text: str): str\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            manifest.write_text(manifest.read_text().replace('parameters: {}','parameters: {text: borrowed}'))
        self.write('main.sn','import "firstgo/src/api" as First\nimport "secondgo/src/api" as Second\n'
                   'fn main(): void =>\n  var original: str = "alive"\n  var first: str = First.provide(original)\n'
                   '  var second: str = Second.provide(original)\n  original = "changed"\n'
                   '  println(first)\n  println(second)\n  println(First.provide(nil)==nil)\n  println(Second.provide(""))\n')
        for target in ('c','rust'):
            with self.subTest(target=target): self.execute(target,b'one:alive\ntwo:alive\ntrue\ntwo:\n')

    def test_go_graph_rejects_handwritten_main_archive_combination(self):
        self.package('legacygo','GO','package main\n/* #include <stdint.h> */\nimport "C"\n'
                     '//export native_legacygo\nfunc native_legacygo() C.int64_t { return 11 }\nfunc main() {}\n')
        self.package('othergo','GO','package backing\nfunc Provide() int64 { return 22 }\n',function='Provide')
        self.write('main.sn','import "legacygo/src/api" as Legacy\nimport "othergo/src/api" as Other\n'
                   'fn main(): void =>\n  println(Legacy.provide()+Other.provide())\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                              cwd=self.root,capture_output=True,timeout=90)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b'handwritten main archives cannot be combined',result.stderr)
        self.assertFalse((self.root/'main.exe').exists())

    def test_packages_cannot_silently_shadow_the_same_wire_export(self):
        for name,value in (('first',11),('second',22)):
            self.package(name,'C',f'long long provide_impl(void) {{ return {value}; }}\n',function='provide_impl')
        manifest=self.root/'.sn/second/sn.yaml'
        manifest.write_text(manifest.read_text().replace('native_second','native_first'))
        self.write('main.sn','import "first/src/api" as First\nimport "second/src/api" as Second\n'
                   'fn main(): void =>\n  println(First.provide() + Second.provide())\n')
        result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','main.exe'],
                              cwd=self.root,capture_output=True,timeout=60)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(b"native export symbol 'native_first' is shared by packages",result.stderr)
        self.assertFalse((self.root/'main.exe').exists())

    def test_generated_providers_convert_scalar_parameters_and_void_results(self):
        sources={
            'C': '#include <stdbool.h>\n#include <stdint.h>\nstatic long long count;\n'
                 'double calculate(long long a, uint32_t b, bool flag) { return a+b+(flag?1:0); }\n'
                 'bool flip(bool flag) { return !flag; }\nvoid touch(void) { ++count; }\n'
                 'uint32_t touched(long long *out) { *out=count; return 0; }\n',
            'RS': 'use std::sync::atomic::{AtomicI64,Ordering};\nstatic COUNT:AtomicI64=AtomicI64::new(0);\n'
                  'pub fn calculate(a:i64,b:u32,flag:bool)->f64 { (a+i64::from(b)+i64::from(flag)) as f64 }\n'
                  'pub fn flip(flag:bool)->bool { !flag }\npub fn touch() { COUNT.fetch_add(1,Ordering::SeqCst); }\n'
                  'pub fn touched()->Result<i64,u32> { Ok(COUNT.load(Ordering::SeqCst)) }\n',
            'GO': 'package backing\nvar count int64\n'
                  'func Calculate(a int64,b uint32,flag bool) float64 { extra:=int64(0); if flag { extra=1 }; return float64(a+int64(b)+extra) }\n'
                  'func Flip(flag bool) bool { return !flag }\nfunc Touch() { count++ }\n'
                  'func Touched() (int64,uint32) { return count,0 }\n'}
        for language,source in sources.items():
            name='scalar'+language.lower()
            self.package(name,language,source,function='Calculate' if language=='GO' else 'calculate')
            self.write(f'.sn/{name}/src/api.sn','native fn calculate(a: int, b: uint32, flag: bool): double\n'
                       'native fn flip(flag: bool): bool\nnative fn touch(): void\nnative fn touched(): int\n')
            manifest=self.root/f'.sn/{name}/sn.yaml'
            text=manifest.read_text().split('  bindings:')[0]+'  bindings:\n'
            for function,parameters,failure in (('calculate','{a: value, b: value, flag: value}','abort'),
                                               ('flip','{flag: value}','abort'),('touch','{}','abort'),('touched','{}','status')):
                backing=function.capitalize() if language=='GO' else function
                text+=(f'    - declaration: src/api.sn::{function}\n      build: backing\n'
                       f'      symbol: native_{name}_{function}\n      function: {backing}\n      convention: C\n'
                       f'      failure: {failure}\n      ownership: {{parameters: {parameters}, result: value}}\n')
            manifest.write_text(text)
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  println(calculate(-40, 2, true))\n  println(flip(true))\n  touch()\n  touch()\n  println(touched())\n')
            for target in ('c','rust'):
                with self.subTest(language=language,target=target): self.execute(target,b'-37.00000\nfalse\n2\n')

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
