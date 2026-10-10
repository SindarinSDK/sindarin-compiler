import contextlib
import io
import json
import os
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import run_rust_tests as harness


class FixtureInventoryTests(unittest.TestCase):
    def setUp(self):
        self.previous_directory = Path.cwd()
        os.chdir(ROOT)
        self.addCleanup(os.chdir, self.previous_directory)
        self.gates = json.loads((ROOT / 'scripts/ci/gates.json').read_text())['gates']

    def discover(self, suite, count, filter_pattern=None):
        discovered = []

        def record(info):
            discovered.append(info)
            return {'test_name': info['test_name'], 'status': 'pass', 'reason': '',
                    'details': None, 'elapsed': 0.0}

        # Exercise the runner's real catalog, raw manifests and count guard;
        # discovery must never invoke a compiler in a CI helper test.
        with contextlib.redirect_stdout(io.StringIO()) as output:
            with harness.TestRunner('unused-in-discovery', required_count=count,
                                    filter_pattern=filter_pattern) as runner:
                with patch.object(runner, '_run_single_test', side_effect=record):
                    passed, _ = runner.run_sn_tests(suite)
        return passed, discovered, output.getvalue()

    def test_every_strict_fixture_gate_matches_the_real_catalog(self):
        checked = 0
        for gate in self.gates:
            command = gate['command']
            if 'scripts/run_rust_tests.py' not in command or '--require-count' not in command:
                continue
            suite = command[command.index('scripts/run_rust_tests.py') + 1]
            count = int(command[command.index('--require-count') + 1])
            filter_flag = next((flag for flag in ('--filter', '-f') if flag in command), None)
            filter_pattern = command[command.index(filter_flag) + 1] if filter_flag else None
            with self.subTest(gate=gate['id'], suite=suite):
                passed, fixtures, output = self.discover(suite, count, filter_pattern)
                self.assertTrue(passed, output)
                self.assertEqual(len(fixtures), count)
                paths = [info['test_file'] for info in fixtures]
                self.assertEqual(len(set(paths)), count, 'duplicate fixture identities')
            checked += 1
        self.assertGreater(checked, 0)

    def test_added_native_fixture_is_rejected_before_execution(self):
        gate = next(g for g in self.gates if 'rust-native-extra' in g['command'])
        command = gate['command']
        count = int(command[command.index('--require-count') + 1])
        original_glob = harness.glob.glob

        def added_fixture(pattern, **options):
            fixtures = original_glob(pattern, **options)
            if pattern == 'tests/rust-native/scalar_*.sn':
                fixtures.append('tests/rust-native/scalar_inventory_probe.sn')
            return fixtures

        with patch.object(harness.glob, 'glob', side_effect=added_fixture):
            passed, fixtures, output = self.discover('rust-native-extra', count)
        self.assertFalse(passed)
        self.assertEqual(fixtures, [])
        self.assertIn(f'required {count} fixtures, found {count + 1}', output)


if __name__ == '__main__':
    unittest.main()
