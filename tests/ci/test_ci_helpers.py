import contextlib
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import re
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts' / 'ci'))
import build_bundle
import collect_reports
import run_gates


@contextlib.contextmanager
def directory(path):
    previous = Path.cwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(previous)


class BuildBundleTests(unittest.TestCase):
    def test_package_fixture_accepts_canonical_assembly_paths_from_an_aliased_temp_root(self):
        sys.path.insert(0, str(ROOT / 'tests' / 'package'))
        import callable_methods
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            (root / 'alias').mkdir()
            fixture = callable_methods.CallableMethodTests()
            self.addCleanup(fixture.doCleanups)
            temporary = SimpleNamespace(name=str(root / 'alias' / '..'), cleanup=lambda: None)
            with patch.object(callable_methods.tempfile, 'TemporaryDirectory', return_value=temporary):
                fixture.setUp()
            assembly = root / '.sn/methods/built/assembly.json'
            self.assertEqual(assembly.relative_to(fixture.package), Path('built/assembly.json'))

    def test_restores_executables_dependencies_and_checksums(self):
        with tempfile.TemporaryDirectory() as folder, directory(folder):
            Path('bin').mkdir()
            compiler = Path('bin/sn.exe' if platform.system() == 'Windows' else 'bin/sn')
            compiler.write_bytes(b'compiler under test')
            compiler.chmod(0o755)
            deps = Path('.sn/sindarin-pkg-libs/libs/platform/include')
            deps.mkdir(parents=True)
            (deps / 'library.h').write_text('required header')
            git = Path('.sn/sindarin-pkg-libs/.git')
            git.mkdir()
            (git / 'config').write_text('not a build dependency')
            with patch.object(build_bundle.subprocess, 'check_output', return_value='revision\n'):
                build_bundle.pack(Path('bundle.tar.gz'))
                shutil.rmtree('bin')
                shutil.rmtree('.sn/sindarin-pkg-libs')
                build_bundle.restore(Path('bundle.tar.gz'))
                self.assertEqual(compiler.read_bytes(), b'compiler under test')
                self.assertTrue(compiler.stat().st_mode & 0o100)
                self.assertEqual((deps / 'library.h').read_text(), 'required header')
                self.assertFalse(git.exists())
                compiler.write_bytes(b'wrong compiler')
                with self.assertRaisesRegex(ValueError, 'artifact changed'):
                    build_bundle.verify()

    def test_rejects_artifacts_from_another_revision(self):
        with tempfile.TemporaryDirectory() as folder, directory(folder):
            Path('bin').mkdir()
            Path('bin/sn.exe' if platform.system() == 'Windows' else 'bin/sn').write_bytes(b'compiler')
            with patch.object(build_bundle.subprocess, 'check_output', return_value='old\n'):
                build_bundle.pack(Path('bundle.tar.gz'))
            with patch.object(build_bundle.subprocess, 'check_output', return_value='new\n'):
                with self.assertRaisesRegex(ValueError, 'checkout/platform'):
                    build_bundle.restore(Path('bundle.tar.gz'))


class CoverageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.incoming = Path(self.temporary.name) / 'incoming'
        self.output = Path(self.temporary.name) / 'complete'
        self.catalog = json.loads(run_gates.CATALOG.read_text())
        self.reports = []
        for runner, system in collect_reports.SYSTEMS.items():
            for group in self.catalog['groups']:
                gates = run_gates.gates_for(self.catalog, group, system)
                if not gates:
                    continue
                root = self.incoming / f'ci-evidence-{runner}-{group}' / 'ci-reports' / group
                root.mkdir(parents=True)
                report = {'group': group, 'platform': system, 'revision': 'revision',
                          'compiler_sha256': system,
                          'catalog_sha256': hashlib.sha256(run_gates.CATALOG.read_bytes()).hexdigest(),
                          'passed': True, 'gates': [{'id': gate['id'], 'status': 0} for gate in gates]}
                path = root / 'results.json'
                path.write_text(json.dumps(report))
                self.reports.append(path)

    def collect(self):
        with patch.object(collect_reports.subprocess, 'check_output', return_value='revision\n'):
            collect_reports.collect(self.incoming, self.output)

    def test_complete_matrix_and_historical_gate_inventory(self):
        runtime = {g['id'] for g in self.catalog['gates'] if g['id'].startswith('runtime-')}
        self.assertTrue({f'runtime-{i:03d}' for i in range(1, 99)} <= runtime)
        self.collect()
        report = json.loads((self.output / 'summary.json').read_text())
        self.assertEqual(report['groups'], 41)
        self.assertEqual(report['gates'], 540)
        capture = [g for g in self.catalog['gates'] if g['id'] == 'integer-match-evidence-capture']
        self.assertEqual(len(capture), 1)
        self.assertEqual(capture[0]['command'], ['{python}', 'tests/package/integer_match_capture.py'])
        self.assertEqual(capture[0]['group'], 'native')
        self.assertFalse(capture[0]['linux_only'])
        for sanitized in (False, True):
            identity = 'numeric-match-conversions' + ('-sanitizers' if sanitized else '')
            entries = [g for g in self.catalog['gates'] if g['id'] == identity]
            self.assertEqual(len(entries), 1)
            self.assertEqual(entries[0]['command'], ['{python}', 'scripts/check_rust_numeric_match.py'] +
                             (['--sanitize'] if sanitized else []))
            self.assertEqual(entries[0]['group'], 'sanitizers' if sanitized else 'core')
            self.assertEqual(entries[0]['linux_only'], sanitized)
        for stem, script in (
                ('string-match-effects', 'scripts/check_rust_string_match_effects.py'),
                ('method-struct-parameters', 'scripts/check_rust_method_struct_parameters.py'),
                ('package-method-struct-parameters', 'tests/package/method_struct_parameters.py'),
                ('package-borrowed-scalar-memory', 'tests/package/borrowed_scalar_memory.py'),
                ('default-array-parameters', 'scripts/check_rust_default_array_parameters.py'),
                ('package-default-array-parameters', 'tests/package/default_array_parameters.py'),
                ('package-string-match-effects', 'tests/package/string_match_effects.py'),
                ('array-call-arguments', 'scripts/check_c_array_call_arguments.py'),
                ('string-call-arguments', 'scripts/check_rust_string_call_arguments.py')):
            for sanitized in (False, True):
                identity = stem + ('-sanitizers' if sanitized else '')
                entries = [g for g in self.catalog['gates'] if g['id'] == identity]
                self.assertEqual(len(entries), 1, identity)
                self.assertEqual(entries[0]['command'], ['{python}', script])
                self.assertEqual(entries[0]['group'], 'sanitizers' if sanitized else 'native')
                self.assertEqual(entries[0]['linux_only'], sanitized)
                if sanitized:
                    self.assertIn('-fsanitize=address,undefined', entries[0]['env']['SN_CFLAGS'])
        for sanitized in (False, True):
            identity = 'package-scalar-value-qualifiers' + ('-sanitizers' if sanitized else '')
            matches = [gate for gate in self.catalog['gates'] if gate['id'] == identity]
            self.assertEqual(len(matches), 1, identity)
            gate = matches[0]
            self.assertEqual(gate['command'], ['{python}', 'tests/package/scalar_value_qualifiers.py'])
            self.assertEqual(gate['group'], 'sanitizers' if sanitized else 'providers')
            self.assertEqual(gate['linux_only'], sanitized)
            if sanitized:
                self.assertIn('-fsanitize=address,undefined', gate['env']['SN_CFLAGS'])

        for sanitized in (False, True):
            identity = 'value-record-stores' + ('-sanitizers' if sanitized else '')
            matches = [gate for gate in self.catalog['gates'] if gate['id'] == identity]
            self.assertEqual(len(matches), 1, identity)
            gate = matches[0]
            self.assertEqual(gate['command'], ['{python}', 'scripts/check_rust_value_record_stores.py'] +
                             (['--sanitize'] if sanitized else []))
            self.assertEqual(gate['group'], 'sanitizers' if sanitized else 'core')
            self.assertEqual(gate['linux_only'], sanitized)
        for sanitized in (False, True):
            identity = 'package-native-threads' + ('-sanitizers' if sanitized else '')
            matches = [gate for gate in self.catalog['gates'] if gate['id'] == identity]
            self.assertEqual(len(matches), 1, identity)
            gate = matches[0]
            self.assertEqual(gate['command'], ['{python}', 'tests/package/native_threads.py'])
            self.assertEqual(gate['group'], 'sanitizers' if sanitized else 'providers')
            self.assertEqual(gate['linux_only'], sanitized)
            if sanitized:
                self.assertIn('-fsanitize=address,undefined', gate['env']['SN_CFLAGS'])
        for name in ('artifacts', 'native'):
            for sanitized in (False, True):
                identity = 'sdk-random-' + name + ('-sanitizers' if sanitized else '')
                matches = [gate for gate in self.catalog['gates'] if gate['id'] == identity]
                self.assertEqual(len(matches), 1, identity)
                gate = matches[0]
                self.assertEqual(gate['group'], 'sanitizers' if sanitized else
                                 'sdk-native' if name == 'native' else 'sdk-artifacts')
                self.assertEqual(gate['linux_only'], sanitized)
                expected = (['{python}', 'scripts/check_sdk_random_artifact.py'] if name == 'artifacts'
                            else ['{python}', 'scripts/check_sdk_native.py', 'random'])
                if sanitized and name == 'native':
                    expected.append('--sanitize')
                self.assertEqual(gate['command'], expected)
                if sanitized:
                    self.assertIn('-fsanitize=address,undefined', gate['env']['SN_CFLAGS'])
        entries = [g for g in self.catalog['gates'] if g['id'] == 'native-entrypoints']
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0]['group'], 'native')
        self.assertFalse(entries[0]['linux_only'])
        namespaces = [g for g in self.catalog['gates'] if g['id'] == 'native-static-namespaces']
        self.assertEqual(len(namespaces), 1)
        self.assertEqual(namespaces[0]['group'], 'native')
        self.assertFalse(namespaces[0]['linux_only'])
        methods = [g for g in self.catalog['gates'] if g['id'] == 'static-instance-methods']
        self.assertEqual(len(methods), 1)
        self.assertEqual(methods[0]['group'], 'native')
        self.assertFalse(methods[0]['linux_only'])
        providers = [g for g in self.catalog['gates'] if g['id'] == 'typed-native-provider-sanitizers']
        self.assertEqual(len(providers), 1)
        self.assertEqual(providers[0]['group'], 'sanitizers')
        self.assertTrue(providers[0]['linux_only'])
        callables = [g for g in self.catalog['gates'] if g['id'] == 'package-callable-methods']
        self.assertEqual(len(callables), 1)
        self.assertEqual(callables[0]['group'], 'providers')
        self.assertFalse(callables[0]['linux_only'])
        sanitized = [g for g in self.catalog['gates'] if g['id'] == 'package-callable-method-sanitizers']
        self.assertEqual(len(sanitized), 1)
        self.assertEqual(sanitized[0]['group'], 'sanitizers')
        self.assertTrue(sanitized[0]['linux_only'])
        string_results = [g for g in self.catalog['gates'] if g['id'] == 'native-string-array-results']
        self.assertEqual(len(string_results), 1)
        self.assertEqual(string_results[0]['group'], 'native')
        self.assertFalse(string_results[0]['linux_only'])
        string_sanitizers = [g for g in self.catalog['gates'] if g['id'] == 'native-string-array-result-sanitizers']
        self.assertEqual(len(string_sanitizers), 1)
        self.assertEqual(string_sanitizers[0]['group'], 'sanitizers')
        self.assertTrue(string_sanitizers[0]['linux_only'])
        opaque = [g for g in self.catalog['gates'] if g['id'] == 'package-opaque-results']
        self.assertEqual(len(opaque), 1)
        self.assertEqual(opaque[0]['group'], 'providers')
        self.assertFalse(opaque[0]['linux_only'])
        failures = [g for g in self.catalog['gates'] if g['id'] == 'package-opaque-result-sanitizers']
        self.assertEqual(len(failures), 1)
        self.assertEqual(failures[0]['group'], 'sanitizers')
        self.assertTrue(failures[0]['linux_only'])
        sdk = [g for g in self.catalog['gates'] if g['id'] == 'sdk-artifact-callers']
        self.assertEqual(len(sdk), 1)
        self.assertEqual(sdk[0]['group'], 'sdk-corpus')
        self.assertFalse(sdk[0]['linux_only'])

        spec = importlib.util.spec_from_file_location('sdk_native_dispatch', ROOT / 'scripts/check_sdk_native.py')
        dispatcher = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(dispatcher)
        for gate in self.catalog['gates']:
            command = gate['command']
            if command[:2] == ['{python}', 'scripts/check_sdk_native.py']:
                self.assertIn(command[2], dispatcher.CHECKS, gate['id'])

        for ident in ('sdk-time-date-artifacts', 'sdk-time-date-native', 'sdk-dtls-lifecycle',
                      'canonical-record-copy', 'sdk-record-copies',
                      'sdk-stringbuilder-artifacts', 'sdk-stringbuilder-native',
                      'sdk-process-artifacts', 'sdk-process-native',
                      'sdk-json-artifacts', 'sdk-json-native', 'native-record-array-packages', 'sdk-xml-native', 'package-internal-native-calls', 'sdk-yaml-artifacts', 'sdk-yaml-native', 'sdk-udp-artifacts', 'sdk-udp-native', 'sdk-zlib-artifacts', 'sdk-tcp-artifacts', 'sdk-tcp-native'):
            entry = [g for g in self.catalog['gates'] if g['id'] == ident]
            self.assertEqual(len(entry), 1)
            self.assertEqual(entry[0]['group'],
                             'providers' if ident in ('canonical-record-copy', 'native-record-array-packages', 'package-internal-native-calls')
                             else 'sdk-native' if ident.endswith('-native') or ident == 'sdk-dtls-lifecycle'
                             else 'sdk-artifacts')
            self.assertFalse(entry[0]['linux_only'])
            sanitized = [g for g in self.catalog['gates'] if g['id'] == ident + '-sanitizers']
            self.assertEqual(len(sanitized), 1)
            self.assertEqual(sanitized[0]['group'], 'sanitizers')
            self.assertTrue(sanitized[0]['linux_only'])

    def test_dynamic_scalar_match_runs_on_every_platform_and_with_sanitizers(self):
        for sanitized in (False, True):
            identity = 'dynamic-scalar-match' + ('-sanitizers' if sanitized else '')
            entries = [g for g in self.catalog['gates'] if g['id'] == identity]
            self.assertEqual(len(entries), 1)
            self.assertEqual(entries[0]['command'],
                             ['{python}', 'scripts/check_rust_dynamic_match.py'] +
                             (['--sanitize'] if sanitized else []))
            self.assertEqual(entries[0]['group'], 'sanitizers' if sanitized else 'core')
            self.assertEqual(entries[0]['linux_only'], sanitized)

    def test_workflow_runs_every_catalog_group_with_its_required_tools_and_reports(self):
        workflow = (ROOT / '.github/workflows/ci.yml').read_text()
        matrix = workflow.split('        group:\n', 1)[1].split('        include:', 1)[0]
        configured = re.findall(r'^        - ([a-z-]+)$', matrix, re.M)
        self.assertEqual(configured, self.catalog['groups'])
        self.assertEqual(len(configured), len(set(configured)))

        def configured_groups(step):
            block = workflow.split('    - name: ' + step + '\n', 1)[1].split('      uses:', 1)[0]
            encoded = re.search(r"fromJSON\('([^']+)'\)", block).group(1)
            return set(json.loads(encoded))

        self.assertTrue({'packages', 'providers', 'sdk-native', 'sanitizers'} <=
                        configured_groups('Setup Go for shared runtime ABI clients'))
        required_sdk = {gate['group'] for gate in self.catalog['gates']
                        if gate['id'].startswith('sdk-') or
                        gate['id'] in ('runtime-105', 'runtime-108', 'runtime-110')}
        self.assertTrue(required_sdk <= configured_groups('Checkout SDK native resource integration'))
        self.assertIn('          .sn/runtime-abi*.json\n', workflow)
        self.assertIn('          .sn/sdk-artifact-validation.json\n', workflow)
        self.assertIn('          .sn/sdk-udp-artifact*.json\n', workflow)
        self.assertIn('          .sn/sdk-native-integration/.sn/udp-*.json\n', workflow)
        self.assertIn('          .sn/sdk-tcp-artifact*.json\n', workflow)
        self.assertIn('          .sn/sdk-native-integration/.sn/tcp-*.json\n', workflow)
        self.assertIn('ref: 9d2e6e792746ce22d4ee3e744bc3f9938b59b104', workflow)
        self.assertIn('github.event.pull_request.number || github.run_id', workflow)

    def test_consumer_libraries_retain_platform_resolution_and_exact_linker_options(self):
        sys.path.insert(0, str(ROOT / 'scripts'))
        from prepare_native_imports import consumer_link_options
        self.assertEqual(consumer_link_options([
            '-lz', '-l', 'xml2', '-l:exact.a', '/a path/archive.a',
            '-framework', 'Security', '-L', '/a path', '-Wl,--whole-archive']),
            ['z', 'xml2', '-l:exact.a', '/a path/archive.a',
             '-Wl,-framework,Security', '-L/a path', '-Wl,--whole-archive'])
        with self.assertRaisesRegex(ValueError, 'missing its argument'):
            consumer_link_options(['-l'])

    def test_missing_group_fails(self):
        self.reports[-1].unlink()
        with self.assertRaises(FileNotFoundError):
            self.collect()

    def test_missing_duplicate_or_failed_gate_fails(self):
        path = self.reports[0]
        original = json.loads(path.read_text())
        for variant in ('missing', 'duplicate', 'failed'):
            report = json.loads(json.dumps(original))
            if variant == 'missing':
                report['gates'].pop()
            elif variant == 'duplicate':
                report['gates'].append(report['gates'][0])
            else:
                report['gates'][0]['status'] = 1
            path.write_text(json.dumps(report))
            with self.assertRaises(ValueError):
                self.collect()

    def test_mixed_compiler_binaries_fail(self):
        path = self.reports[1]
        report = json.loads(path.read_text())
        report['compiler_sha256'] = 'different compiler'
        path.write_text(json.dumps(report))
        with self.assertRaisesRegex(ValueError, 'different compilers'):
            self.collect()


