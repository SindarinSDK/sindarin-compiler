#!/usr/bin/env python3
"""Native package thread calls preserve logical provider identity and joins."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import callable_methods as fixtures


class NativeThreadTests(unittest.TestCase):
    write = fixtures.CallableMethodTests.write
    binding = staticmethod(fixtures.CallableMethodTests.binding)

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='sn-package-native-threads-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.package = self.root / '.sn/methods'
        self.write('.sn/methods/src/api.sn',
            '@alias "second"\nnative fn first(): int\n'
            '@alias "third"\nnative fn second(): int\n'
            '@alias "shared"\nnative fn left(): int\n'
            '@alias "shared"\nnative fn right(): int\n'
            'native fn plain(): int\n'
            '@alias "readTouches"\nnative fn touch(value: int): void\n'
            '@alias "original_read"\nnative fn readTouches(): int\n'
            'struct Holder =>\n  callback: fn(): int\n'
            'fn exercise(): int =>\n'
            '  var a: int = & first()\n  var b: int = & second()\n'
            '  var c: int = & left()\n  var d: int = & right()\n'
            '  var e: int = & plain()\n'
            '  println(a!)\n  println(b!)\n  println(c!)\n  println(d!)\n  println(e!)\n'
            '  var f: Holder = Holder {callback: first}\n'
            '  var g: Holder = Holder {callback: second}\n'
            '  var h: Holder = Holder {callback: left}\n'
            '  var i: Holder = Holder {callback: right}\n'
            '  println(f.callback())\n  println(g.callback())\n'
            '  println(h.callback())\n  println(i.callback())\n'
            '  & touch(5) !\n  return readTouches()\n')
        self.write('.sn/methods/src/provider.c',
            'static long long touches;\n'
            'long long provider_first(void) { return 11; }\n'
            'long long provider_second(void) { return 22; }\n'
            'long long provider_left(void) { return 33; }\n'
            'long long provider_right(void) { return 44; }\n'
            'long long provider_plain(void) { return 55; }\n'
            'void provider_touch(long long value) { touches += value; }\n'
            'long long provider_read(void) { return touches; }\n')
        self.manifest = ('name: methods\nruntime: C\nnative:\n  abi: 1.7\n'
            '  declarations: [src/api.sn]\n  builds:\n'
            '    - name: storage\n      language: C\n      sources: [src/provider.c]\n'
            '    - name: facade\n      language: SN\n      entry: src/api.sn\n      sources: [src/api.sn]\n'
            '  bindings:\n')
        for name in ('first', 'second', 'left', 'right', 'plain'):
            self.manifest += self.binding(name, 'provider_'+name, 'storage', 'value', '')
        self.manifest += self.binding('touch', 'provider_touch', 'storage', 'value', 'value: value')
        self.manifest += self.binding('readTouches', 'provider_read', 'storage', 'value', '')
        self.manifest += self.binding('exercise', 'exercise', 'facade', 'value', '')
        self.write('main.sn', 'import "methods/src/api"\nfn main(): void =>\n  println(exercise())\n')

    def test_thread_and_function_wrappers_select_declared_providers_in_all_modes(self):
        source = self.package / 'src/provider.c'
        frozen = {path: path.read_bytes() for path in (source, self.package/'src/api.sn', self.root/'main.sn')}
        cases = []
        for runtime in ('C', 'RS'):
            manifest = self.manifest.replace('runtime: C', 'runtime: '+runtime)
            self.write('.sn/methods/sn.yaml', manifest)
            built = subprocess.run([str(fixtures.COMPILER), '--build-native', str(self.package/'sn.yaml'),
                '-o', str(self.package/('built-'+runtime))], cwd=self.root, capture_output=True, timeout=240)
            self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
            assembly = Path(json.loads(built.stdout)['assembly'])
            for sealed in (False, True):
                if sealed:
                    destination = self.package / ('sealed-'+runtime)
                    shutil.copytree(assembly.parent, destination)
                    descriptor = destination / assembly.name
                    digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                    self.write('.sn/methods/sn.yaml', manifest.replace('native:\n',
                        f'native:\n  assembly: {{path: sealed-{runtime}/{assembly.name}, sha256: {digest}}}\n'))
                    source.unlink()
                for target in ('c', 'rust'):
                    for optimization in ('-O0', '-O1', '-O2'):
                        for mode in ('default', 'checked', 'unchecked'):
                            with self.subTest(runtime=runtime, sealed=sealed, target=target,
                                              optimization=optimization, mode=mode):
                                executable = self.root / 'consumer.exe'
                                built = subprocess.run([str(fixtures.COMPILER), 'main.sn', '--no-install',
                                    '--target', target, optimization,
                                    *([] if mode == 'default' else ['--'+mode]), '-o', str(executable)],
                                    cwd=self.root, capture_output=True, timeout=180)
                                self.assertEqual(built.returncode, 0, built.stderr.decode(errors='replace'))
                                result = subprocess.run([str(executable)], cwd=self.root, capture_output=True, timeout=20)
                                expected = b'11\n22\n33\n44\n55\n11\n22\n33\n44\n5\n'
                                if os.name == 'nt':
                                    expected = expected.replace(b'\n', b'\r\n')
                                self.assertEqual((result.returncode, result.stdout, result.stderr), (0, expected, b''))
                                cases.append({'runtime': runtime, 'sealed': sealed, 'target': target,
                                    'optimization': optimization, 'mode': mode, 'compile_status': built.returncode,
                                    'run_status': result.returncode, 'stdout_hex': result.stdout.hex(),
                                    'stderr_hex': result.stderr.hex()})
                if sealed:
                    source.write_bytes(frozen[source])
            for path, data in frozen.items():
                self.assertEqual(path.read_bytes(), data)
        self.assertEqual(len(cases), 72)
        report = {'passed': True, 'cases': cases,
            'compiler_sha256': hashlib.sha256(fixtures.COMPILER.read_bytes()).hexdigest(),
            'fixture_sha256': {str(path.relative_to(self.root)): hashlib.sha256(data).hexdigest()
                               for path, data in frozen.items()},
            'environment': {name: os.environ.get(name) for name in
                            ('SN_CC', 'SN_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
        suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
        output = fixtures.ROOT/'.sn'/f'package-native-threads{suffix}.json'
        output.parent.mkdir(exist_ok=True)
        output.write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    unittest.main(verbosity=2)
