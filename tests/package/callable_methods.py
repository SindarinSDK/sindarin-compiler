#!/usr/bin/env python3
"""Preserve static/instance method identity in source and sealed package adapters."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('SN_COMPILER', ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))).resolve()


class CallableMethodTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='sn-package-methods-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.package = self.root / '.sn/methods'
        self.write('.sn/methods/src/api.sn',
            'native struct Counter as ref =>\n  amount: int\n'
            '  static fn read(value: int): int =>\n    return value+10\n'
            '  @alias "counter_read"\n  native fn read(): int\n'
            '  static fn make(value: int): Counter =>\n    return makeCounter(value)\n'
            '@alias "counter_original_make"\nnative fn makeCounter(value: int): Counter\nnative fn destroyed(): int\n'
            'native fn same(a: Counter, b: Counter): bool\n')
        self.write('.sn/methods/src/counter.h',
            '#ifndef COUNTER_H\n#define COUNTER_H\n'
            'typedef struct { int __rc__; long long __sn__amount; } CanonicalCounter;\n'
            'CanonicalCounter *counter_create(void);\nCanonicalCounter *counter_retain(CanonicalCounter *);\n'
            'void counter_release(CanonicalCounter *);\nint counter_refs(CanonicalCounter *);\n'
            'long long counter_read(CanonicalCounter *);\nCanonicalCounter *counter_original_make(long long);\n#endif\n')
        self.write('.sn/methods/src/counter.c',
            '#include "counter.h"\n#include <stdlib.h>\n#include <stdbool.h>\nstatic int dead;\n'
            'CanonicalCounter *counter_create(void) { CanonicalCounter *p=calloc(1,sizeof(*p)); if(!p) abort(); p->__rc__=1; return p; }\n'
            'CanonicalCounter *counter_retain(CanonicalCounter *p) { if(p) __atomic_add_fetch(&p->__rc__,1,__ATOMIC_RELAXED); return p; }\n'
            'void counter_release(CanonicalCounter *p) { if(p && __atomic_sub_fetch(&p->__rc__,1,__ATOMIC_ACQ_REL)==0) { dead++; free(p); } }\n'
            'int counter_refs(CanonicalCounter *p) { return p ? __atomic_load_n(&p->__rc__,__ATOMIC_RELAXED) : 0; }\n'
            'long long counter_read(CanonicalCounter *p) { return p ? p->__sn__amount : -1; }\n'
            'CanonicalCounter *counter_original_make(long long value) { CanonicalCounter *p=counter_create();p->__sn__amount=value;return p; }\n'
            'CanonicalCounter *make_counter(long long value) { return counter_original_make(value); }\n'
            'long long destroyed_count(void) { return dead; }\n'
            'bool same_counter(CanonicalCounter *a, CanonicalCounter *b) { return a == b; }\n')
        self.manifest = (
            'name: methods\nruntime: C\nnative:\n  abi: 1.7\n  declarations: [src/api.sn]\n'
            '  types:\n    - declaration: src/api.sn::Counter\n      identity: methods/Counter@1\n'
            '      c_type: CanonicalCounter\n      header: src/counter.h\n      create: counter_create\n'
            '      retain: counter_retain\n      release: counter_release\n      refs: counter_refs\n      owner: atomic\n'
            '  builds:\n    - name: storage\n      language: C\n      sources: [src/counter.c]\n'
            '    - name: facade\n      language: SN\n      entry: src/api.sn\n      sources: [src/api.sn]\n'
            '  bindings:\n'
            + self.binding('makeCounter', 'make_counter', 'storage', 'owned', 'value: value')
            + self.binding('destroyed', 'destroyed_count', 'storage', 'value', '')
            + self.binding('same', 'same_counter', 'storage', 'value', 'a: borrowed, b: borrowed')
            + self.binding('Counter.read', 'Counter.read', 'facade', 'value', 'value: value', 'static')
            + self.binding('Counter.read', 'Counter.read', 'facade', 'value', 'self: borrowed', 'instance')
            + self.binding('Counter.make', 'Counter.make', 'facade', 'owned', 'value: value'))
        self.write('.sn/methods/sn.yaml', self.manifest)
        self.write('main.sn',
            'import "methods/src/api"\nfn exercise(): void =>\n'
            '  var item: Counter = Counter.make(5)\n  var alias: Counter = item\n  alias.amount=9\n'
            '  println(Counter.read(2))\n  println(item.read())\n  println(Counter.read(item.read()))\n'
            '  println(same(item, alias))\nfn main(): void =>\n  exercise()\n  println(destroyed())\n')

    @staticmethod
    def binding(name, function, build, result, params, selector=None):
        suffix = '_' + selector if selector else ''
        return (f'    - declaration: src/api.sn::{name}\n'
            + (f'      method: {selector}\n' if selector else '')
            + f'      function: {function}\n      symbol: export_{name.replace(".", "_")}{suffix}\n'
            + f'      build: {build}\n      convention: C\n      failure: abort\n'
            + f'      ownership: {{parameters: {{{params}}}, result: {result}}}\n')

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')

    def build(self):
        result = subprocess.run([str(COMPILER), '--build-native', str(self.package / 'sn.yaml'),
            '-o', str(self.package / 'built')], cwd=self.root, capture_output=True, timeout=180)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))
        return Path(json.loads(result.stdout)['assembly'])

    def test_static_instance_calls_preserve_aliases_and_cleanup_in_all_modes(self):
        assembly = self.build()
        metadata = json.loads(assembly.read_text())
        signatures = metadata['provider_signatures']
        methods = [s for s in signatures if s['binding']['declaration'].endswith('::Counter.read')]
        self.assertEqual({s['binding']['method']: [p['name'] for p in s['params']] for s in methods},
            {'static': ['value'], 'instance': ['self']})
        cases = []
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, self.package / 'sealed')
                descriptor = self.package / 'sealed' / assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                self.write('.sn/methods/sn.yaml', self.manifest.replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                for path in (self.package / 'src').glob('*'):
                    # The public layout header is part of the sealed API and
                    # is hash-checked against the descriptor; backing C is gone.
                    if path.suffix == '.c':
                        path.unlink()
            for target in ('c', 'rust'):
                for opt in ('-O0', '-O1', '-O2'):
                    for mode in ('default', 'checked', 'unchecked'):
                        exe = self.root / 'consumer.exe'
                        command = [str(COMPILER), 'main.sn', '--no-install', '--target', target, opt, '-o', str(exe)]
                        if mode != 'default':
                            command.append('--' + mode)
                        built = subprocess.run(command, cwd=self.root, capture_output=True, timeout=180)
                        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
                        run = subprocess.run([str(exe)], cwd=self.root, capture_output=True, timeout=20)
                        expected = b'12\n9\n19\ntrue\n1\n'
                        if os.name == 'nt':
                            expected = expected.replace(b'\n', b'\r\n')
                        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))
                        cases.append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode, 'passed': True})
        destination = ROOT / '.sn/package-callable-methods.json'
        destination.parent.mkdir(exist_ok=True)
        destination.write_text(json.dumps({'passed': True, 'cases': cases}, indent=2) + '\n')

    def test_invalid_and_ambiguous_selectors_are_rejected(self):
        variants = (
            (self.manifest.replace('method: static', 'method: automatic'), b'native method selector'),
            (self.manifest.replace('method: instance', 'method: static'), b'duplicate native binding declaration'),
            (self.manifest.replace('::makeCounter\n', '::makeCounter\n      method: static\n'), b'native method selector'),
            (self.manifest.replace(self.binding('Counter.read', 'Counter.read', 'facade', 'value', 'self: borrowed', 'instance'), '')
                .replace('      method: static\n', ''), b'ambiguous package method'),
        )
        for manifest, message in variants:
            self.write('.sn/methods/sn.yaml', manifest)
            result = subprocess.run([str(COMPILER), '--build-native', str(self.package / 'sn.yaml'),
                '-o', str(self.package / 'rejected')], cwd=self.root, capture_output=True, timeout=90)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertFalse((self.package / 'rejected').exists())
            for target in ('c', 'rust'):
                output = self.root / (target + '-rejected.json')
                result = subprocess.run([str(COMPILER), 'main.sn', '--no-install', '--target', target,
                    '--emit-model', '-o', str(output)], cwd=self.root, capture_output=True, timeout=90)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)
                self.assertFalse(output.exists())

    def test_rust_body_static_export_uses_private_collision_name(self):
        self.write('.sn/methods/src/api.sn',
            'struct Counter =>\n  amount: int\n'
            '  static fn read(value: int): int =>\n    return value+10\n'
            '  fn read(): int =>\n    return self.amount\n'
            'fn exercise(): int =>\n  var item: Counter = Counter {amount: 5}\n'
            '  return Counter.read(item.read())\n')
        self.write('.sn/methods/sn.yaml',
            'name: methods\nruntime: RS\nnative:\n  abi: 1.7\n  declarations: [src/api.sn]\n'
            '  builds:\n    - name: facade\n      language: SN\n      entry: src/api.sn\n      sources: [src/api.sn]\n'
            '  bindings:\n'
            + self.binding('Counter.read', 'Counter.read', 'facade', 'value', 'value: value', 'static')
            + self.binding('exercise', 'exercise', 'facade', 'value', ''))
        assembly = self.build()
        metadata = json.loads(assembly.read_text())
        unit = metadata['units'][0]
        archive = assembly.parent / unit['archive']
        links = [str(archive), metadata['shared_runtime']['archive']] + unit['native_link_flags']
        exe = self.root / 'wire-client.exe'
        self.write('wire-client.c',
            '#include <assert.h>\nextern long long export_Counter_read_static(long long);\n'
            'extern long long export_exercise(void);\n'
            'int main(void) { assert(export_Counter_read_static(2)==12);assert(export_exercise()==15); }\n')
        cc = shlex.split(os.environ.get('SN_CC', 'clang' if os.name == 'nt' or os.sys.platform == 'darwin' else 'gcc'))
        command = cc + ['-UNDEBUG'] + shlex.split(os.environ.get('SN_CFLAGS', ''))
        command += [str(self.root / 'wire-client.c')] + links + ['-o', str(exe)]
        built = subprocess.run(command, cwd=self.root, capture_output=True, timeout=90)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        run = subprocess.run([str(exe)], cwd=self.root, capture_output=True, timeout=15)
        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, b'', b''))
        rustflags = shlex.split(os.environ.get('SN_RUSTFLAGS', ''))
        configured_probe = ('sn_wire_client_probe' in rustflags or '--cfg=sn_wire_client_probe' in rustflags)
        probe = '#[cfg(not(sn_wire_client_probe))] compile_error!("configured Rust client flags were ignored");\n' if configured_probe else ''
        self.write('wire-client.rs', probe +
            'extern "C" { fn export_Counter_read_static(value:i64)->i64;fn export_exercise()->i64; }\n'
            'fn main() { unsafe { assert_eq!(export_Counter_read_static(2),12);assert_eq!(export_exercise(),15); } }\n')
        rustc = shlex.split(os.environ.get('SN_RUSTC', 'rustc'))
        command = rustc + ['--edition=2021', str(self.root / 'wire-client.rs'), '-C', 'default-linker-libraries=yes',
            '-C', 'linker=' + cc[0], '-o', str(exe)]
        command += rustflags
        command += [value for flag in cc[1:] + shlex.split(os.environ.get('SN_CFLAGS', '')) for value in ('-C', 'link-arg=' + flag)]
        command += [value for link in links for value in ('-C', 'link-arg=' + link)]
        built = subprocess.run(command, cwd=self.root, capture_output=True, timeout=90)
        self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
        run = subprocess.run([str(exe)], cwd=self.root, capture_output=True, timeout=15)
        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, b'', b''))

    def test_configured_flags_reach_raw_rust_client(self):
        configured = os.environ.get('SN_RUSTFLAGS', '') + ' --cfg=sn_wire_client_probe'
        with patch.dict(os.environ, {'SN_RUSTFLAGS': configured}):
            self.test_rust_body_static_export_uses_private_collision_name()


if __name__ == '__main__':
    unittest.main(verbosity=2)