class ParityPromotionTests(unittest.TestCase):
    def test_changed_historical_source_or_oracle_is_rejected(self):
        sys.path.insert(0, str(ROOT / 'scripts'))
        import run_rust_tests
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); source=root/'case.sn'; oracle=root/'case.expected'; marker=root/'case.parity.json'
            source.write_bytes(b'original source\n'); oracle.write_bytes(b'historical rejection\n')
            contract={'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                      'legacy_oracle_sha256':hashlib.sha256(oracle.read_bytes()).hexdigest(),
                      'stdout_hex':'', 'exit':0}
            marker.write_text(json.dumps(contract))
            runner=run_rust_tests.TestRunner('unused')
            for changed in (source,oracle):
                original=changed.read_bytes(); changed.write_bytes(b'changed\n')
                with patch.object(run_rust_tests,'run_with_timeout') as compile_call:
                    result=runner._run_promoted_rejection(str(source),str(oracle),str(root/'out.rs'),marker)
                    self.assertEqual(result[0],'fail'); compile_call.assert_not_called()
                changed.write_bytes(original)

    def test_c_rejection_does_not_count_as_promoted_parity(self):
        sys.path.insert(0, str(ROOT / 'scripts'))
        import run_rust_tests
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); source=root/'case.sn'; oracle=root/'case.expected'; marker=root/'case.parity.json'
            source.write_bytes(b'original source\n'); oracle.write_bytes(b'historical rejection\n')
            marker.write_text(json.dumps({'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                'legacy_oracle_sha256':hashlib.sha256(oracle.read_bytes()).hexdigest(),'stdout_hex':'','exit':0}))
            runner=run_rust_tests.TestRunner('unused')
            with patch.object(run_rust_tests,'run_with_timeout',return_value=(1,'','C rejects this','')):
                result=runner._run_promoted_rejection(str(source),str(oracle),str(root/'out.rs'),marker)
            self.assertEqual(result[0],'fail')


if __name__ == '__main__':
    unittest.main()
