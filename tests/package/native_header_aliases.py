#!/usr/bin/env python3
"""Private native imports preserve foreign headers, fields and provider dispatch."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import unittest

import callable_methods as fixtures


class NativeHeaderAliases(unittest.TestCase):
    setUp = fixtures.CallableMethodTests.setUp
    write = fixtures.CallableMethodTests.write
    binding = staticmethod(fixtures.CallableMethodTests.binding)

    def test_binding_contract_rejects_invalid_names_duplicates_and_defined_bodies(self):
        filename = self.root / 'bindings.json'
        valid = {'name': 'makeCounter', 'kind': 'function', 'symbol': '__private_make'}
        probes = [({}, 'JSON array'),
                  ([dict(valid, symbol='not an identifier')], 'invalid or duplicate'),
                  ([dict(valid, symbol='prefix\0suffix')], 'invalid or duplicate'),
                  ([valid, valid], 'invalid or duplicate'),
                  ([dict(valid, name='missing')], 'matching native declaration'),
                  ([dict(valid, name='Counter.read', kind='static')], 'matching native declaration')]
        for data, diagnostic in probes:
            with self.subTest(data=data):
                filename.write_text(json.dumps(data))
                result = subprocess.run([str(fixtures.COMPILER), str(self.package/'src/api.sn'),
                    '--no-install', '--package-body', '--emit-source',
                    '--package-native-bindings', str(filename), '-o', str(self.root/'invalid.c')],
                    cwd=self.root, capture_output=True, timeout=30)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stderr.decode(errors='replace'))

    def test_headers_fields_and_function_references_use_declared_providers(self):
        api = self.package / 'src/api.sn'
        include = (self.package / 'src/no-guard.h').as_posix()
        api.write_text('@include <math.h>\n@include "'+include+'"\n' +
            api.read_text().replace('  amount: int\n', '  @alias "cos"\n  amount: int\n') +
            '@alias "cos"\nnative fn cosine(value: double): double\n'
            '@alias "cosf"\nnative fn cosineF(value: float): float\n'
            'fn calculate(): double =>\n  return 0.0\n'
            'fn implementation(): double =>\n'
            '  var item: Counter = Counter.make(7)\n'
            '  var function = cosine\n'
            '  return item.amount * 1.0 + cosine(0.0) + cosineF(0.0f) + function(0.0)\n')
        header = self.package / 'src/counter.h'
        header.write_text(header.read_text().replace('__sn__amount', 'cos'))
        source = self.package / 'src/counter.c'
        source.write_text('#include <math.h>\n' + source.read_text().replace('__sn__amount', 'cos') +
            'double provider_cos(double value) { return 4.0 + cos(value); }\n'
            'float provider_cosf(float value) { return 2.0f + cosf(value); }\n')
        self.write('.sn/methods/src/no-guard.h',
                   'typedef struct { long long untouched; } HeaderIncludedExactlyOnce;\n')
        manifest = self.manifest.replace('      sources: [src/counter.c]\n',
            '      sources: [src/counter.c]\n      libraries: [m]\n').replace(
            '      sources: [src/api.sn]\n',
            '      sources: [src/api.sn]\n      include_dirs: [src]\n')
        manifest += self.binding('cosine', 'provider_cos', 'storage', 'value', 'value: value')
        manifest += self.binding('cosineF', 'provider_cosf', 'storage', 'value', 'value: value')
        manifest += self.binding('calculate', 'implementation', 'facade', 'value', '')
        manifest += self.binding('implementation', 'implementation', 'facade', 'value', '')
        self.write('main.sn', 'import "methods/src/api"\nfn main(): void =>\n  println(calculate())\n')
        frozen = {path: path.read_bytes() for path in (api, header, source,
                  self.package / 'src/no-guard.h')}
        cases = []
        for runtime in ('C', 'RS'):
            self.write('.sn/methods/sn.yaml', manifest.replace('runtime: C', 'runtime: '+runtime))
            built = subprocess.run([str(fixtures.COMPILER), '--build-native', str(self.package / 'sn.yaml'),
                '-o', str(self.package / ('built-'+runtime))], cwd=self.root,
                capture_output=True, timeout=240)
            self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
            assembly = Path(json.loads(built.stdout)['assembly'])
            for sealed in (False, True):
                if sealed:
                    destination = self.package / ('sealed-'+runtime)
                    shutil.copytree(assembly.parent, destination)
                    descriptor = destination / assembly.name
                    digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                    self.write('.sn/methods/sn.yaml', manifest.replace('runtime: C', 'runtime: '+runtime).replace(
                        'native:\n', f'native:\n  assembly: {{path: sealed-{runtime}/assembly.json, sha256: {digest}}}\n'))
                    source.unlink()
                for target in ('c', 'rust'):
                    for optimization in ('-O0', '-O1', '-O2'):
                        for mode in ('default', 'checked', 'unchecked'):
                                with self.subTest(runtime=runtime, sealed=sealed, target=target,
                                                  optimization=optimization, mode=mode):
                                    executable = self.root / 'client.exe'
                                    compiled = subprocess.run([str(fixtures.COMPILER), 'main.sn', '--no-install',
                                        '--target', target, optimization,
                                        *([] if mode == 'default' else ['--'+mode]), '-o', str(executable)], cwd=self.root,
                                        capture_output=True, timeout=180)
                                    self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors='replace'))
                                    result = subprocess.run([str(executable)], cwd=self.root, capture_output=True, timeout=20)
                                    self.assertEqual((result.returncode, result.stdout, result.stderr),
                                        (0, b'20.00000\r\n' if os.name == 'nt' else b'20.00000\n', b''))
                                    cases.append({'runtime': runtime, 'sealed': sealed, 'target': target,
                                        'optimization': optimization, 'mode': mode,
                                        'compile_status': compiled.returncode, 'run_status': result.returncode,
                                        'stdout_hex': result.stdout.hex(), 'stderr_hex': result.stderr.hex()})
                if sealed: source.write_bytes(frozen[source])
            for path, data in frozen.items():self.assertEqual(path.read_bytes(), data)
        self.assertEqual(len(cases), 72)
        report = {'passed': True, 'cases': cases,
            'compiler_sha256': hashlib.sha256(fixtures.COMPILER.read_bytes()).hexdigest(),
            'original_fixture_sha256': {str(path.relative_to(self.root)): hashlib.sha256(data).hexdigest()
                                        for path, data in frozen.items()},
            'environment': {name: os.environ.get(name) for name in
                            ('SN_CC', 'SN_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
        suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
        output = fixtures.ROOT/'.sn'/f'native-header-aliases{suffix}.json'
        output.parent.mkdir(exist_ok=True)
        output.write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    unittest.main(verbosity=2)
