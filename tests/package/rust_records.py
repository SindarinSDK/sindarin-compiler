#!/usr/bin/env python3
"""Rust native backing retains canonical C record identity and lifecycle."""
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


RAW_CLIENT = '#include <assert.h>\n#include <stdint.h>\n#include <stddef.h>\n#include "sn_abi.h"\n#include "counter.h"\nextern uint32_t export_makeRust(int64_t, SnAbiValue **);\nextern uint32_t export_borrowRust(SnAbiValue *, SnAbiValue *, SnAbiValue **);\nextern uint32_t export_checkedRust(SnAbiValue *, SnAbiValue *, int64_t, SnAbiValue **);\nextern int64_t export_destroyed(void);\nstatic SnAbiValue *owner;\nstatic int visited;\nstatic void consume_credit(void) { sn_abi_v1_release(owner); owner=NULL; visited++; }\nint main(void) {\n    SnAbiValue *out=(SnAbiValue *)(uintptr_t)1;\n    assert(export_makeRust(7, NULL)==SN_ABI_INVALID_ARGUMENT);\n    assert(export_makeRust(-1, &out)==SN_ABI_OK && out==NULL);\n    assert(export_makeRust(7, &owner)==SN_ABI_OK && owner);\n    CanonicalCounter *p=NULL;\n    assert(sn_abi_v1_resource_data_typed(owner,"methods/Counter@1",(void **)&p)==SN_ABI_OK);\n    assert(p && counter_refs(p)==1 && p->__sn__amount==7);\n    SnAbiValue *wrong=NULL;\n    assert(sn_abi_v1_string_copy("wrong", &wrong)==SN_ABI_OK);\n    out=(SnAbiValue *)(uintptr_t)1;\n    assert(export_checkedRust(wrong,owner,0,&out)==SN_ABI_WRONG_KIND && out==(SnAbiValue *)(uintptr_t)1);\n    sn_abi_v1_release(wrong);\n    assert(sn_abi_v1_resource_new_typed("other/Counter@1",p,NULL,0,&wrong)==SN_ABI_OK);\n    assert(export_checkedRust(wrong,owner,0,&out)==SN_ABI_WRONG_KIND && out==(SnAbiValue *)(uintptr_t)1);\n    sn_abi_v1_release(wrong);\n    assert(p->__sn__amount==7 && counter_refs(p)==1);\n    assert(export_checkedRust(owner,owner,1,&out)==SN_ABI_OUT_OF_RANGE && out==(SnAbiValue *)(uintptr_t)1);\n    assert(export_checkedRust(owner,owner,2,&out)==SN_ABI_FOREIGN_ERROR && out==(SnAbiValue *)(uintptr_t)1);\n    assert(counter_refs(p)==1 && export_destroyed()==0);\n    counter_set_callback(consume_credit);\n    assert(export_checkedRust(owner,owner,3,&out)==SN_ABI_OK && out && !owner && visited==1);\n    CanonicalCounter *result=NULL;\n    assert(sn_abi_v1_resource_data_typed(out,"methods/Counter@1",(void **)&result)==SN_ABI_OK && result==p);\n    assert(counter_refs(result)==1 && result->__sn__amount==8 && export_destroyed()==0);\n    sn_abi_v1_release(out);\n    assert(export_destroyed()==1);\n    out=(SnAbiValue *)(uintptr_t)1;\n    assert(export_borrowRust(NULL,NULL,&out)==SN_ABI_OK && out==NULL);\n}\n'

