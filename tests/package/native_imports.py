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

    def test_package_owned_records_preserve_storage_fields_aliases_and_cleanup(self):
        self.write('.sn/records/src/api.sn',
                   '@alias "LegacyRecord"\nnative struct Record as ref =>\n'
                   '  @alias "number"\n  amount: int32\n'
                   '  @alias "text"\n  label: str\n'
                   '  @alias "record_amount"\n  native fn getAmount(): int32\n'
                   'native fn make(value: int32): Record\n'
                   'native fn borrow(a: Record, b: Record): Record\n'
                   'native fn inspect(a: Record, b: Record): int\n'
                   'native fn checked(a: Record, b: Record, flag: bool, mode: int, text: str): Record\n'
                   'native fn destroyed(): int\n'
                   'native fn created(): int\n')
        self.write('.sn/records/src/record.h',
                   '#ifndef TEST_RECORD_H\n#define TEST_RECORD_H\n#include <stdint.h>\n'
                   '#include "record_fields.h"\n'
                   'CanonicalRecord *record_create(void);\n'
                   'CanonicalRecord *record_retain(CanonicalRecord *value);\n'
                   'void record_release(CanonicalRecord *value);\n'
                   'int record_refs(CanonicalRecord *value);\n'
                   'int32_t record_amount(CanonicalRecord *value);\n#endif\n')
        public=self.root/'.sn/records/src/record.h'
        public.write_text(public.read_text().replace('#endif\n',
            'void record_set_callback(void (*callback)(CanonicalRecord *));\n'
            'long long record_checked_calls(void);\n#endif\n'))
        self.write('.sn/records/src/record_fields.h',
                   'typedef struct { int __rc__; int32_t number; char *text; } CanonicalRecord;\n')
        self.write('.sn/records/src/record.c',
                   '#include "record.h"\n#include <stdlib.h>\n#include <string.h>\n'
                   '#include "sn_abi.h"\n#include <stdbool.h>\n'
                   'static int born, dead, calls; static void (*visit)(CanonicalRecord *);\n'
                   'void record_set_callback(void (*callback)(CanonicalRecord *)) { visit=callback; }\n'
                   'long long record_checked_calls(void) { return calls; }\n'
                   'CanonicalRecord *record_create(void) { CanonicalRecord *p=calloc(1,sizeof(*p)); '
                   'if(!p) abort(); p->__rc__=1; born++; return p; }\n'
                   'CanonicalRecord *record_retain(CanonicalRecord *p) { if(p) __atomic_add_fetch(&p->__rc__,1,__ATOMIC_RELAXED); return p; }\n'
                   'void record_release(CanonicalRecord *p) { if(p && __atomic_sub_fetch(&p->__rc__,1,__ATOMIC_ACQ_REL)==0) { dead++; free(p->text); free(p); } }\n'
                   'int record_refs(CanonicalRecord *p) { return p ? __atomic_load_n(&p->__rc__,__ATOMIC_RELAXED) : 0; }\n'
                   'int32_t record_amount(CanonicalRecord *p) { return p ? p->number : -1; }\n'
                   'CanonicalRecord *make_record(int32_t value) { if(value<0) return NULL; CanonicalRecord *p=record_create(); '
                   'p->number=value; p->text=strdup("original C"); return p; }\n'
                   'CanonicalRecord *borrow_record(CanonicalRecord *a, CanonicalRecord *b) { if(a!=b) abort(); return a; }\n'
                   'long long inspect_record(CanonicalRecord *a, CanonicalRecord *b) { return a==b && a && a->number==9 && !strcmp(a->text,"changed"); }\n'
                   'long long destroyed_records(void) { return dead; }\n'
                   'long long created_records(void) { return born; }\n')
        backing=self.root/'.sn/records/src/record.c'
        backing.write_text(backing.read_text()+
            'uint32_t checked_record(CanonicalRecord *a, CanonicalRecord *b, bool flag, long long mode, char *text, CanonicalRecord **out) { '
            'calls++; if(mode) return SN_ABI_OUT_OF_RANGE; if(!flag) return SN_ABI_INVALID_ARGUMENT; '
            'if(a!=b || !text || strcmp(text,"credit text")) abort(); if(visit) visit(a); '
            'if(strcmp(text,"credit text")) abort(); *out=a; return SN_ABI_OK; }\n')
        manifest=('name: records\nruntime: C\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
                  '  types:\n    - declaration: src/api.sn::Record\n      identity: records/Record@1\n'
                  '      c_type: CanonicalRecord\n      header: src/record.h\n      create: record_create\n'
                  '      retain: record_retain\n      release: record_release\n      refs: record_refs\n      owner: atomic\n'
                  '  builds:\n    - name: backing\n      language: C\n      sources: [src/record.c]\n      include_dirs: [src]\n'
                  '  bindings:\n')
        for name,function,ownership,result in (
                ('make','make_record','value: value','owned'),
                ('borrow','borrow_record','a: borrowed, b: borrowed','borrowed'),
                ('inspect','inspect_record','a: borrowed, b: borrowed','value'),
                ('destroyed','destroyed_records','','value'),
                ('created','created_records','','value')):
            manifest+=(f'    - declaration: src/api.sn::{name}\n      function: {function}\n'
                       f'      symbol: native_records_{name}\n      build: backing\n      convention: C\n      failure: abort\n'
                       f'      ownership: {{parameters: {{{ownership}}}, result: {result}'+(', borrowed_from: a' if result=='borrowed' else '')+'}\n')
        self.write('.sn/records/sn.yaml',manifest)
        manifest+=('    - declaration: src/api.sn::checked\n      function: checked_record\n'
                   '      symbol: native_records_checked\n      build: backing\n      convention: C\n      failure: status\n'
                   '      ownership: {parameters: {a: borrowed, b: borrowed, flag: value, mode: value, text: borrowed}, result: borrowed, borrowed_from: a}\n')
        self.write('.sn/records/sn.yaml',manifest)
        self.write('main.sn','import "records/src/api"\nnative fn createLocal(): Record =>\n'
                   '  return Record {amount: 3, label: "local"}\nfn exercise(): void =>\n'
                   '  var first: Record = make(7)\n  var alias: Record = first\n'
                   '  alias.amount = 9\n  alias.label = "changed"\n  println(first.amount)\n  println(first.label)\n'
                   '  println(first.getAmount())\n  println(inspect(first, alias))\n'
                   '  var borrowed: Record = borrow(first, alias)\n  println(inspect(borrowed, first))\n'
                   '  println(make(-1) == nil)\n  println(borrow(nil, nil) == nil)\n'
                   '  var local: Record = createLocal()\n  println(local.getAmount())\n'
                   'fn main(): void =>\n  exercise()\n  println(created())\n  println(destroyed())\n')
        expected=b'9\nchanged\n9\n1\n1\ntrue\ntrue\n3\n2\n2\n'
        import hashlib,json
        original_root=self.root
        package=self.root/'.sn/records'
        for optimization in ('-O0','-O1','-O2'):
            for arithmetic in (None,'--checked','--unchecked'):
                flags=(optimization,)+((arithmetic,) if arithmetic else ())
                for target in ('c','rust'):
                    with self.subTest(target=target,flags=flags,archive='source'):
                        self.execute(target,expected,flags)
                built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),
                                      '--target','rust',*flags,'-o',str(self.root/'artifacts')],
                                     cwd=self.root,capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly=Path(json.loads(built.stdout)['assembly'])
                metadata=json.loads(assembly.read_text())
                self.assertEqual({item['path'] for item in metadata['record_headers']},
                                 {'src/record.h','src/record_fields.h'})
                relocated=self.root/'relocated consumer'
                if relocated.exists():shutil.rmtree(relocated)
                copied=relocated/'.sn/records'
                shutil.copytree(package,copied)
                shutil.copytree(assembly.parent,copied/'.sn/sealed')
                (copied/'src/record.c').unlink()
                (copied/'sn.yaml').write_text(manifest.replace('  abi: 1.5\n',
                    '  abi: 1.5\n  assembly:\n    path: .sn/sealed/assembly.json\n    sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'\n'))
                shutil.copyfile(self.root/'main.sn',relocated/'main.sn')
                self.root=relocated
                try:
                    for target in ('c','rust'):
                        with self.subTest(target=target,flags=flags,archive='relocated prebuilt'):
                            self.execute(target,expected,flags)
                finally:self.root=original_root
        import re
        caller=(self.root/'main.sn').read_text().replace('import "records/src/api"',
            'import "records/src/api"\nimport "records/src/api" as Records')
        caller=re.sub(r'\b(make|borrow|inspect|created|destroyed)\(',r'Records.\1(',caller)
        for location in (original_root,relocated):
            (location/'main.sn').write_text(caller)
            self.root=location
            try:
                for target in ('c','rust'):
                    with self.subTest(target=target,archive='namespaced '+location.name):
                        self.execute(target,expected)
            finally:self.root=original_root
        # Direct wire clients exercise credit loss during reentry, tag/argument
        # validation and output preservation independently of generated callers.
        self.write('record-client.c',
            '#include <assert.h>\n#include <stdbool.h>\n#include <stddef.h>\n#include "sn_abi.h"\n#include "record.h"\n'
            'extern SnAbiValue *native_records_make(int32_t);\n'
            'extern SnAbiStatus native_records_checked(SnAbiValue *,SnAbiValue *,uint8_t,int64_t,SnAbiValue *,SnAbiValue **);\n'
            'extern int64_t native_records_destroyed(void);\n'
            'static SnAbiValue *owner,*text; static int visited;\n'
            'static void consume(CanonicalRecord *p) { assert(p && p->number==9 && record_refs(p)==1); '
            'sn_abi_v1_release(owner); owner=NULL; sn_abi_v1_release(text); text=NULL; visited++; assert(p->number==9); }\n'
            'int main(void) { owner=native_records_make(9); assert(owner); assert(sn_abi_v1_string_copy("credit text",&text)==SN_ABI_OK); void *p=NULL; '
            'assert(sn_abi_v1_resource_data_typed(owner,"records/Record@1",&p)==SN_ABI_OK); '
            'SnAbiValue *out=(SnAbiValue *)(uintptr_t)1,*wrong=NULL; '
            'assert(sn_abi_v1_resource_new_typed("other/Record@1",p,NULL,0,&wrong)==SN_ABI_OK); '
            'assert(native_records_checked(wrong,owner,1,0,text,&out)==SN_ABI_WRONG_KIND && out==(SnAbiValue *)(uintptr_t)1); '
            'sn_abi_v1_release(wrong); '
            'assert(native_records_checked(owner,owner,2,0,text,&out)==SN_ABI_INVALID_ARGUMENT && out==(SnAbiValue *)(uintptr_t)1); '
            'assert(record_checked_calls()==0 && record_refs(p)==1); '
            'assert(native_records_checked(owner,owner,1,1,text,&out)==SN_ABI_OUT_OF_RANGE && out==(SnAbiValue *)(uintptr_t)1); '
            'assert(record_checked_calls()==1 && native_records_destroyed()==0); '
            'record_set_callback(consume); assert(native_records_checked(owner,owner,1,0,text,&out)==SN_ABI_OK); '
            'assert(!owner && visited==1 && out); void *result=NULL; '
            'assert(sn_abi_v1_resource_data_typed(out,"records/Record@1",&result)==SN_ABI_OK && result==p && record_refs(result)==1); '
            'assert(native_records_destroyed()==0); sn_abi_v1_release(out); assert(native_records_destroyed()==1); }\n')
        import shlex
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or os.sys.platform=='darwin' else 'gcc'))
        runtime=COMPILER.parent/'lib'/('clang' if os.name=='nt' else 'gcc')/'libsn_runtime_min.a'
        client=self.root/'record-client.exe'
        compiled=subprocess.run(cc+['-std=c11','-UNDEBUG']+shlex.split(os.environ.get('SN_CFLAGS',''))+
            ['-I',str(COMPILER.parent/'include/runtime'),'-I',str(package/'src'),str(self.root/'record-client.c')]+
            [str(assembly.parent/unit['archive']) for unit in metadata['units']]+[str(runtime),'-o',str(client)],
            cwd=self.root,capture_output=True,timeout=90)
        self.assertEqual(compiled.returncode,0,compiled.stderr.decode(errors='replace'))
        checked=subprocess.run([str(client)],capture_output=True,timeout=15)
        self.assertEqual(checked.returncode,0,checked.stderr.decode(errors='replace'))
        self.assertEqual(checked.stdout,b'');self.assertEqual(checked.stderr,b'')
        # Public transitive layout headers are sealed even after backing removal.
        (copied/'src/record_fields.h').write_text('typedef struct { int __rc__; float number; char *text; } CanonicalRecord;\n')
        for target in ('c','rust'):
            rejected=subprocess.run([str(COMPILER),'main.sn','--no-install','--target',target,
                                     '--emit-source','-o',str(relocated/'rejected.source')],
                                    cwd=relocated,capture_output=True,timeout=30)
            self.assertNotEqual(rejected.returncode,0)
            self.assertIn('public record header bytes differ',rejected.stderr.decode(errors='replace'))
            self.assertFalse((relocated/'rejected.source').exists())

    def test_generated_record_adapters_consume_canonical_sdk_textfile_archive(self):
        import hashlib,json
        sdk=ROOT/'.sn/sdk-native-integration'
        self.assertTrue((sdk/'src/io/textfile.native.c').is_file(),'pinned SDK integration checkout is required')
        package=self.root/'.sn/sdkrecord'
        shutil.copytree(sdk/'src/io',package/'src/io')
        original=(sdk/'src/io/textfile.sn').read_text()
        start=original.index('@alias "RtTextFile"')
        end=original.index('  # ===',start)
        fields=original[start:end]
        self.write('.sn/sdkrecord/src/api.sn',fields+
            '  @alias "sn_text_file_is_eof"\n  native fn isEof(): bool\n'
            '  @alias "sn_text_file_dispose"\n  native fn dispose(): void\n'
            'native fn sdkOpen(path: str): TextFile\n'
            'native fn sdkLine(file: TextFile): str\n'
            'native fn sdkLines(file: TextFile): str[]\n'
            'native fn sdkFinals(): int\nnative fn sdkCloses(): int\n')
        self.write('.sn/sdkrecord/src/record_contract.h',
            '#ifndef SDK_RECORD_CONTRACT_H\n#define SDK_RECORD_CONTRACT_H\n'
            '#define SN_SDK_TEXTFILE_STANDALONE 1\n#include "sn_minimal.h"\n#include "io/textfile.native.h"\n'
            'SnSdkTextFileRecord *bridge_sdk_create(void);\n'
            'void bridge_sdk_release(SnSdkTextFileRecord *file);\n'
            'int bridge_sdk_refs(SnSdkTextFileRecord *file);\n#endif\n')
        self.write('.sn/sdkrecord/src/bridge.c',
            '#include <stdio.h>\nstatic int closes;\n'
            'static int tracked_close(FILE *file) { closes++; return fclose(file); }\n'
            '#define fclose tracked_close\n#include "io/textfile.native.c"\n#undef fclose\nstatic int finalized;\n'
            'SnSdkTextFileRecord *bridge_sdk_create(void) { return sn_sdk_text_file_new(); }\n'
            'int bridge_sdk_refs(SnSdkTextFileRecord *file) { return file ? __atomic_load_n(&file->__rc__,__ATOMIC_RELAXED) : 0; }\n'
            'void bridge_sdk_release(SnSdkTextFileRecord *file) { if(file && bridge_sdk_refs(file)==1) finalized++; sn_sdk_text_file_release(file); }\n'
            'SnSdkTextFileRecord *bridge_open(char *path) { return sn_text_file_open(path); }\n'
            'char *bridge_line(SnSdkTextFileRecord *file) { return sn_text_file_read_line(file); }\n'
            'SnArray *bridge_lines(SnSdkTextFileRecord *file) { return sn_text_file_read_lines(file); }\n'
            'long long bridge_finals(void) { return finalized; }\n'
            'long long bridge_closes(void) { return closes; }\n')
        manifest=('name: sdkrecord\nruntime: C\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
            '  types:\n    - declaration: src/api.sn::TextFile\n'
            '      identity: SindarinSDK/sindarin-pkg-sdk:io.TextFile@1\n'
            '      c_type: SnSdkTextFileRecord\n      header: src/record_contract.h\n'
            '      create: bridge_sdk_create\n      retain: sn_sdk_text_file_retain\n'
            '      release: bridge_sdk_release\n      refs: bridge_sdk_refs\n      owner: atomic\n'
            '  builds:\n    - name: sdk\n      language: C\n      sources: [src/bridge.c]\n      include_dirs: [src]\n'
            '  bindings:\n')
        for name,function,parameters,result in (
                ('sdkOpen','bridge_open','path: borrowed','owned'),
                ('sdkLine','bridge_line','file: borrowed','owned'),
                ('sdkLines','bridge_lines','file: borrowed','owned'),
                ('sdkFinals','bridge_finals','','value'),
                ('sdkCloses','bridge_closes','','value')):
            manifest+=(f'    - declaration: src/api.sn::{name}\n      function: {function}\n'
                f'      symbol: native_sdk_record_{name}\n      build: sdk\n      convention: C\n      failure: abort\n'
                f'      ownership: {{parameters: {{{parameters}}}, result: {result}}}\n')
        self.write('.sn/sdkrecord/sn.yaml',manifest)
        self.write('data.txt','alpha\nbeta\ngamma\n')
        self.write('main.sn','import "sdkrecord/src/api"\nfn exercise(): void =>\n'
            '  var file: TextFile = sdkOpen("data.txt")\n  var alias: TextFile = file\n'
            '  println(file._path)\n  alias._path = "changed path"\n  println(file._path)\n'
            '  println(file._is_open)\n  println(sdkLine(alias))\n  var lines: str[] = sdkLines(file)\n'
            '  println(lines.length)\n  println(lines[0])\n  println(lines[1])\n  println(alias.isEof())\n'
            '  file.dispose()\n  println(alias._is_open)\n  println(lines[1])\n'
            'fn savedLines(): str[] =>\n  var file: TextFile = sdkOpen("data.txt")\n'
            '  var alias: TextFile = file\n  return sdkLines(alias)\n'
            'fn main(): void =>\n  exercise()\n  var held: str[] = savedLines()\n'
            '  println(held[0])\n  println(held[2])\n  println(sdkFinals())\n  println(sdkCloses())\n')
        expected=b'data.txt\nchanged path\n1\nalpha\n2\nbeta\ngamma\ntrue\n0\ngamma\nalpha\ngamma\n2\n2\n'
        modes=[(optimization,)+((arithmetic,) if arithmetic else ())
               for optimization in ('-O0','-O1','-O2') for arithmetic in (None,'--checked','--unchecked')]
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='source'):self.execute(target,expected,flags)
        built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'--target','rust',
                              '-o',str(self.root/'artifacts')],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        assembly=Path(json.loads(built.stdout)['assembly'])
        shutil.copytree(assembly.parent,package/'.sn/sealed')
        (package/'sn.yaml').write_text(manifest.replace('  abi: 1.5\n',
            '  abi: 1.5\n  assembly:\n    path: .sn/sealed/assembly.json\n    sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'\n'))
        (package/'src/bridge.c').unlink()
        # Only canonical public headers remain; consuming the archive cannot
        # compile the SDK implementation or its original Sindarin facade again.
        for pattern in ('*.c','*.sn'):
            for file in (package/'src/io').glob(pattern):file.unlink()
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='prebuilt'):self.execute(target,expected,flags)

    def test_native_package_adapts_only_loaded_api_modules(self):
        import hashlib,json
        package=self.root/'.sn/multimodule'
        self.write('.sn/multimodule/src/numbers.sn','native fn number(): int\n')
        self.write('.sn/multimodule/src/words.sn','native fn word(): str\n')
        self.write('.sn/multimodule/src/backing.c','#include <string.h>\n'
            'long long number(void) { return 17; }\nchar *word(void) { return strdup("second module"); }\n')
        manifest=('name: multimodule\nruntime: C\nnative:\n  abi: 1.0\n'
            '  declarations: [src/numbers.sn, src/words.sn]\n  builds:\n'
            '    - name: backing\n      language: C\n      sources: [src/backing.c]\n  bindings:\n')
        for file,function,result in (('numbers','number','value'),('words','word','owned')):
            manifest+=(f'    - declaration: src/{file}.sn::{function}\n      function: {function}\n'
                f'      symbol: multimodule_{function}\n      build: backing\n      convention: C\n'
                f'      failure: abort\n      ownership: {{parameters: {{}}, result: {result}}}\n')
        self.write('.sn/multimodule/sn.yaml',manifest)
        callers=[('import "multimodule/src/numbers"\nfn main(): void =>\n  println(number())\n',b'17\n'),
                 ('import "multimodule/src/words"\nfn main(): void =>\n  println(word())\n',b'second module\n'),
                 ('import "multimodule/src/numbers" as Numbers\nimport "multimodule/src/words" as Words\n'
                  'fn main(): void =>\n  println(Numbers.number())\n  println(Words.word())\n',b'17\nsecond module\n')]
        for archive in ('source','prebuilt'):
            if archive=='prebuilt':
                built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'-o',str(package/'dist-build')],
                    cwd=self.root,capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly=Path(json.loads(built.stdout)['assembly']);shutil.copytree(assembly.parent,package/'dist')
                (package/'sn.yaml').write_text(manifest.replace('  abi: 1.0\n',
                    '  abi: 1.0\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                (package/'src/backing.c').unlink()
            for index,(caller,expected) in enumerate(callers):
                self.write('main.sn',caller)
                for target in ('c','rust'):
                    with self.subTest(archive=archive,caller=index,target=target):self.execute(target,expected)
        # A missing callable in an imported API must still be diagnosed.
        self.write('.sn/multimodule/src/numbers.sn','native fn renamed(): int\n')
        self.write('main.sn','import "multimodule/src/numbers"\nfn main(): void =>\n  println(renamed())\n')
        for target in ('c','rust'):
            result=subprocess.run([str(COMPILER),'main.sn','--no-install','--target',target,'-o',str(self.root/'bad')],
                cwd=self.root,capture_output=True,timeout=30)
            self.assertNotEqual(result.returncode,0)
            self.assertIn(b'requires a supported native declaration',result.stderr)

    def test_length_less_comparisons_preserve_single_evaluation(self):
        self.write('main.sn','var calls: int = 0\nfn text(): str =>\n  calls += 1\n  return "abc"\n'
            'fn values(): int[] =>\n  calls += 1\n  return {1,2}\nfn main(): void =>\n'
            '  println(text().length < 4)\n  println(calls)\n'
            '  println(values().length < 3)\n  println(calls)\n')
        modes=[(o,)+((a,) if a else ()) for o in ('-O0','-O1','-O2') for a in (None,'--checked','--unchecked')]
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags):self.execute(target,b'true\n1\ntrue\n2\n',flags)

    def test_typed_nested_native_arrays_preserve_inputs_and_owned_copies(self):
        import hashlib,json
        package=self.root/'.sn/typedarrays'
        self.write('.sn/typedarrays/src/api.sn','native fn mutate(a: int32[][], b: int32[][]): int32[][]\n')
        self.write('.sn/typedarrays/src/body.c','#include "sn_array.h"\n#include "sn_abi.h"\n'
            'SnArray *mutate(SnArray*a,SnArray*b) { if(a!=b) abort(); if(!a) return NULL; '
            'SnArray *row=((SnArray**)a->data)[0]; int32_t added=99; ((int32_t*)row->data)[0]=42; '
            'sn_array_push(row,&added); if(((SnArray**)b->data)[0]!=row || row->len!=3) abort(); '
            'SnAbiNativeArrayType type={SN_ABI_ARRAY_INT32,2}; SnAbiValue *view=NULL,*copy=NULL; SnArray *out=NULL; '
            'if(sn_abi_v1_native_array_borrow(a,type,&view) || sn_abi_v1_native_array_copy(view,&copy) || '
            'sn_abi_v1_native_array_take(&copy,type,&out)) abort(); sn_abi_v1_release(view); return out; }\n')
        manifest=('name: typedarrays\nruntime: C\nnative:\n  abi: 1.7\n  declarations: [src/api.sn]\n'
            '  builds:\n    - name: backing\n      language: C\n      sources: [src/body.c]\n'
            '  bindings:\n    - declaration: src/api.sn::mutate\n      function: mutate\n      symbol: typed_arrays_mutate\n'
            '      build: backing\n      convention: C\n      failure: abort\n'
            '      ownership: {parameters: {a: borrowed, b: borrowed}, result: owned}\n')
        self.write('.sn/typedarrays/sn.yaml',manifest)
        self.write('main.sn','import "typedarrays/src/api"\nfn main(): void =>\n'
            '  println(mutate(nil,nil)==nil)\n  var rows: int32[][] = {{1,2}}\n'
            '  var result: int32[][] = mutate(rows,rows)\n  println(rows[0][0])\n  println(rows[0].length)\n'
            '  rows[0][0]=7\n  println(result[0][0])\n  println(result[0][2])\n')
        modes=[(o,)+((a,) if a else ()) for o in ('-O0','-O1','-O2') for a in (None,'--checked','--unchecked')]
        for archive in ('source','prebuilt'):
            if archive=='prebuilt':
                built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'-o',str(package/'dist-build')],
                    cwd=self.root,capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly=Path(json.loads(built.stdout)['assembly']);shutil.copytree(assembly.parent,package/'dist')
                (package/'sn.yaml').write_text(manifest.replace('  abi: 1.7\n','  abi: 1.7\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                (package/'src/body.c').unlink()
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(target=target,flags=flags,archive=archive):
                        self.execute(target,b'true\n42\n3\n42\n99\n',flags)

    def test_native_byte_arrays_preserve_live_inputs_and_owned_results(self):
        import hashlib,json
        for language in ('C','RS','GO'):
            name='bytes'+language.lower(); package=self.root/'.sn'/name
            self.write(f'.sn/{name}/src/api.sn','native fn mutate(a: byte[], b: byte[]): byte[]\n')
            self.write(f'.sn/{name}/src/ops.h','#include <stdint.h>\n'
                'uint32_t test_byte_mutate(void*,void*);\nconst uint8_t *test_byte_data(void*,uint64_t*);\n')
            self.write(f'.sn/{name}/src/ops.c','#include "sn_array.h"\n#include "ops.h"\n'
                'uint32_t test_byte_mutate(void *left,void *right) { SnArray *a=left; if(left!=right) return 1; if(!a) return 0; '
                'a->elem_tag=SN_TAG_BYTE; unsigned char value=128; if(!a->len) { sn_array_push(a,&value); return 0; } '
                '((unsigned char*)a->data)[0]=42; value=255; for(int i=0;i<64;i++) sn_array_push(a,&value); '
                'if(((SnArray*)right)->len!=68 || ((unsigned char*)((SnArray*)right)->data)[0]!=42) abort(); '
                'value=0; sn_array_push(a,&value); return 0; }\n'
                'const uint8_t *test_byte_data(void *value,uint64_t *length) { SnArray *a=value; *length=a?(uint64_t)a->len:0; return a?a->data:NULL; }\n')
            source={
                'C':'#include "sn_array.h"\n#include "ops.h"\nuint32_t mutate(SnArray*a,SnArray*b,SnArray**out) { uint32_t status=test_byte_mutate(a,b); if(status) return status; *out=sn_array_copy(a); return 0; }\n',
                'RS':'use std::ffi::c_void; extern "C" { fn test_byte_mutate(a:*mut c_void,b:*mut c_void)->u32; fn test_byte_data(a:*mut c_void,length:*mut u64)->*const u8; }\n'
                     'pub fn mutate(a:*mut c_void,b:*mut c_void)->Result<Option<Vec<u8>>,u32> { unsafe { let status=test_byte_mutate(a,b); if status!=0 { return Err(status); } '
                     'let mut length=0; let data=test_byte_data(a,&mut length); Ok(if data.is_null() { None } else { Some(std::slice::from_raw_parts(data,length as usize).to_vec()) }) } }\n',
                'GO':'package backing\n/* #include "../ops.h" */\nimport "C"\nimport "unsafe"\n'
                     'func Mutate(a,b unsafe.Pointer) ([]byte,uint32) { if status:=C.test_byte_mutate(a,b); status!=0 { return nil,uint32(status) }; var length C.uint64_t; '
                     'data:=C.test_byte_data(a,&length); if data==nil { return nil,0 }; return C.GoBytes(unsafe.Pointer(data),C.int(length)),0 }\n'}[language]
            suffix={'C':'c','RS':'rs','GO':'go'}[language]; path='src/backing/main.go' if language=='GO' else 'src/impl.'+suffix
            self.write(f'.sn/{name}/'+path,source)
            if language=='GO':self.write(f'.sn/{name}/src/backing/go.mod','module sindarin.test/'+name+'\n\ngo 1.26.0\n')
            manifest=(f'name: {name}\nruntime: {language}\nnative:\n  abi: 1.6\n  declarations: [src/api.sn]\n  builds:\n'
                f'    - name: backing\n      language: {language}\n      sources: [{path}]\n      include_dirs: [src]\n')
            if language=='RS':manifest+='      entry: '+path+'\n'
            if language=='GO':manifest+='      module: src/backing\n'
            manifest+='    - name: operations\n      language: C\n      sources: [src/ops.c]\n'
            manifest+=('  bindings:\n    - declaration: src/api.sn::mutate\n      function: '+('Mutate' if language=='GO' else 'mutate')+
                f'\n      build: backing\n      symbol: {name}_mutate\n      convention: C\n      failure: status\n'
                '      ownership: {parameters: {a: borrowed, b: borrowed}, result: owned}\n')
            self.write(f'.sn/{name}/sn.yaml',manifest)
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                '  println(mutate(nil,nil) == nil)\n  var empty: byte[] = {}\n  var fresh=mutate(empty,empty)\n'
                '  println(empty.length)\n  println(fresh[0])\n'
                '  var bytes: byte[] = {0,127,128,255}\n  var result=mutate(bytes,bytes)\n'
                '  println(bytes.length)\n  println(bytes[0])\n  println(bytes[68])\n  bytes[0]=9\n'
                '  println(result.length)\n  println(result[0])\n  println(result[2])\n  println(result[3])\n  println(result[68])\n')
            expected=b'true\n1\n0x80\n69\n0x2A\n0x00\n69\n0x2A\n0x80\n0xFF\n0x00\n'
            modes=[(o,)+((a,) if a else ()) for o in ('-O0','-O1','-O2') for a in (None,'--checked','--unchecked')]
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(language=language,target=target,flags=flags):self.execute(target,expected,flags)
            built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'--target','rust','-o',str(package/'dist-build')],
                cwd=self.root,capture_output=True,timeout=180)
            self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
            assembly=Path(json.loads(built.stdout)['assembly']); shutil.copytree(assembly.parent,package/'dist')
            (package/'sn.yaml').write_text(manifest.replace('  abi: 1.6\n','  abi: 1.6\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
            for file in (package/'src').rglob('*'):
                if file.is_file() and file.suffix in ('.c','.rs','.go'):file.unlink()
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(prebuilt=language,target=target,flags=flags):self.execute(target,expected,flags)

    def test_sindarin_byte_array_bodies_preserve_mixed_array_arguments(self):
        import hashlib,json
        for runtime in ('C','RS'):
            name='bytebody'+runtime.lower(); package=self.root/'.sn'/name
            self.write(f'.sn/{name}/src/api.sn','native fn mutate(bytes: byte[], alias: byte[]): byte[]\n'
                'native fn label(values: str[], bytes: byte[]): str[]\n')
            self.write(f'.sn/{name}/src/observer.h','#include "sn_array.h"\n'
                'static inline void observe_bytes(SnArray *a) { if(!a || a->len!=68 || ((unsigned char*)a->data)[0]!=42) abort(); '
                'unsigned char byte=0; sn_array_push(a,&byte); }\n')
            self.write(f'.sn/{name}/src/body.sn','@include "observer.h"\n@alias "observe_bytes"\nnative fn observe(bytes: byte[]): void\n'
                'fn mutate(bytes: byte[], alias: byte[]): byte[] =>\n  if bytes == nil =>\n    return nil\n'
                '  if bytes.length == 0 =>\n    bytes.push(128)\n    return alias\n'
                '  bytes[0]=42\n  for i in 0..64 =>\n    bytes.push(255)\n  observe(alias)\n  return alias\n'
                'fn label(values: str[], bytes: byte[]): str[] =>\n  values.push("binary")\n  bytes.push(0)\n  return values\n')
            manifest=(f'name: {name}\nruntime: {runtime}\nnative:\n  abi: 1.6\n  declarations: [src/api.sn]\n  builds:\n'
                '    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n      include_dirs: [src]\n  bindings:\n')
            for function,parameters in (('mutate','bytes: borrowed, alias: borrowed'),('label','values: borrowed, bytes: borrowed')):
                manifest+=(f'    - declaration: src/api.sn::{function}\n      function: {function}\n      symbol: {name}_{function}\n'
                    '      build: body\n      convention: C\n      failure: status\n'
                    '      ownership: {parameters: {'+parameters+'}, result: owned}\n')
            self.write(f'.sn/{name}/sn.yaml',manifest)
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                '  println(mutate(nil,nil) == nil)\n  var empty: byte[] = {}\n  var fresh=mutate(empty,empty)\n'
                '  println(empty.length)\n  println(fresh[0])\n  var bytes: byte[] = {0,127,128,255}\n'
                '  var result=mutate(bytes,bytes)\n  println(bytes.length)\n  println(bytes[0])\n  println(bytes[68])\n'
                '  bytes[0]=9\n  println(result.length)\n  println(result[0])\n  println(result[2])\n  println(result[3])\n'
                '  println(result[68])\n  var names: str[] = {"first"}\n  var labels=label(names,bytes)\n'
                '  println(names.length)\n  println(labels[1])\n  println(bytes.length)\n')
            expected=b'true\n1\n0x80\n69\n0x2A\n0x00\n69\n0x2A\n0x80\n0xFF\n0x00\n2\nbinary\n70\n'
            modes=[(o,)+((a,) if a else ()) for o in ('-O0','-O1','-O2') for a in (None,'--checked','--unchecked')]
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(runtime=runtime,target=target,flags=flags):self.execute(target,expected,flags)
            built=subprocess.run([str(COMPILER),'--build-package',str(package/'sn.yaml'),'--target','rust','-o',str(package/'dist-build')],
                cwd=self.root,capture_output=True,timeout=180)
            self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
            assembly=Path(json.loads(built.stdout)['assembly']); shutil.copytree(assembly.parent,package/'dist')
            (package/'sn.yaml').write_text(manifest.replace('  abi: 1.6\n','  abi: 1.6\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
            (package/'src/body.sn').unlink(); (package/'src/observer.h').unlink()
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(prebuilt=runtime,target=target,flags=flags):self.execute(target,expected,flags)

    def test_unchanged_sdk_bytes_facade_is_an_independent_c_library(self):
        import hashlib,json
        sdk=Path(os.environ.get('SN_SDK_ROOT',ROOT/'.sn/sdk-native-integration')).resolve()
        package=self.root/'.sn/sindarin-pkg-sdk'
        for name in ('bytes.sn','bytes.sn.c','bytes.native.c','bytes.native.h'):
            source=sdk/'src/io'/name
            self.assertTrue(source.is_file(),'independent SDK Bytes module is required: '+str(source))
            destination=package/'src/io'/name
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,destination)
        original=(package/'src/io/bytes.sn').read_bytes()
        manifest=('name: sindarin-pkg-sdk\nruntime: C\nnative:\n  abi: 1.6\n'
            '  declarations: [src/io/bytes.sn]\n  builds:\n'
            '    - name: native\n      language: C\n      sources: [src/io/bytes.native.c]\n'
            '      provides_sources: [src/io/bytes.sn.c]\n      include_dirs: [src/io]\n'
            '    - name: facade\n      language: SN\n      entry: src/io/bytes.sn\n'
            '      sources: [src/io/bytes.sn]\n      include_dirs: [src/io]\n  bindings:\n')
        for declaration,function,symbol,build,parameter in (
            ('sn_bytes_from_hex','sn_sdk_bytes_from_hex','sdk_bytes_helper_hex','native','hex'),
            ('sn_bytes_from_base64','sn_sdk_bytes_from_base64','sdk_bytes_helper_base64','native','b64'),
            ('Bytes.fromHex','Bytes.fromHex','sdk_bytes_method_hex','facade','hexString'),
            ('Bytes.fromBase64','Bytes.fromBase64','sdk_bytes_method_base64','facade','base64String')):
            manifest+=(f'    - declaration: src/io/bytes.sn::{declaration}\n'
                f'      function: {function}\n      symbol: {symbol}\n      build: {build}\n'
                '      convention: C\n      failure: abort\n'
                f'      ownership: {{parameters: {{{parameter}: borrowed}}, result: owned}}\n')
        self.write('.sn/sindarin-pkg-sdk/sn.yaml',manifest)
        shutil.copyfile(sdk/'tests/io/test_bytes.sn',self.root/'main.sn')
        expected=(sdk/'tests/io/test_bytes.expected').read_bytes().replace(b'\r\n',b'\n')
        modes=[(optimization,)+((arithmetic,) if arithmetic else ())
               for optimization in ('-O0','-O1','-O2') for arithmetic in (None,'--checked','--unchecked')]
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='source'):self.execute(target,expected,flags)
        built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'--target','rust',
            '-o',str(self.root/'artifacts')],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        assembly=Path(json.loads(built.stdout)['assembly'])
        shutil.copytree(assembly.parent,package/'dist')
        self.assertEqual((package/'src/io/bytes.sn').read_bytes(),original)
        (package/'sn.yaml').write_text(manifest.replace('  abi: 1.6\n',
            '  abi: 1.6\n  assembly:\n    path: dist/assembly.json\n    sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'\n'))
        (package/'src/io/bytes.native.c').unlink(); (package/'src/io/bytes.sn.c').unlink()
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='prebuilt'):self.execute(target,expected,flags)
        self.assertEqual((package/'src/io/bytes.sn').read_bytes(),original)
        self.write('main.sn','import "sindarin-pkg-sdk/src/io/bytes"\nfn main(): void =>\n'
            '  var bytes = sn_bytes_from_hex("0080ff")\n  println(bytes.toHex())\n'
            '  println(sn_bytes_from_base64("AID/").toHex())\n')
        for target in ('c','rust'):
            with self.subTest(helper=target):self.execute(target,b'0080ff\n0080ff\n')
        self.write('main.sn','import "sindarin-pkg-sdk/src/io/bytes"\nnative fn inspect(): void =>\n'
            '  var marker: Bytes = Bytes {_unused: 7}\n  println(marker._unused)\n  println(sizeof(Bytes))\n'
            '  println(Bytes.fromHex("0080ff").toHex())\nfn main(): void =>\n  inspect()\n')
        for target in ('c','rust'):
            with self.subTest(namespace=target):self.execute(target,b'7\n4\n0080ff\n')

    def test_unchanged_sdk_binaryfile_facade_is_an_independent_c_library(self):
        import hashlib,json
        sdk=Path(os.environ.get('SN_SDK_ROOT',ROOT/'.sn/sdk-native-integration')).resolve()
        package=self.root/'.sn/sindarin-pkg-sdk'
        for name in ('binaryfile.sn','binaryfile.sn.c','binaryfile.native.c','binaryfile.native.h'):
            source=sdk/'src/io'/name
            self.assertTrue(source.is_file(),'independent SDK BinaryFile module is required: '+str(source))
            destination=package/'src/io'/name
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,destination)
        original=(package/'src/io/binaryfile.sn').read_bytes()
        self.write('.sn/sindarin-pkg-sdk/sn.yaml','name: sindarin-pkg-sdk\n')
        model_path=self.root/'sdk-model.json'
        emitted=subprocess.run([str(COMPILER),str(package/'src/io/binaryfile.sn'),'--no-install',
            '--package-body','--emit-model','-o',str(model_path)],cwd=self.root,capture_output=True,timeout=30)
        self.assertEqual(emitted.returncode,0,emitted.stderr.decode(errors='replace'))
        model=json.loads(model_path.read_text())
        record=next(s for s in model['structs'] if s['name']=='BinaryFile')
        manifest=('name: sindarin-pkg-sdk\nruntime: C\nnative:\n  abi: 1.6\n'
            '  declarations: [src/io/binaryfile.sn]\n  types:\n'
            '    - declaration: src/io/binaryfile.sn::BinaryFile\n'
            '      identity: SindarinSDK/sindarin-pkg-sdk:io.BinaryFile@1\n'
            '      c_type: SnSdkBinaryFileRecord\n      header: src/io/binaryfile.native.h\n'
            '      create: sn_sdk_binary_file_create\n      retain: sn_sdk_binary_file_retain\n'
            '      release: sn_sdk_binary_file_release\n      refs: sn_sdk_binary_file_refs\n      owner: atomic\n'
            '  builds:\n    - name: native\n      language: C\n'
            '      sources: [src/io/binaryfile.native.c]\n      provides_sources: [src/io/binaryfile.sn.c]\n'
            '      include_dirs: [src/io]\n    - name: facade\n      language: SN\n'
            '      entry: src/io/binaryfile.sn\n      sources: [src/io/binaryfile.sn]\n      include_dirs: [src/io]\n'
            '  bindings:\n')
        for method in record['methods']:
            name=method['name'];result=method['return_type']['kind']
            parameters=[] if method['is_static'] else ['self: borrowed']
            for parameter in method['params']:
                ownership='borrowed' if parameter['type']['kind'] in ('string','array','struct') else 'value'
                parameters.append(parameter['name']+': '+ownership)
            manifest+=(f'    - declaration: src/io/binaryfile.sn::BinaryFile.{name}\n'
                f'      function: BinaryFile.{name}\n      symbol: sdk_binary_file_method_{name}\n'
                '      build: facade\n      convention: C\n      failure: abort\n'
                '      ownership: {parameters: {'+', '.join(parameters)+'}, result: '+
                ('owned' if result in ('string','array','struct') else 'value')+'}\n')
        for function in model['functions']:
            name=function['name'];result=function['return_type']['kind']
            parameters=[]
            for parameter in function['params']:
                ownership='borrowed' if parameter['type']['kind'] in ('string','array','struct') else 'value'
                parameters.append(parameter['name']+': '+ownership)
            manifest+=(f'    - declaration: src/io/binaryfile.sn::{name}\n'
                f'      function: {name.replace("sn_binary_file_","sn_sdk_binary_file_")}\n      symbol: sdk_binary_file_helper_{name}\n'
                '      build: native\n      convention: C\n      failure: abort\n'
                '      ownership: {parameters: {'+', '.join(parameters)+'}, result: '+
                ('owned' if result in ('string','array','struct') else 'value')+'}\n')
        self.write('.sn/sindarin-pkg-sdk/sn.yaml',manifest)
        shutil.copyfile(sdk/'tests/io/test_binaryfile.sn',self.root/'main.sn')
        expected=(sdk/'tests/io/test_binaryfile.expected').read_bytes().replace(b'\r\n',b'\n')
        modes=[(optimization,)+((arithmetic,) if arithmetic else ())
               for optimization in ('-O0','-O1','-O2') for arithmetic in (None,'--checked','--unchecked')]
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='source'):self.execute(target,expected,flags)
        built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'--target','rust',
            '-o',str(self.root/'artifacts')],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        assembly=Path(json.loads(built.stdout)['assembly'])
        metadata=json.loads(assembly.read_text())
        self.assertEqual({u['language'] for u in metadata['units']},{'C','SN'})
        shutil.copytree(assembly.parent,package/'dist')
        (package/'sn.yaml').write_text(manifest.replace('  abi: 1.6\n',
            '  abi: 1.6\n  assembly:\n    path: dist/assembly.json\n    sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'\n'))
        (package/'src/io/binaryfile.native.c').unlink(); (package/'src/io/binaryfile.sn.c').unlink()
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='prebuilt'):self.execute(target,expected,flags)
        self.assertEqual((package/'src/io/binaryfile.sn').read_bytes(),original)
        self.write('main.sn','import "sindarin-pkg-sdk/src/io/binaryfile"\n'
            'fn readShared(file: BinaryFile, bytes: byte[], alias: byte[]): int =>\n'
            '  var count: int = file.readInto(bytes)\n'
            '  assert(alias.toHex() == "0080ff2a07", "borrowed alias must see file bytes")\n'
            '  return count\nfn main(): void =>\n'
            '  var written: byte[] = {0, 128, 255, 42}\n  BinaryFile.writeAll("alias.bin", written)\n'
            '  var file: BinaryFile = BinaryFile.open("alias.bin")\n  var alias: BinaryFile = file\n'
            '  println(file._fp != nil)\n'
            '  var data: byte[] = {7, 7, 7, 7, 7}\n  var shared: byte[] = data\n'
            '  println(readShared(alias, data, data))\n  println(data.toHex())\n  println(shared.toHex())\n'
            '  file.rewind()\n  var held: byte[] = file.readRemaining()\n'
            '  var path: str = file.path()\n  var name: str = alias.name()\n'
            '  alias.dispose()\n  file.dispose()\n  println(file._is_open)\n'
            '  println(file._fp == nil)\n'
            '  println(held.toHex())\n  println(path)\n  println(name)\n'
            '  println(file._path)\n  println(sizeof(BinaryFile))\n'
            '  var direct: byte[] = sn_binary_file_read_all_static("alias.bin")\n'
            '  println(direct.toHex())\n'
            '  var helper: BinaryFile = sn_binary_file_open("alias.bin")\n'
            '  println(sn_binary_file_get_path(helper))\n  sn_binary_file_dispose(helper)\n'
            '  BinaryFile.delete("alias.bin")\n')
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,lifecycle=True):
                    # sizeof an as-ref language value is its pointer width;
                    # native record storage layout is checked separately.
                    self.execute(target,b'true\n4\n0080ff2a07\n0707070707\n0\ntrue\n0080ff2a\nalias.bin\nalias.bin\nalias.bin\n8\n0080ff2a\nalias.bin\n',flags)
        assemblies=[]
        for name,language,source in (
            ('binaryrs','RS','pub fn generate()->Option<Vec<u8>> { Some(vec![0,128,255]) }\n'),
            ('binarygo','GO','package backing\nfunc Generate() []byte { return []byte{42,7} }\n'),
            ('binarybody','SN','fn append(bytes: byte[], suffix: byte[]): byte[] =>\n'
                '  for b in suffix =>\n    bytes.push(b)\n  return bytes\n')):
            suffix={'RS':'rs','GO':'go','SN':'sn'}[language]
            backing=f'src/body.{suffix}'
            self.write(f'.sn/{name}/'+backing,source)
            declaration=('native fn append(bytes: byte[], suffix: byte[]): byte[]\n' if language=='SN'
                         else 'native fn generate(): byte[]\n')
            self.write(f'.sn/{name}/src/api.sn',declaration)
            text=(f'name: {name}\nruntime: '+('RS' if language=='SN' else language)+'\nnative:\n'
                '  abi: 1.6\n  declarations: [src/api.sn]\n  builds:\n'
                f'    - name: backing\n      language: {language}\n      sources: [{backing}]\n')
            if language=='GO':
                text+='      module: src\n'
                self.write(f'.sn/{name}/src/go.mod','module sindarin.test/'+name+'\n\ngo 1.26.0\n')
            else:text+='      entry: '+backing+'\n'
            function='append' if language=='SN' else 'generate'
            text+=(f'  bindings:\n    - declaration: src/api.sn::{function}\n'
                '      function: '+('Generate' if language=='GO' else function)+'\n'
                f'      symbol: {name}_{function}\n      build: backing\n      convention: C\n      failure: abort\n'
                '      ownership: {parameters: {'+('bytes: borrowed, suffix: borrowed' if language=='SN' else '')+'}, result: owned}\n')
            self.write(f'.sn/{name}/sn.yaml',text)
            assemblies.append((name,text,backing))
        self.write('main.sn','import "sindarin-pkg-sdk/src/io/binaryfile"\n'
            'import "binaryrs/src/api" as RS\nimport "binarygo/src/api" as Go\n'
            'import "binarybody/src/api" as Body\n'
            'fn load(): byte[] =>\n  var file: BinaryFile = BinaryFile.open("mixed.bin")\n'
            '  var alias: BinaryFile = file\n  var bytes: byte[] = alias.readRemaining()\n'
            '  file.dispose()\n  return bytes\nfn main(): void =>\n'
            '  var bytes: byte[] = RS.generate()\n  var tail: byte[] = Go.generate()\n'
            '  var result: byte[] = Body.append(bytes, tail)\n  println(bytes.toHex())\n'
            '  BinaryFile.writeAll("mixed.bin", result)\n  var held: byte[] = load()\n'
            '  println(held.toHex())\n  println(tail.toHex())\n  BinaryFile.delete("mixed.bin")\n')
        for archive in ('source','prebuilt'):
            if archive=='prebuilt':
                for name,text,backing in assemblies:
                    dependency=self.root/'.sn'/name
                    built=subprocess.run([str(COMPILER),'--build-native',str(dependency/'sn.yaml'),'--target','rust',
                        '-o',str(dependency/'dist-build')],cwd=self.root,capture_output=True,timeout=180)
                    self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                    assembly=Path(json.loads(built.stdout)['assembly']);shutil.copytree(assembly.parent,dependency/'dist')
                    (dependency/'sn.yaml').write_text(text.replace('  abi: 1.6\n',
                        '  abi: 1.6\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                    (dependency/backing).unlink()
            for flags in modes:
                for target in ('c','rust'):
                    with self.subTest(target=target,flags=flags,mixed=archive):
                        self.execute(target,b'0080ff2a07\n0080ff2a07\n2a07\n',flags)

    def test_unchanged_sdk_textfile_facade_is_an_independent_c_library(self):
        import hashlib,json
        sdk=Path(os.environ.get('SN_SDK_ROOT',ROOT/'.sn/sdk-native-integration')).resolve()
        self.assertTrue((sdk/'src/io/textfile.sn').is_file(),'pinned SDK integration checkout is required')
        package=self.root/'.sn/sindarin-pkg-sdk'
        shutil.copytree(sdk/'src/io',package/'src/io')
        original=(package/'src/io/textfile.sn').read_bytes()
        self.write('.sn/sindarin-pkg-sdk/sn.yaml','name: sindarin-pkg-sdk\n')
        model_path=self.root/'sdk-model.json'
        emitted=subprocess.run([str(COMPILER),str(package/'src/io/textfile.sn'),'--no-install',
            '--package-body','--emit-model','-o',str(model_path)],cwd=self.root,capture_output=True,timeout=30)
        self.assertEqual(emitted.returncode,0,emitted.stderr.decode(errors='replace'))
        model=json.loads(model_path.read_text())
        textfile=next(s for s in model['structs'] if s['name']=='TextFile')
        manifest=('name: sindarin-pkg-sdk\nruntime: C\nnative:\n  abi: 1.5\n'
            '  declarations: [src/io/textfile.sn]\n  types:\n'
            '    - declaration: src/io/textfile.sn::TextFile\n'
            '      identity: SindarinSDK/sindarin-pkg-sdk:io.TextFile@1\n'
            '      c_type: SnSdkTextFileRecord\n      header: src/io/textfile.native.h\n'
            '      create: sn_sdk_text_file_create\n      retain: sn_sdk_text_file_retain\n'
            '      release: sn_sdk_text_file_release\n      refs: sn_sdk_text_file_refs\n      owner: atomic\n'
            '  builds:\n    - name: native\n      language: C\n'
            '      sources: [src/io/textfile.native.c]\n      provides_sources: [src/io/textfile.sn.c]\n'
            '      include_dirs: [src/io]\n    - name: facade\n      language: SN\n'
            '      entry: src/io/textfile.sn\n      sources: [src/io/textfile.sn]\n      include_dirs: [src/io]\n'
            '  bindings:\n')
        for method in textfile['methods']:
            name=method['name'];result=method['return_type']['kind']
            parameters=[] if method['is_static'] else ['self: borrowed']
            for parameter in method['params']:
                ownership='borrowed' if parameter['type']['kind'] in ('string','array','struct') else 'value'
                parameters.append(parameter['name']+': '+ownership)
            manifest+=(f'    - declaration: src/io/textfile.sn::TextFile.{name}\n'
                f'      function: TextFile.{name}\n      symbol: sdk_text_file_method_{name}\n'
                '      build: facade\n      convention: C\n      failure: abort\n'
                '      ownership: {parameters: {'+', '.join(parameters)+'}, result: '+
                ('owned' if result in ('string','array','struct') else 'value')+'}\n')
        self.write('.sn/sindarin-pkg-sdk/sn.yaml',manifest)
        shutil.copyfile(sdk/'tests/io/test_textfile.sn',self.root/'main.sn')
        expected=(sdk/'tests/io/test_textfile.expected').read_bytes().replace(b'\r\n',b'\n')
        modes=[(optimization,)+((arithmetic,) if arithmetic else ())
               for optimization in ('-O0','-O1','-O2') for arithmetic in (None,'--checked','--unchecked')]
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='source'):self.execute(target,expected,flags)
        built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'--target','rust',
            '-o',str(self.root/'artifacts')],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        assembly=Path(json.loads(built.stdout)['assembly'])
        metadata=json.loads(assembly.read_text())
        self.assertEqual({u['language'] for u in metadata['units']},{'C','SN'})
        shutil.copytree(assembly.parent,package/'.sn/sealed')
        (package/'sn.yaml').write_text(manifest.replace('  abi: 1.5\n',
            '  abi: 1.5\n  assembly:\n    path: .sn/sealed/assembly.json\n    sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'\n'))
        for file in (package/'src/io').glob('*.c'):file.unlink()
        # The original public module still contains its unchanged method bodies
        # and @source directive; the sealed facade consumes neither as app code.
        for flags in modes:
            for target in ('c','rust'):
                with self.subTest(target=target,flags=flags,archive='prebuilt'):self.execute(target,expected,flags)
        self.assertEqual((package/'src/io/textfile.sn').read_bytes(),original)
        self.package('rsdep','RS','pub fn Provide()->i64 { 22 }\n',function='Provide')
        self.package('godep','GO','package backing\nfunc Provide() int64 { return 33 }\n',function='Provide')
        self.write('.sn/http/src/response.sn','fn requestLine(): str =>\n  return "GET / HTTP/1.1"\n'
            'fn append(values: str[]): str[] =>\n  values.push("HTTP result")\n  return values\n')
        self.write('.sn/http/sn.yaml','name: http\nruntime: RS\nnative:\n  abi: 1.5\n'
            '  declarations: [src/response.sn]\n  builds:\n    - name: response\n      language: SN\n'
            '      entry: src/response.sn\n      sources: [src/response.sn]\n  bindings:\n'
            '    - declaration: src/response.sn::requestLine\n      function: requestLine\n      symbol: http_request_line\n'
            '      build: response\n      convention: C\n      failure: abort\n      ownership: {parameters: {}, result: owned}\n'
            '    - declaration: src/response.sn::append\n      function: append\n      symbol: http_append\n'
            '      build: response\n      convention: C\n      failure: abort\n      ownership: {parameters: {values: borrowed}, result: owned}\n')
        self.write('main.sn','import "sindarin-pkg-sdk/src/io/textfile"\nimport "http/src/response" as HTTP\n'
            'import "rsdep/src/api" as RS\nimport "godep/src/api" as Go\nfn main(): void =>\n'
            '  TextFile.writeAll("mixed.txt", HTTP.requestLine() + "\\nsecond")\n'
            '  var file: TextFile = TextFile.open("mixed.txt")\n  var alias: TextFile = file\n'
            '  var lines: str[] = file.readLines()\n  var result: str[] = HTTP.append(lines)\n'
            '  alias.dispose()\n  println(lines[0])\n  println(lines.length)\n  println(result[2])\n'
            '  println(file._is_open)\n  println(RS.provide() + Go.provide())\n  TextFile.delete("mixed.txt")\n')
        self.execute('rust',b'GET / HTTP/1.1\n3\nHTTP result\n0\n55\n')

    def test_unchanged_public_static_method_bodies_compile_independently(self):
        self.write('.sn/methods/src/api.sn','native struct Helpers as ref =>\n'
                   '  static fn calculate(value: int): int =>\n    return value * 2 + 1\n'
                   '  static fn echo(text: str): str =>\n    return text\n'
                   '  static fn labels(): str[] =>\n    return {"first", "second"}\n')
        source=(self.root/'.sn/methods/src/api.sn').read_bytes()
        for runtime in ('C','RS'):
            manifest=('name: methods\nruntime: '+runtime+'\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
                '  builds:\n    - name: facade\n      language: SN\n      entry: src/api.sn\n      sources: [src/api.sn]\n  bindings:\n')
            for name,parameters,result in (('calculate','value: value','value'),('echo','text: borrowed','owned'),('labels','','owned')):
                manifest+=(f'    - declaration: src/api.sn::Helpers.{name}\n      function: Helpers.{name}\n'
                    f'      symbol: native_method_{name}\n      build: facade\n      convention: C\n      failure: abort\n'
                    f'      ownership: {{parameters: {{{parameters}}}, result: {result}}}\n')
            self.write('.sn/methods/sn.yaml',manifest)
            self.write('main.sn','import "methods/src/api"\nfn main(): void =>\n'
                '  println(Helpers.calculate(20))\n  println(Helpers.echo("method body"))\n'
                '  println(Helpers.echo(nil)==nil)\n  var labels: str[] = Helpers.labels()\n'
                '  println(labels[0])\n  println(labels[1])\n')
            for target in ('c','rust'):
                with self.subTest(runtime=runtime,target=target):
                    self.execute(target,b'41\nmethod body\ntrue\nfirst\nsecond\n')
            self.assertEqual((self.root/'.sn/methods/src/api.sn').read_bytes(),source)

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
                ('C','fn main(): void =>\n  println("not a library")\nfn provide(): int =>\n  return 11\n',b'cannot contain an application main')):
            with self.subTest(runtime=runtime,source=source):
                self.write('.sn/body/sn.yaml',template.format(runtime=runtime))
                self.write('.sn/body/src/body.sn',source)
                result=subprocess.run([str(COMPILER),'main.sn','--target','rust','--no-install','-o','rejected.exe'],cwd=self.root,capture_output=True,timeout=90)
                self.assertNotEqual(result.returncode,0)
                self.assertIn(message,result.stderr)
                self.assertFalse((self.root/'rejected.exe').exists())

    def test_c_sindarin_mutable_array_body_preserves_live_aliases_and_prebuilt_consumption(self):
        self.check_sindarin_mutable_array_body("C")

    def test_rust_sindarin_mutable_array_body_preserves_live_aliases_and_prebuilt_consumption(self):
        self.check_sindarin_mutable_array_body("RS")

    def check_sindarin_mutable_array_body(self, runtime):
        import hashlib, json
        package = self.root/'.sn/livebody'
        self.write('.sn/livebody/src/api.sn', 'native fn mutate(values: str[], alias: str[]): str[]\n'
                   'native fn count(values: str[]): int\n')
        self.write('.sn/livebody/src/observer.h', '#include "sn_array.h"\n'
                   'static inline void observe_package_array(SnArray *a) {\n'
                   ' if (!a || a->len != 68 || strcmp(((char **)a->data)[0], "changed")) abort();\n'
                   ' char *text = strdup("callback"); sn_array_push(a, &text);\n}\n')
        self.write('.sn/livebody/src/body.sn', '@include "observer.h"\n'
                   '@alias "observe_package_array"\nnative fn observe(values: str[]): void\n'
                   'fn mutate(values: str[], alias: str[]): str[] =>\n'
                   '  if values == nil =>\n    return nil\n'
                   '  if values.length == 0 =>\n    values.push("fresh")\n    return alias\n'
                   '  values[0] = "changed"\n  for i in 0..64 =>\n    values.push("grow")\n'
                   '  observe(alias)\n  return alias\n'
                   'fn count(values: str[]): int =>\n  if values == nil =>\n    return 0\n  return values.length\n')
        manifest = (f'name: livebody\nruntime: {runtime}\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
                    '  builds:\n    - name: body\n      language: SN\n      entry: src/body.sn\n'
                    '      sources: [src/body.sn]\n      include_dirs: [src]\n  bindings:\n'
                    '    - declaration: src/api.sn::mutate\n      function: mutate\n      symbol: live_body_mutate\n'
                    '      build: body\n      convention: C\n      failure: status\n'
                    '      ownership: {parameters: {values: borrowed, alias: borrowed}, result: owned}\n'
                    '    - declaration: src/api.sn::count\n      function: count\n      symbol: live_body_count\n'
                    '      build: body\n      convention: C\n      failure: abort\n'
                    '      ownership: {parameters: {values: borrowed}, result: value}\n')
        self.write('.sn/livebody/sn.yaml', manifest)
        self.write('main.sn', 'import "livebody/src/api"\nfn main(): void =>\n'
                   '  println(mutate(nil, nil) == nil)\n  println(count(nil))\n'
                   '  var empty: str[] = {}\n  var fresh = mutate(empty, empty)\n'
                   '  println(empty.length)\n  println(fresh[0])\n'
                   '  var missing: str = nil\n  var values: str[] = {"one", missing, "", "\\x80\\xff"}\n'
                   '  var result = mutate(values, values)\n  println(count(values))\n  println(result.length)\n'
                   '  values[0] = "after"\n  println(result[0])\n  println(result[1] == nil)\n'
                   '  println(result[2] == nil)\n  println(result[3] == "\\x80\\xff")\n  println(result[68])\n')
        wanted = b'true\n0\n1\nfresh\n69\n69\nchanged\ntrue\nfalse\ntrue\ncallback\n'
        for target in ('c', 'rust'):
            for optimization in ('-O0', '-O1', '-O2'):
                for arithmetic in (None, '--checked', '--unchecked'):
                    flags = (optimization,) + ((arithmetic,) if arithmetic else ())
                    with self.subTest(target=target, flags=flags): self.execute(target, wanted, flags)
        built = subprocess.run([str(COMPILER), '--build-package', str(package/'sn.yaml'), '--target', 'rust',
                                '-o', str(package/'.sn/published')], capture_output=True, timeout=180)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        descriptor = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(descriptor.read_text())
        self.assertEqual(metadata['abi'], '1.5')
        self.assertEqual(metadata['units'][0]['implementation_runtime'], runtime)
        destination = package/'dist'
        shutil.copytree(descriptor.parent, destination)
        digest = hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
        (package/'sn.yaml').write_text(manifest.replace('native:\n',
            'native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
        (package/'src/body.sn').unlink()
        (package/'src/observer.h').unlink()
        shutil.rmtree(package/'.sn')
        for target in ('c', 'rust'):
            for optimization in ('-O0', '-O1', '-O2'):
                for arithmetic in (None, '--checked', '--unchecked'):
                    flags = (optimization,) + ((arithmetic,) if arithmetic else ())
                    with self.subTest(prebuilt=target, flags=flags): self.execute(target, wanted, flags)

    def test_two_rust_array_body_libraries_namespace_helpers_and_compile_native_sources(self):
        import json
        for name, seed in (('firstbody',1),('secondbody',10)):
            root='.sn/'+name
            self.write(root+'/src/api.sn','native fn mutate(values: str[]): str[]\nnative fn label(): str\n')
            self.write(root+'/src/body.sn', '@source "value.c"\n@alias "'+name+'_seed"\nnative fn seed(): int\n'
                       'var count: int = seed()\nfn label(): str =>\n  return "'+name+':__sn_native_handle_array_set_0"\n'
                       'fn mutate(values: str[]): str[] =>\n  count += 1\n  println(count)\n'
                       '  if values != nil =>\n    if values.length > 0 =>\n      values[0] = label()\n  return values\n')
            self.write(root+'/src/value.h',f'#define SEED_VALUE {seed}\n')
            self.write(root+'/src/value.c','#include "value.h"\nlong long '+name+'_seed(void) { return SEED_VALUE; }\n')
            text=f'name: {name}\nruntime: RS\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
            text+='  builds:\n    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n  bindings:\n'
            for function,params,result in (('mutate','{values: borrowed}','owned'),('label','{}','owned')):
                text+=f'    - declaration: src/api.sn::{function}\n      function: {function}\n      symbol: {name}_{function}\n'
                text+='      build: body\n      convention: C\n      failure: status\n'
                text+=f'      ownership: {{parameters: {params}, result: {result}}}\n'
            self.write(root+'/sn.yaml',text)
        self.write('main.sn','import "firstbody/src/api" as First\nimport "secondbody/src/api" as Second\n'
                   'fn main(): void =>\n  var values: str[] = {"initial"}\n  var first = First.mutate(values)\n'
                   '  println(values[0])\n  var second = Second.mutate(values)\n  println(values[0])\n'
                   '  println(first[0])\n  println(second[0])\n  var third = First.mutate(values)\n  println(First.label())\n')
        wanted=(b'2\nfirstbody:__sn_native_handle_array_set_0\n11\nsecondbody:__sn_native_handle_array_set_0\n'
                b'firstbody:__sn_native_handle_array_set_0\nsecondbody:__sn_native_handle_array_set_0\n3\nfirstbody:__sn_native_handle_array_set_0\n')
        for target in ('c','rust'):
            with self.subTest(target=target):self.execute(target,wanted)
        symbols=[]
        for name in ('firstbody','secondbody'):
            package=self.root/'.sn'/name
            built=subprocess.run([str(COMPILER),'--build-package',str(package/'sn.yaml'),'--target','rust',
                '-o',str(package/'.sn/check')],capture_output=True,timeout=180)
            self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
            descriptor=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(descriptor.read_text())
            archive=descriptor.parent/metadata['units'][0]['archive']
            data=archive.read_bytes()
            # Canonical array helpers are generated per package. Inspect actual
            # archive symbol bytes, alongside the two-library behavioural check.
            import re
            names=set(re.findall(rb'sn_native_000_[0-9a-f]{12}___sn_native_handle_array_[a-z_]+_0',data))
            self.assertTrue(names)
            symbols.append(names)
        self.assertFalse(symbols[0]&symbols[1])

    def test_generated_rust_and_go_native_views_mutate_c_storage_with_live_callbacks(self):
        import hashlib, json
        from unittest.mock import patch
        for language in ('RS', 'GO'):
            name = 'live' + language.lower()
            root = '.sn/' + name
            prefix = name + '_array'
            self.write(root+'/src/api.sn', 'native fn mutate(values: str[], alias: str[]): str[]\n')
            self.write(root+'/src/array_ops.h', '#include "sn_array.h"\n'
                       f'long long {prefix}_length(SnArray *a);\n'
                       f'const char *{prefix}_get(SnArray *a, long long index);\n'
                       f'void {prefix}_push(SnArray *a, const char *text);\n'
                       f'void {prefix}_set(SnArray *a, long long index, const char *text);\n')
            self.write(root+'/src/array_ops.c', '#include "array_ops.h"\n'
                       f'long long {prefix}_length(SnArray *a) {{ return a ? a->len : 0; }}\n'
                       f'const char *{prefix}_get(SnArray *a, long long index) {{ return ((char **)a->data)[index]; }}\n'
                       f'void {prefix}_push(SnArray *a, const char *text) {{ char *copy = text ? strdup(text) : NULL; sn_array_push(a, &copy); }}\n'
                       f'void {prefix}_set(SnArray *a, long long index, const char *text) {{ char *copy = text ? strdup(text) : NULL; free(((char **)a->data)[index]); ((char **)a->data)[index] = copy; }}\n')
            if language == 'RS':
                self.write(root+'/src/impl.rs', 'use std::ffi::{c_void, c_char, CStr};\nextern "C" {\n'
                           f' fn {prefix}_length(a: *mut c_void) -> i64;\n'
                           f' fn {prefix}_get(a: *mut c_void, index: i64) -> *const c_char;\n'
                           f' fn {prefix}_push(a: *mut c_void, text: *const c_char);\n'
                           f' fn {prefix}_set(a: *mut c_void, index: i64, text: *const c_char);\n'+'}\n'
                           'pub fn mutate(a: *mut c_void, b: *mut c_void) -> Result<Option<Vec<Option<Vec<u8>>>>,u32> { unsafe {\n'
                           ' if a != b { return Err(1); } if a.is_null() { return Ok(None); }\n'
                           f' if {prefix}_length(a) == 0 {{ {prefix}_push(a, b"fresh\\0".as_ptr().cast()); }} else {{\n'
                           f'  {prefix}_set(a, 0, b"native\\0".as_ptr().cast()); for _ in 0..64 {{ {prefix}_push(a, b"grow\\0".as_ptr().cast()); }}\n'
                           f'  let callback = || {{ assert_eq!({prefix}_length(b), 68); {prefix}_push(b, b"callback\\0".as_ptr().cast()); }}; callback();\n'+' }\n'
                           f' let values = (0..{prefix}_length(b)).map(|i| {{ let text={prefix}_get(b,i); if text.is_null() {{ None }} else {{ Some(CStr::from_ptr(text).to_bytes().to_vec()) }} }}).collect();\n'
                           ' Ok(Some(values)) } }\n')
                build = '      entry: src/impl.rs\n      sources: [src/impl.rs]\n'
                function = 'mutate'
            else:
                self.write(root+'/src/go.mod', 'module sindarin.test/'+name+'\n\ngo 1.26.0\n')
                self.write(root+'/src/impl.go', 'package backing\n/* #include <stdlib.h>\n#include "array_ops.h" */\n'
                           'import "C"\nimport ("unsafe"; "runtime")\n'
                           f'func push(a unsafe.Pointer, text string) {{ p:=C.CString(text); C.{prefix}_push((*C.SnArray)(a), p); C.free(unsafe.Pointer(p)) }}\n'
                           'func Mutate(a,b unsafe.Pointer) ([]*string,uint32) {\n'
                           ' if a != b { return nil,1 }; if a == nil { return nil,0 };\n'
                           f' if C.{prefix}_length((*C.SnArray)(a)) == 0 {{ push(a,"fresh") }} else {{\n'
                           f'  text:=C.CString("native"); C.{prefix}_set((*C.SnArray)(a),0,text); C.free(unsafe.Pointer(text))\n'
                           '  for i:=0; i<64; i++ { push(a,"grow") }; runtime.GC()\n'
                           f'  callback:=func() {{ if C.{prefix}_length((*C.SnArray)(b)) != 68 {{ panic("stale view") }}; push(b,"callback") }}; callback()\n'+' }\n'
                           f' values:=make([]*string,int(C.{prefix}_length((*C.SnArray)(b)))); for i:=range values {{ p:=C.{prefix}_get((*C.SnArray)(b),C.longlong(i)); if p!=nil {{ text:=C.GoString(p); values[i]=&text }} }}\n'
                           ' runtime.GC(); return values,0 }\n')
                build = '      module: src\n      sources: [src/impl.go]\n      include_dirs: [src]\n'
                function = 'Mutate'
            text = (f'name: {name}\nruntime: {language}\nnative:\n  abi: 1.5\n  declarations: [src/api.sn]\n'
                    f'  builds:\n    - name: backing\n      language: {language}\n' + build +
                    '    - name: helpers\n      language: C\n      sources: [src/array_ops.c]\n      include_dirs: [src]\n'
                    f'  bindings:\n    - declaration: src/api.sn::mutate\n      function: {function}\n'
                    f'      symbol: {name}_mutate\n      build: backing\n      convention: C\n      failure: status\n'
                    '      ownership: {parameters: {values: borrowed, alias: borrowed}, result: owned}\n')
            self.write(root+'/sn.yaml', text)
            self.write('main.sn', f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  println(mutate(nil,nil) == nil)\n  var empty: str[] = {}\n  var fresh = mutate(empty,empty)\n'
                       '  println(empty.length)\n  println(fresh[0])\n'
                       '  var missing: str = nil\n  var values: str[] = {"one", missing, "", "\\x80\\xff"}\n'
                       '  var result = mutate(values,values)\n  println(values.length)\n  println(result.length)\n'
                       '  values[0] = "after"\n  println(result[0])\n  println(result[1] == nil)\n'
                       '  println(result[2] == nil)\n  println(result[3] == "\\x80\\xff")\n  println(result[68])\n')
            wanted = b'true\n1\nfresh\n69\n69\nnative\ntrue\nfalse\ntrue\ncallback\n'
            with patch.dict(os.environ, {'GOEXPERIMENT': 'cgocheck2'}):
                for target in ('c','rust'):
                    with self.subTest(language=language, target=target): self.execute(target,wanted)
                package = self.root/root
                built = subprocess.run([str(COMPILER), '--build-package', str(package/'sn.yaml'), '--target', 'rust',
                    '-o', str(package/'.sn/published')], capture_output=True, timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                descriptor = Path(json.loads(built.stdout)['assembly'])
                metadata = json.loads(descriptor.read_text())
                self.assertEqual(metadata['units'][0]['language'],language)
                self.assertEqual(metadata['units'][0]['generated_provider_exports'],[name+'_mutate'])
                destination=package/'dist';shutil.copytree(descriptor.parent,destination)
                digest=hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
                (package/'sn.yaml').write_text(text.replace('native:\n',
                    'native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
                for file in ('impl.rs','impl.go','array_ops.c','array_ops.h','go.mod'):
                    (package/'src'/file).unlink(missing_ok=True)
                shutil.rmtree(package/'.sn')
                for target in ('c','rust'):
                    with self.subTest(prebuilt=language,target=target): self.execute(target,wanted)

    def test_mutable_array_body_and_provider_contracts_do_not_fall_back_to_readonly_copies(self):
        self.package('livecontract', 'C', 'long long count(void) { return 0; }\n', function='count')
        self.write('.sn/livecontract/src/api.sn', 'native fn provide(values: str[]): int\n')
        path = self.root/'.sn/livecontract/sn.yaml'
        original = path.read_text().replace('parameters: {}', 'parameters: {values: borrowed}')
        self.write('.sn/livecontract/src/body.sn', 'fn count(values: str[]): int =>\n  return values.length\n')
        body = original.replace('language: C\n', 'language: SN\n').replace('sources: [src/impl.c]',
            'entry: src/body.sn\n      sources: [src/body.sn]')
        self.write('.sn/livecontract/src/impl.rs', 'pub fn count(_: Option<&[Option<&[u8]>]>) -> i64 { 0 }\n')
        self.write('main.sn', 'import "livecontract/src/api"\nfn main(): void =>\n  println(provide(nil))\n')
        for text, message in (
                (body.replace('abi: 1.0', 'abi: 1.1'), b'array inputs require generated mutable/borrowed-body contracts'),
                (original.replace('runtime: C', 'runtime: RS').replace('language: C', 'language: RS')
                 .replace('sources: [src/impl.c]', 'entry: src/impl.rs\n      sources: [src/impl.rs]')
                 .replace('abi: 1.0', 'abi: 1.5'), b'mismatched types')):
            with self.subTest(message=message):
                path.write_text(text)
                result = subprocess.run([str(COMPILER), 'main.sn', '--target', 'rust', '--no-install',
                                         '-o', 'rejected.exe'], cwd=self.root, capture_output=True, timeout=90)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)
                self.assertFalse((self.root/'rejected.exe').exists())

    def test_sindarin_library_globals_initialize_once_and_owned_returns_survive(self):
        import hashlib,json
        for runtime in ('C','RS'):
            name='globals'+runtime.lower()
            self.write(f'.sn/{name}/src/api.sn','native fn next(): int\nnative fn read(): str\n')
            self.write(f'.sn/{name}/src/body.sn','var count: int = 0\nvar label: str = initialize()\nvar later: int = 42\n'
                       'fn initialize(): str =>\n  count += 1\n  if later != 42 =>\n    return "wrong initialization order"\n  return "initialized"\n'
                       'fn next(): int =>\n  count += 1\n  return count\n'
                       'fn read(): str =>\n  return label\n')
            text=f'name: {name}\nruntime: {runtime}\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n'
            text+='    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n  bindings:\n'
            for function,result in (('next','value'),('read','owned')):
                text+=f'    - declaration: src/api.sn::{function}\n      function: {function}\n      symbol: {name}_{function}\n      build: body\n'
                text+=f'      convention: C\n      failure: status\n      ownership: {{parameters: {{}}, result: {result}}}\n'
            self.write(f'.sn/{name}/sn.yaml',text)
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  println(next())\n  var result: str = read()\n  println(next())\n  println(result)\n')
            for target in ('c','rust'):
                with self.subTest(runtime=runtime,target=target):self.execute(target,b'2\n3\ninitialized\n')
            package=self.root/'.sn'/name;manifest=package/'sn.yaml'
            built=subprocess.run([str(COMPILER),'--build-package',str(manifest),'--target','rust','-o',str(package/'.sn/published')],capture_output=True,timeout=180)
            self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
            descriptor=Path(json.loads(built.stdout)['assembly']);destination=package/'dist';shutil.copytree(descriptor.parent,destination)
            digest=hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
            manifest.write_text(text.replace('native:\n','native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
            (package/'src/body.sn').unlink();shutil.rmtree(package/'.sn')
            for target in ('c','rust'):
                with self.subTest(prebuilt=runtime,target=target):self.execute(target,b'2\n3\ninitialized\n')

    def test_sindarin_library_owned_string_arrays_preserve_nil_and_mutation(self):
        import hashlib,json
        for runtime in ('C','RS'):
            name='arraybody'+runtime.lower()
            self.write(f'.sn/{name}/src/api.sn','native fn items(mode: int): str[]\n')
            self.write(f'.sn/{name}/src/body.sn','fn items(mode: int): str[] =>\n'
                       '  if mode == 0 =>\n    return nil\n  if mode == 1 =>\n    return {}\n'
                       '  var missing: str = nil\n  var result: str[] = {"one", missing, ""}\n  return result\n')
            self.write(f'.sn/{name}/sn.yaml',f'name: {name}\nruntime: {runtime}\nnative:\n  abi: 1.1\n  declarations: [src/api.sn]\n  builds:\n'
                       '    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n  bindings:\n'
                       f'    - declaration: src/api.sn::items\n      function: items\n      symbol: {name}_items\n      build: body\n'
                       '      convention: C\n      failure: status\n      ownership: {parameters: {mode: value}, result: owned}\n')
            self.write('main.sn',f'import "{name}/src/api"\nfn main(): void =>\n'
                       '  println(items(0) == nil)\n  println(items(1) == nil)\n  var result: str[] = items(2)\n'
                       '  println(result.length)\n  println(result[0])\n  println(result[1] == nil)\n'
                       '  println(result[2] == nil)\n  result[0] = "changed"\n  println(result[0])\n')
            for target in ('c','rust'):
                with self.subTest(runtime=runtime,target=target):self.execute(target,b'true\nfalse\n3\none\ntrue\nfalse\nchanged\n')
            package=self.root/'.sn'/name;manifest=package/'sn.yaml'
            text=manifest.read_text()
            built=subprocess.run([str(COMPILER),'--build-package',str(manifest),'--target','rust','-o',str(package/'.sn/published')],capture_output=True,timeout=180)
            self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
            descriptor=Path(json.loads(built.stdout)['assembly']);destination=package/'dist'
            shutil.copytree(descriptor.parent,destination)
            digest=hashlib.sha256((destination/'assembly.json').read_bytes()).hexdigest()
            manifest.write_text(text.replace('native:\n','native:\n  assembly: {path: dist/assembly.json, sha256: '+digest+'}\n'))
            (package/'src/body.sn').unlink();shutil.rmtree(package/'.sn')
            for target in ('c','rust'):
                with self.subTest(prebuilt=runtime,target=target):self.execute(target,b'true\nfalse\n3\none\ntrue\nfalse\nchanged\n')

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

    def execute(self,target,expected,flags=()):
        executable=self.root/'main.exe'
        built=subprocess.run([str(COMPILER),'main.sn','--target',target,'--no-install',*flags,'-o',str(executable)],cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        run=subprocess.run([str(executable)],cwd=self.root,capture_output=True,timeout=15)
        self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))
        self.assertEqual(run.stderr,b'')
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

    def test_rust_links_large_archive_graph_from_paths_with_spaces(self):
        import hashlib,json
        self.root=self.root/'working tree with spaces';self.root.mkdir()
        package=self.root/'.sn/largegraph';count=18
        self.write('.sn/largegraph/src/api.sn',''.join(f'native fn part{i}(): int\n' for i in range(count)))
        manifest='name: largegraph\nruntime: C\nnative:\n  abi: 1.0\n  declarations: [src/api.sn]\n  builds:\n'
        for i in range(count):
            self.write(f'.sn/largegraph/src/unit{i}.c',f'long long provide(void) {{ return {i}; }}\n')
            manifest+=f'    - name: unit{i}\n      language: C\n      sources: [src/unit{i}.c]\n'
        manifest+='  bindings:\n'
        for i in range(count):
            manifest+=(f'    - declaration: src/api.sn::part{i}\n      function: provide\n      symbol: largegraph_part_{i}\n'
                f'      build: unit{i}\n      convention: C\n      failure: abort\n      ownership: {{parameters: {{}}, result: value}}\n')
        self.write('.sn/largegraph/sn.yaml',manifest)
        self.write('main.sn','import "largegraph/src/api"\nfn main(): void =>\n  println('+
            ' + '.join(f'part{i}()' for i in range(count))+')\n')
        for archive in ('source','prebuilt'):
            if archive=='prebuilt':
                built=subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),'-o',str(package/'dist-build')],
                    cwd=self.root,capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(assembly.read_text())
                self.assertEqual(len(metadata['units']),count)
                shutil.copytree(assembly.parent,package/'dist')
                (package/'sn.yaml').write_text(manifest.replace('  abi: 1.0\n','  abi: 1.0\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                for source in (package/'src').glob('*.c'):source.unlink()
            for target in ('c','rust'):
                with self.subTest(target=target,archive=archive):self.execute(target,b'153\n')
            if os.name=='nt':
                proxies=list((self.root/'.sn/build/rust').glob('*/sn_rust_linker_proxy.cmd'))
                self.assertTrue(proxies)
                for proxy in proxies:
                    self.assertLess(len(proxy.read_bytes()),1024)
                    suffix=Path(str(proxy)+'.suffix.rsp').read_text()
                    self.assertEqual(suffix.count('libsn_native_'),count)

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
