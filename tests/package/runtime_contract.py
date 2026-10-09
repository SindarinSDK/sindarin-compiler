#!/usr/bin/env python3
"""Exercise package runtime selection through the actual compiler and linker."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('SN_COMPILER', ROOT / 'bin' /
                              ('sn.exe' if os.name == 'nt' else 'sn'))).resolve()


class PackageRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='sn-package-runtime-')
        self.addCleanup(self.temporary.cleanup)
        self.project = Path(self.temporary.name)
        self.write('sn.yaml', 'name: application\nruntime: C\n')
        self.write('src/main.sn', 'import "dep/src/value"\n\n'
                   'fn main(): void =>\n  println(package_value() + native_value())\n')
        self.write('.sn/dep/src/value.sn', '@source "value.c"\n\n'
                   'native fn native_value(): int\n\n'
                   'fn package_value(): int =>\n  return 40\n')
        self.write('.sn/dep/src/value.c', 'long long native_value(void) { return 42; }\n')
        self.manifest(None)

    def write(self, path, text):
        dest = self.project / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(text)

    def manifest(self, runtime):
        self.write('.sn/dep/sn.yaml', 'name: dep\nversion: 1.0.0\n' +
                   (f'runtime: {runtime}\n' if runtime else ''))

    def compile(self, target, mode=None, optimization='-O0'):
        product = self.project / ('result.exe' if os.name == 'nt' else 'result')
        product.unlink(missing_ok=True)
        command = [str(COMPILER), 'src/main.sn', '--no-install', '--target', target,
                   optimization, '-o', str(product)]
        if mode:
            command.append(mode)
        result = subprocess.run(command, cwd=self.project, capture_output=True, timeout=120)
        return result, product

    def assert_runs(self, target):
        build, product = self.compile(target)
        self.assertEqual(build.returncode, 0, build.stderr.decode(errors='replace'))
        run = subprocess.run([str(product)], capture_output=True, timeout=10)
        self.assertEqual(run.returncode, 0, run.stderr.decode(errors='replace'))
        self.assertEqual(run.stdout, b'82\r\n' if os.name == 'nt' else b'82\n')

    def assert_rejected(self, target, diagnostic, mode=None, optimization='-O0'):
        build, product = self.compile(target, mode, optimization)
        self.assertNotEqual(build.returncode, 0)
        self.assertIn(diagnostic, build.stderr.decode(errors='replace'))
        self.assertFalse(product.exists(), 'rejected package produced an output artifact')

    def test_inherited_and_matching_runtimes_keep_native_c_backing(self):
        for target, declared in (('c', None), ('rust', None), ('c', 'C'), ('rust', 'RS')):
            with self.subTest(target=target, runtime=declared):
                self.manifest(declared)
                self.assert_runs(target)

    def test_explicit_package_runtime_never_silently_follows_consumer(self):
        for target, declared in (('c', 'RS'), ('rust', 'C')):
            for mode in (None, '--emit-source', '--emit-model'):
                for optimization in ('-O0', '-O2'):
                    with self.subTest(target=target, runtime=declared, mode=mode, opt=optimization):
                        self.manifest(declared)
                        self.assert_rejected(target, 'cross-runtime package artifacts', mode, optimization)

    def test_go_package_reports_unimplemented_backend_and_bridge(self):
        self.manifest('GO')
        for target in ('c', 'rust'):
            for mode in (None, '--emit-source', '--emit-model'):
                with self.subTest(target=target, mode=mode):
                    self.assert_rejected(target, 'Sindarin Go backend and Go-native package bridge', mode)

    def test_transitive_package_runtime_is_checked(self):
        self.write('src/main.sn', 'import "front/src/value"\n\n'
                   'fn main(): void =>\n  println(package_value())\n')
        self.write('.sn/front/src/value.sn', 'import "dep/src/value"\n')
        self.write('.sn/front/sn.yaml', 'name: front\n')
        self.manifest('GO')
        self.assert_rejected('rust', "package 'dep' selects runtime GO")

    def test_relative_import_uses_owning_package_manifest(self):
        self.write('src/main.sn', 'import "../.sn/dep/src/value"\n\n'
                   'fn main(): void =>\n  println(package_value())\n')
        self.manifest('C')
        self.assert_rejected('rust', "package 'dep' selects runtime C")

    def test_malformed_runtime_is_rejected_even_without_install(self):
        for invalid in ('Rust', '[]', 'C\nruntime: RS', '"C\\0extra"'):
            with self.subTest(runtime=invalid):
                self.manifest(invalid)
                self.assert_rejected('rust', 'runtime')

    def test_nested_extension_does_not_select_runtime(self):
        self.write('.sn/dep/sn.yaml', 'name: dep\nextension: {runtime: GO}\n')
        self.assert_runs('rust')

    def test_same_application_modules_do_not_override_cli_target(self):
        self.write('src/main.sn', 'import "value"\n\n'
                   'fn main(): void =>\n  println(package_value() + native_value())\n')
        self.write('src/value.sn', 'native fn native_value(): int =>\n  return 42\n\n'
                   'fn package_value(): int =>\n  return 40\n')
        self.assert_runs('rust')

    def test_namespaced_import_checks_runtime(self):
        self.write('src/main.sn', 'import "dep/src/value" as Dep\n\n'
                   'fn main(): void =>\n  println(Dep.package_value())\n')
        self.manifest('C')
        self.assert_rejected('rust', "package 'dep' selects runtime C")

    def test_manifestless_application_retains_target_selection(self):
        (self.project / 'sn.yaml').unlink()
        for target in ('c', 'rust'):
            with self.subTest(target=target):
                self.assert_runs(target)

    def test_symlink_import_uses_dependency_manifest(self):
        if os.name == 'nt':
            self.skipTest('symlink creation needs Windows host privileges; other ownership cases remain required')
        (self.project / 'src/alias.sn').symlink_to(self.project / '.sn/dep/src/value.sn')
        self.write('src/main.sn', 'import "alias"\n\n'
                   'fn main(): void =>\n  println(package_value())\n')
        self.manifest('C')
        self.assert_rejected('rust', "package 'dep' selects runtime C")


if __name__ == '__main__':
    unittest.main(verbosity=2)
