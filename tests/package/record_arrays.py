#!/usr/bin/env python3
"""Generated native record-array exports preserve slots and cleanup."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import unittest
import callable_methods as fixtures

class RecordArrayTests(unittest.TestCase):
    setUp = fixtures.CallableMethodTests.setUp
    write = fixtures.CallableMethodTests.write
    binding = staticmethod(fixtures.CallableMethodTests.binding)
    def test_generated_record_array_providers(self):
        api = self.package/'src/api.sn'
        api.write_text(api.read_text()+'native fn makeArray(): Counter[]\n'
            'native fn mutateArray(a: Counter[], b: Counter[]): Counter[]\n'
            'fn arrayBody(a: Counter[]): Counter[] =>\n  a[0].amount += 1\n  return a\n')
        source=self.package/'src/counter.c'
        source.write_text(source.read_text()+'''\n#include "sn_array.h"
#include <assert.h>
static void slot_release(void *slot) { counter_release(*(CanonicalCounter **)slot); }
static void slot_copy(const void *src, void *dst) { *(CanonicalCounter **)dst=counter_retain(*(CanonicalCounter *const *)src); }
SnArray *make_array(void) {
 SnArray *a=sn_array_new(sizeof(CanonicalCounter *),2);
 a->elem_tag=SN_TAG_STRUCT; a->elem_release=slot_release; a->elem_copy=slot_copy;
 CanonicalCounter *p=counter_original_make(7); sn_array_push(a,&p);
 p=counter_retain(p); sn_array_push(a,&p); return a;
}
SnArray *mutate_array(SnArray *a, SnArray *b) {
 assert(a==b); if(!a) return NULL;
 assert(a->len==2 && ((void **)a->data)[0]==((void **)a->data)[1]);
 ((CanonicalCounter **)a->data)[0]->__sn__amount++;
 return sn_array_copy(a);
}
''')
        self.write('.sn/methods/src/backing.rs', '''use std::ffi::c_void;
extern "C" { fn make_array()->*mut c_void; fn mutate_array(a:*mut c_void,b:*mut c_void)->*mut c_void; }
pub fn make()->*mut c_void { unsafe { make_array() } }
pub fn mutate(a:*mut c_void,b:*mut c_void)->*mut c_void { unsafe { mutate_array(a,b) } }
''')
        self.write('.sn/methods/src/go/go.mod','module sindarin.test/recordarrays\n\ngo 1.26.0\n')
        self.write('.sn/methods/src/go/backing.go','''package recordarrays
/*
#include "sn_array.h"
SnArray *make_array(void);
SnArray *mutate_array(SnArray *, SnArray *);
*/
import "C"
import "unsafe"
import "runtime"
func Make() unsafe.Pointer { runtime.GC(); return unsafe.Pointer(C.make_array()) }
func Mutate(a,b unsafe.Pointer) unsafe.Pointer { runtime.GC(); return unsafe.Pointer(C.mutate_array((*C.SnArray)(a),(*C.SnArray)(b))) }
''')
        self.write('main.sn','import "methods/src/api"\nfn exercise(): void =>\n'
            '  var a: Counter[] = makeArray()\n  var b: Counter[] = mutateArray(a,a)\n'
            '  println(a.length)\n  println(a[0].amount)\n  println(same(a[0],b[1]))\n'
            '  b[1].amount=12\n  println(a[1].amount)\n'
            '  var body: Counter[] = arrayBody(a)\n  println(body[0].amount)\n'
            '  println(mutateArray(nil,nil)==nil)\nfn main(): void =>\n  exercise()\n  println(destroyed())\n')
        expected=b'2\n8\ntrue\n12\n13\ntrue\n1\n'
        if os.name=='nt': expected=expected.replace(b'\n',b'\r\n')
        for language,make,mutate,build in [('C','make_array','mutate_array',''),
                ('RS','make','mutate','    - name: arrays\n      language: RS\n      entry: src/backing.rs\n      sources: [src/backing.rs]\n'),
                ('GO','Make','Mutate','    - name: arrays\n      language: GO\n      module: src/go\n      sources: [src/go/backing.go]\n')]:
            for runtime in ('C','RS'):
                with self.subTest(language=language, runtime=runtime):
                    manifest=self.manifest.replace('runtime: C','runtime: '+runtime).replace('abi: 1.7','abi: 1.8').replace('  bindings:\n',build+'  bindings:\n')
                    unit='storage' if language=='C' else 'arrays'
                    manifest+=self.binding('arrayBody','arrayBody','facade','owned','a: borrowed')
                    manifest+=self.binding('makeArray',make,unit,'owned','')+self.binding('mutateArray',mutate,unit,'owned','a: borrowed, b: borrowed')
                    self.write('.sn/methods/sn.yaml',manifest)
                    built=subprocess.run([str(fixtures.COMPILER),'--build-native',str(self.package/'sn.yaml'),'-o',str(self.package/('built-'+language+'-'+runtime))],cwd=self.root,capture_output=True,timeout=180)
                    self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                    assembly=Path(json.loads(built.stdout)['assembly'])
                    for sealed in (False,True):
                        if sealed:
                            descriptor=assembly.relative_to(self.package)
                            digest=hashlib.sha256(assembly.read_bytes()).hexdigest()
                            self.write('.sn/methods/sn.yaml',manifest.replace('native:\n',f'native:\n  assembly: {{path: {descriptor}, sha256: {digest}}}\n'))
                            removed=[]
                            for path in (self.package/'src/counter.c', self.package/'src/backing.rs', self.package/'src/go/backing.go'):
                                removed.append((path,path.read_bytes()))
                                path.unlink()
                        for target in ('c','rust'):
                            for opt,mode in [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]:
                                exe=self.root/'app'
                                compiled=subprocess.run([str(fixtures.COMPILER),'main.sn','--no-install','--target',target,opt,*([] if mode=='default' else ['--'+mode]),'-o',str(exe)],cwd=self.root,capture_output=True,timeout=180)
                                self.assertEqual(compiled.returncode,0,compiled.stderr.decode(errors='replace'))
                                result=subprocess.run([str(exe)],capture_output=True,timeout=20)
                                self.assertEqual((result.returncode,result.stdout,result.stderr),(0,expected,b''))
                        if sealed:
                            for path,data in removed: path.write_bytes(data)

if __name__=='__main__':
    os.environ['GOEXPERIMENT']='cgocheck2'
    unittest.main(verbosity=2)
