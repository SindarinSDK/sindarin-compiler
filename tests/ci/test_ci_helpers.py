import contextlib
import hashlib
import json
import os
from pathlib import Path
import platform
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
        self.assertEqual(report['groups'], 26)
        self.assertEqual(report['gates'], 287)

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


if __name__ == '__main__':
    unittest.main()
