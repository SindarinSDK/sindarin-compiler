#!/usr/bin/env python3
"""Owned opaque results keep their public pointer ABI and canonical cleanup."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import unittest

import callable_methods as fixtures

ROOT = fixtures.ROOT
COMPILER = fixtures.COMPILER


class OpaqueResultTests(fixtures.CallableMethodTests):
    def setUp(self):
        super().setUp()
        api = self.package / 'src/api.sn'
        source = api.read_text().replace('  amount: int\n', '  @alias "number"\n  amount: int\n').replace('native struct Counter as ref =>\n',
            'native struct Counter as ref =>\n  fn toOpaque(): *void =>\n    return makeOpaque(self.amount)\n')
        api.write_text(source + '@alias "counter_original_opaque"\nnative fn makeOpaque(value: int): *void\n'
            'native fn checkedOpaque(owner: Counter, mode: int): *void\n'
            'native fn checkedEmpty(mode: int): *void\n')
        header = self.package / 'src/counter.h'
        header.write_text(header.read_text().replace('__sn__amount','number').replace('#endif\n', 'void *counter_original_opaque(long long);\n#endif\n'))
        c = self.package / 'src/counter.c'
        c.write_text(c.read_text().replace('__sn__amount','number') + 'void *counter_original_opaque(long long value) { return value < 0 ? NULL : counter_original_make(value); }\n'
            'void *make_opaque(long long value) { return counter_original_opaque(value); }\n'
            '#include "sn_abi.h"\n'
            'uint32_t checked_opaque(CanonicalCounter *owner,long long mode,void **out) { *out=counter_original_make(owner ? owner->number : 17);return mode ? SN_ABI_OUT_OF_RANGE : SN_ABI_OK; }\n'
            'uint32_t checked_empty(long long mode,void **out) { return checked_opaque(NULL,mode,out); }\n')
        self.manifest += self.binding('makeOpaque', 'make_opaque', 'storage', 'owned', 'value: value')
        self.manifest += self.binding('Counter.toOpaque', 'Counter.toOpaque', 'facade', 'owned', 'self: borrowed')
        self.manifest += self.binding('checkedOpaque', 'checked_opaque', 'storage', 'owned', 'owner: borrowed, mode: value').replace('failure: abort','failure: status')
        self.manifest += self.binding('checkedEmpty', 'checked_empty', 'storage', 'owned', 'mode: value').replace('failure: abort','failure: status')
        self.manifest = self.manifest.replace('::makeOpaque\n', '::makeOpaque\n      result_type: src/api.sn::Counter\n')
        self.manifest = self.manifest.replace('::Counter.toOpaque\n', '::Counter.toOpaque\n      result_type: src/api.sn::Counter\n')
        for name in ('checkedOpaque','checkedEmpty'):
            self.manifest = self.manifest.replace('::'+name+'\n', '::'+name+'\n      result_type: src/api.sn::Counter\n')
        self.write('.sn/methods/sn.yaml', self.manifest)

    def test_owned_pointer_survives_source_owner_and_transfers_to_caller_cleanup(self):
        self.write('consume.c', '#include ".sn/methods/src/counter.h"\n'
            'long long consume_owned(void *value) { CanonicalCounter *p=value;long long result=counter_read(p);counter_release(p);return result; }\n')
        self.write('main.sn', 'import "methods/src/api"\n@source "consume.c"\n'
            '@alias "consume_owned"\nnative fn consumeOwned(value: *void): int\n'
            'native fn makeAndDrop(): *void =>\n  var item: Counter = Counter.make(5)\n'
            '  var alias: Counter = item\n  alias.amount=9\n  return item.toOpaque()\n'
            'native fn exercise(): void =>\n  var pointer: *void = makeAndDrop()\n'
            '  println(destroyed())\n  println(consumeOwned(pointer))\n  println(destroyed())\n'
            '  println(makeOpaque(-1) == nil)\nfn main(): void =>\n  exercise()\n')
        assembly = self.build()
        metadata = json.loads(assembly.read_text())
        opaque = [s for s in metadata['provider_signatures'] if s['binding'].get('result_type')]
        self.assertEqual(len(opaque), 4)
        self.assertTrue(all(s['return_type']['kind'] == 'pointer' and s['return_type']['native_record']['identity'] == 'methods/Counter@1'
            and s['return_type']['resource_layout']['fields'][0]['name'] == 'amount' for s in opaque))
        cases = []
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, self.package / 'sealed')
                descriptor = self.package / 'sealed' / assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                self.write('.sn/methods/sn.yaml', self.manifest.replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                (self.package / 'src/counter.c').unlink()
            for target in ('c', 'rust'):
                for opt in ('-O0', '-O1', '-O2'):
                    for mode in ('default', 'checked', 'unchecked'):
                        exe = self.root / 'consumer.exe'
                        command = [str(COMPILER), 'main.sn', '--no-install', '--target', target, opt, '-o', str(exe)]
                        if mode != 'default':
                            command.append('--' + mode)
                        built = subprocess.run(command, cwd=self.root, capture_output=True, timeout=180)
                        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
                        run = subprocess.run([str(exe)], cwd=self.root, capture_output=True, timeout=15)
                        expected = b'1\n9\n2\ntrue\n'
                        if os.name == 'nt':
                            expected = expected.replace(b'\n', b'\r\n')
                        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))
                        cases.append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode, 'passed': True})
        runtime = 'rs' if 'runtime: RS' in self.manifest else 'c'
        sanitized = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
        output = ROOT / f'.sn/package-opaque-results-{runtime}{sanitized}.json'
        output.parent.mkdir(exist_ok=True)
        output.write_text(json.dumps({'passed': True, 'cases': cases}, indent=2) + '\n')

    def test_missing_wrong_and_nonowned_pointer_contracts_are_rejected(self):
        variants = (
            (self.manifest.replace('      result_type: src/api.sn::Counter\n', ''), b'native result_type is required'),
            (self.manifest.replace('result_type: src/api.sn::Counter', 'result_type: src/api.sn::Missing'), b'native result_type requires'),
            (self.manifest.replace('::destroyed\n', '::destroyed\n      result_type: src/api.sn::Counter\n')
                .replace('parameters: {}, result: value', 'parameters: {}, result: owned'), b'cannot annotate other types'),
            (self.manifest.replace('parameters: {value: value}, result: owned', 'parameters: {value: value}, result: value'), b'native result_type requires'),
        )
        for manifest, message in variants:
            self.write('.sn/methods/sn.yaml', manifest)
            result = subprocess.run([str(COMPILER), '--build-native', str(self.package / 'sn.yaml'),
                '-o', str(self.package / 'rejected')], cwd=self.root, capture_output=True, timeout=90)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertFalse((self.package / 'rejected').exists())

    def test_raw_wire_failure_preserves_outputs_and_releases_owned_storage(self):
        assembly = self.build()
        metadata = json.loads(assembly.read_text())
        self.write('wire.c', '#include <assert.h>\n#include <stdint.h>\n#include <stddef.h>\n#include "sn_abi.h"\n#include ".sn/methods/src/counter.h"\n'
            'extern SnAbiValue *export_makeCounter(long long);extern long long export_destroyed(void);\n'
            'extern uint32_t export_checkedOpaque(SnAbiValue *,long long,SnAbiValue **);\n'
            'extern uint32_t export_checkedEmpty(long long,SnAbiValue **);\n'
            'int main(void) { SnAbiValue *owner=export_makeCounter(9),*out=(SnAbiValue *)(uintptr_t)1;\n'
            'assert(owner && export_destroyed()==0);assert(export_checkedOpaque(owner,0,NULL)==SN_ABI_INVALID_ARGUMENT);\n'
            'assert(export_checkedOpaque(owner,1,&out)==SN_ABI_OUT_OF_RANGE && out==(SnAbiValue *)(uintptr_t)1);\n'
            'assert(export_destroyed()==1);assert(export_checkedEmpty(1,&out)==SN_ABI_OUT_OF_RANGE && out==(SnAbiValue *)(uintptr_t)1);\n'
            'assert(export_destroyed()==2);assert(export_checkedOpaque(owner,0,&out)==SN_ABI_OK);\n'
            'SnAbiValue *bad=NULL;assert(sn_abi_v1_string_copy("wrong kind",&bad)==SN_ABI_OK);SnAbiValue *sentinel=(SnAbiValue *)(uintptr_t)1;\n'
            'assert(export_checkedOpaque(bad,0,&sentinel)==SN_ABI_WRONG_KIND && sentinel==(SnAbiValue *)(uintptr_t)1);sn_abi_v1_release(bad);\n'
            'void *p=(void *)(uintptr_t)1;assert(sn_abi_v1_resource_data_typed(out,"wrong",&p)==SN_ABI_WRONG_KIND && p==(void *)(uintptr_t)1);\n'
            'assert(sn_abi_v1_resource_data_typed(out,"methods/Counter@1",&p)==SN_ABI_OK && counter_read(p)==9 && counter_refs(p)==1);\n'
            'sn_abi_v1_release(out);assert(export_destroyed()==3);sn_abi_v1_release(owner);assert(export_destroyed()==4); }\n')
        cc = shlex.split(os.environ.get('SN_CC','clang' if os.name == 'nt' or os.sys.platform == 'darwin' else 'gcc'))
        exe = self.root / 'wire.exe'
        links = [str(assembly.parent / u['archive']) for u in metadata['units']]
        links += [metadata['shared_runtime']['archive']] + [f for u in metadata['units'] for f in u['native_link_flags']]
        command = cc + ['-UNDEBUG'] + shlex.split(os.environ.get('SN_CFLAGS',''))
        command += ['-I',str(COMPILER.parent/'include/runtime'),str(self.root/'wire.c')] + links + ['-o',str(exe)]
        built = subprocess.run(command,cwd=self.root,capture_output=True,timeout=90)
        self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
        run = subprocess.run([str(exe)],cwd=self.root,capture_output=True,timeout=15)
        self.assertEqual((run.returncode,run.stdout,run.stderr),(0,b'',b''))

    def test_rust_resource_body_preserves_owned_opaque_lifetimes(self):
        self.manifest = self.manifest.replace('runtime: C', 'runtime: RS')
        api = self.package / 'src/api.sn'
        api.write_text(api.read_text().replace('native fn destroyed():', '@alias "destroyed_original"\nnative fn destroyed():')
            .replace('native fn checkedEmpty(', '@alias "checked_empty_original"\nnative fn checkedEmpty('))
        source = self.package / 'src/counter.c'
        source.write_text(source.read_text() + 'long long destroyed_original(void) { return destroyed_count(); }\n'
            'void *checked_empty_original(long long mode) { void *value=NULL;uint32_t status=checked_empty(mode,&value);if(status) { counter_release(value);abort(); }return value; }\n')
        self.write('.sn/methods/sn.yaml', self.manifest)
        # Reuse the independent pointer-consumption protocol: the producer
        # dies before the returned pointer is read and explicitly released.
        self.test_owned_pointer_survives_source_owner_and_transfers_to_caller_cleanup()


if __name__ == '__main__':
    unittest.main(verbosity=2)
