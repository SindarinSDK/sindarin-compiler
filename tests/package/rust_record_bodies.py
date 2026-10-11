#!/usr/bin/env python3
"""Canonical C record ownership through independently compiled Rust Sindarin bodies."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import shlex
import sys
import subprocess
import unittest
import callable_methods as fixtures

ROOT = fixtures.ROOT
COMPILER = fixtures.COMPILER


class RustRecordBodyTests(fixtures.CallableMethodTests):
    def setUp(self):
        super().setUp()
        api = self.package / 'src/api.sn'
        api.write_text(api.read_text().replace('native fn same(', '@alias "same_counter"\nnative fn same(') +
            '@alias "counter_credit_count"\nnative fn refs(item: Counter): int32\n'
            'fn bodyCredits(item: Counter): int32 =>\n  return refs(item)\n'
            'fn bodyAlias(item: Counter, other: Counter): Counter =>\n'
            '  if item==nil =>\n    return nil\n'
            '  if same(item,other) =>\n    item.amount+=1\n  return item\n'
            'fn bodyBorrow(item: Counter): Counter =>\n  return item\n')
        backing=self.package/'src/counter.c'
        backing.write_text(backing.read_text()+'int counter_credit_count(CanonicalCounter *value) { return counter_refs(value); }\nint counter_credit_backing(CanonicalCounter *value) { return counter_credit_count(value); }\nbool same_backing(CanonicalCounter *a, CanonicalCounter *b) { return same_counter(a,b); }\n')
        self.manifest = self.manifest.replace('runtime: C', 'runtime: RS').replace('function: same_counter', 'function: same_backing')
        for name, function, build, result, params in (
                ('refs', 'counter_credit_backing', 'storage', 'value', 'item: borrowed'),
                ('bodyCredits', 'bodyCredits', 'facade', 'value', 'item: borrowed'),
                ('bodyAlias', 'bodyAlias', 'facade', 'owned', 'item: borrowed, other: borrowed'),
                ('bodyBorrow', 'bodyBorrow', 'facade', 'borrowed', 'item: borrowed')):
            binding = self.binding(name, function, build, result, params)
            if result == 'borrowed':
                binding = binding.replace('result: borrowed}', 'result: borrowed, borrowed_from: item}')
            if build=='facade':binding=binding.replace('failure: abort','failure: status')
            self.manifest += binding
        self.write('.sn/methods/sn.yaml', self.manifest)

    def test_wire_kind_validation_and_transferred_return_credit(self):
        assembly=self.build();metadata=json.loads(assembly.read_text())
        self.write('wire.c', r'''#undef NDEBUG
#include "sn_abi.h"
#include "counter.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
extern SnAbiValue *export_Counter_make(int64_t);
extern uint32_t export_bodyBorrow(SnAbiValue *, SnAbiValue **);
extern uint32_t export_bodyAlias(SnAbiValue *, SnAbiValue *, SnAbiValue **);
extern int64_t export_destroyed(void);
int main(void) {
  SnAbiValue *owner=export_Counter_make(7), *result=(void *)(uintptr_t)1;
  assert(owner);
  CanonicalCounter *pointer=NULL;
  assert(sn_abi_v1_resource_data_typed(owner,"methods/Counter@1",(void **)&pointer)==0);
  assert(counter_refs(pointer)==1 && pointer->__sn__amount==7);
  SnAbiValue *wrong=NULL;
  assert(sn_abi_v1_string_copy("wrong",&wrong)==0);
  assert(export_bodyBorrow(wrong,&result)==SN_ABI_WRONG_KIND && result==(void *)(uintptr_t)1);
  sn_abi_v1_release(wrong);
  assert(sn_abi_v1_resource_new_typed("wrong/Counter@1",pointer,NULL,0,&wrong)==0);
  assert(export_bodyBorrow(wrong,&result)==SN_ABI_WRONG_KIND && result==(void *)(uintptr_t)1);
  sn_abi_v1_release(wrong);
  assert(export_bodyBorrow(owner,NULL)==SN_ABI_INVALID_ARGUMENT && counter_refs(pointer)==1);
  assert(export_bodyBorrow(owner,&result)==0 && counter_refs(pointer)==2);
  sn_abi_v1_release(owner);
  assert(counter_refs(pointer)==1 && export_destroyed()==0);
  CanonicalCounter *returned=NULL;
  assert(sn_abi_v1_resource_data_typed(result,"methods/Counter@1",(void **)&returned)==0 && returned==pointer);
  SnAbiValue *again=NULL;
  assert(export_bodyAlias(result,result,&again)==0 && pointer->__sn__amount==8 && counter_refs(pointer)==2);
  sn_abi_v1_release(result);sn_abi_v1_release(again);
  assert(export_destroyed()==1);
  result=(void *)(uintptr_t)1;
  assert(export_bodyBorrow(NULL,&result)==0 && !result);
}
''')
        sys.path.insert(0,str(ROOT/'scripts'))
        from prepare_native_imports import native_link_options
        cc=shlex.split(os.environ.get('SN_CC','clang' if os.name=='nt' or sys.platform=='darwin' else 'gcc'))
        runtime=COMPILER.parent/'lib'/('clang' if os.name=='nt' else 'gcc')/'libsn_runtime_min.a'
        links=[flag for unit in metadata['units'] for flag in native_link_options(unit['native_link_flags'])]
        exe=self.root/'wire.exe'
        command=cc+['-std=c11','-UNDEBUG']+shlex.split(os.environ.get('SN_CFLAGS',''))
        command+=['-I',str(COMPILER.parent/'include/runtime'),'-I',str(self.package/'src'),str(self.root/'wire.c')]
        command+=[str(assembly.parent/unit['archive']) for unit in metadata['units']]+[str(runtime)]+links+['-o',str(exe)]
        run=subprocess.run(command,cwd=self.root,capture_output=True,timeout=180)
        self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))
        run=subprocess.run([str(exe)],capture_output=True,timeout=30)
        self.assertEqual((run.returncode,run.stdout,run.stderr),(0,b'',b''))

    def test_c_body_transfers_its_existing_return_credit(self):
        self.manifest=self.manifest.replace('runtime: RS','runtime: C')
        self.write('.sn/methods/sn.yaml',self.manifest)
        self.test_wire_kind_validation_and_transferred_return_credit()

    def test_body_inputs_aliases_nil_and_result_credits(self):
        self.write('main.sn', 'import "methods/src/api"\nfn exercise(): void =>\n'
            '  var item: Counter = Counter.make(5)\n  println(bodyCredits(item))\n'
            '  var alias: Counter = bodyAlias(item,item)\n  println(same(item,alias))\n'
            '  println(item.amount)\n  alias.amount=19\n  println(item.read())\n'
            '  var borrowed: Counter = bodyBorrow(item)\n  println(same(item,borrowed))\n'
            '  println(bodyAlias(nil,nil)==nil)\n  println(bodyBorrow(nil)==nil)\n'
            'fn main(): void =>\n  exercise()\n  println(destroyed())\n')
        assembly = self.build()
        cases = []
        for artifact in ('source','sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent,self.package/'sealed')
                descriptor = self.package/'sealed'/assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                self.write('.sn/methods/sn.yaml',self.manifest.replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                (self.package/'src/counter.c').unlink()
            for target in ('c','rust'):
                for opt in ('-O0','-O1','-O2'):
                    for mode in ('default','checked','unchecked'):
                        exe = self.root/'consumer.exe'
                        command = [str(COMPILER),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                        if mode!='default':command.append('--'+mode)
                        run = subprocess.run(command,cwd=self.root,capture_output=True,timeout=180)
                        self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))
                        run = subprocess.run([str(exe)],cwd=self.root,capture_output=True,timeout=20)
                        expected=b'2\ntrue\n6\n19\ntrue\ntrue\ntrue\n1\n'
                        if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                        self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))
                        cases.append(dict(artifact=artifact,target=target,opt=opt,mode=mode,passed=True))
        output=ROOT/'.sn/rust-record-body-ownership.json';output.parent.mkdir(exist_ok=True)
        output.write_text(json.dumps(dict(passed=True,cases=cases),indent=2)+'\n')


if __name__=='__main__':unittest.main()