class RustRecordTests(unittest.TestCase):
    binding = staticmethod(fixtures.CallableMethodTests.binding)
    write = fixtures.CallableMethodTests.write

    def setUp(self):
        fixtures.CallableMethodTests.setUp(self)
        api = self.package / 'src/api.sn'
        api.write_text(api.read_text() + 'native fn makeRust(value: int): Counter\n'
            'native fn borrowRust(a: Counter, b: Counter): Counter\n'
            'native fn checkedRust(a: Counter, b: Counter, mode: int): Counter\n')
        self.write('.sn/methods/src/backing.rs',
            'use std::ffi::c_void;\n#[repr(C)] struct Counter { rc:i32, amount:i64 }\n'
            'extern "C" { fn counter_create()->*mut Counter; fn counter_visit(); }\n'
            'pub fn make_rust(value:i64)->Result<*mut c_void,u32> { if value<0 { return Ok(std::ptr::null_mut()); } '
            'unsafe { let p=counter_create();(*p).amount=value;Ok(p.cast()) } }\n'
            'pub fn borrow_rust(a:*mut c_void,b:*mut c_void)->Result<*mut c_void,u32> { assert_eq!(a,b);'
            'if !a.is_null() {unsafe {(*a.cast::<Counter>()).amount+=1;} } Ok(a) }\n'
            'pub fn checked_rust(a:*mut c_void,b:*mut c_void,mode:i64)->Result<*mut c_void,u32> {'
            'if mode==1 {return Err(5);} if mode==2 {panic!("record foreign panic");} if mode==3 { unsafe { counter_visit(); } } borrow_rust(a,b) }\n')
        source = self.package / 'src/counter.c'
        source.write_text(source.read_text() + 'static void (*visit)(void);\n'
            'void counter_set_callback(void (*value)(void)) { visit=value; }\n'
            'void counter_visit(void) { if(visit) visit(); }\n')
        header = self.package / 'src/counter.h'
        header.write_text(header.read_text().replace('#endif\n',
            'void counter_set_callback(void (*)(void));\nvoid counter_visit(void);\n#endif\n'))
        self.manifest = self.manifest.replace('  bindings:\n',
            '    - name: rust_ops\n      language: RS\n      entry: src/backing.rs\n      sources: [src/backing.rs]\n  bindings:\n')
        for name, function, result, params in (
                ('makeRust', 'make_rust', 'owned', 'value: value'),
                ('borrowRust', 'borrow_rust', 'borrowed', 'a: borrowed, b: borrowed'),
                ('checkedRust', 'checked_rust', 'borrowed', 'a: borrowed, b: borrowed, mode: value')):
            binding = self.binding(name, function, 'rust_ops', result, params).replace('failure: abort', 'failure: status')
            if result == 'borrowed':
                binding = binding.replace('result: borrowed}', 'result: borrowed, borrowed_from: a}')
            self.manifest += binding
        self.write('.sn/methods/sn.yaml', self.manifest)
        self.write('main.sn', 'import "methods/src/api"\nfn exercise(): void =>\n'
            '  var item: Counter = makeRust(5)\n  var alias: Counter = borrowRust(item,item)\n'
            '  println(same(item,alias))\n  println(item.amount)\n  alias.amount=9\n'
            '  var another: Counter = checkedRust(alias,item,0)\n  println(item.amount)\n'
            '  println(same(item,another))\n  println(makeRust(-1)==nil)\n'
            '  println(borrowRust(nil,nil)==nil)\nfn main(): void =>\n  exercise()\n  println(destroyed())\n')

    def execute(self, target, expected, flags=()):
        executable = self.root / 'consumer.exe'
        built = subprocess.run([str(COMPILER), 'main.sn', '--no-install', '--target', target,
            *flags, '-o', str(executable)], cwd=self.root, capture_output=True, timeout=180)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        run = subprocess.run([str(executable)], cwd=self.root, capture_output=True, timeout=20)
        if os.name == 'nt':
            expected = expected.replace(b'\n', b'\r\n')
        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))

    def test_rust_records_preserve_live_aliases_and_sealed_cleanup(self):
        expected = b'true\n6\n10\ntrue\ntrue\ntrue\n1\n'
        cases = []
        built = subprocess.run([str(COMPILER), '--build-native', str(self.package/'sn.yaml'),
            '-o', str(self.package/'built')], cwd=self.root, capture_output=True, timeout=180)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        self.assertEqual({u['language'] for u in metadata['units']}, {'C', 'SN', 'RS'})
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, self.package/'sealed')
                descriptor = self.package/'sealed'/assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                self.write('.sn/methods/sn.yaml', self.manifest.replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                (self.package/'src/counter.c').unlink()
                (self.package/'src/backing.rs').unlink()
                # The public facade carries bodies and is immutable API data.
            for target in ('c', 'rust'):
                for opt in ('-O0', '-O1', '-O2'):
                    for mode in ('default', 'checked', 'unchecked'):
                        flags = [opt] + ([] if mode == 'default' else ['--'+mode])
                        self.execute(target, expected, flags)
                        cases.append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode, 'passed': True})
        (ROOT/'.sn').mkdir(exist_ok=True)
        (ROOT/'.sn/rust-record-providers.json').write_text(json.dumps({'passed': True, 'cases': cases}, indent=2)+'\n')

    def test_wire_errors_panics_and_callback_credit_loss(self):
        built = subprocess.run([str(COMPILER), '--build-native', str(self.package/'sn.yaml'),
            '-o', str(self.package/'built')], cwd=self.root, capture_output=True, timeout=180)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        self.write('wire.c', RAW_CLIENT)
        cc = shlex.split(os.environ.get('SN_CC', 'clang' if os.name == 'nt' or sys.platform == 'darwin' else 'gcc'))
        runtime = COMPILER.parent/'lib'/('clang' if os.name == 'nt' else 'gcc')/'libsn_runtime_min.a'
        sys.path.insert(0, str(ROOT/'scripts'))
        from prepare_native_imports import native_link_options
        libraries = []
        for unit in metadata['units']:
            libraries += native_link_options(unit['native_link_flags'])
        executable = self.root/'wire.exe'
        command = cc + ['-std=c11', '-UNDEBUG'] + shlex.split(os.environ.get('SN_CFLAGS', ''))
        command += ['-I', str(COMPILER.parent/'include/runtime'), '-I', str(self.package/'src'), str(self.root/'wire.c')]
        command += [str(assembly.parent/unit['archive']) for unit in metadata['units']]
        command += [str(runtime)] + libraries + ['-o', str(executable)]
        compiled = subprocess.run(command, cwd=self.root, capture_output=True, timeout=180)
        self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors='replace'))
        run = subprocess.run([str(executable)], capture_output=True, timeout=30)
        self.assertEqual(run.returncode, 0, run.stderr.decode(errors='replace'))
        self.assertEqual(run.stdout, b'')
        self.assertIn(b'record foreign panic', run.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
