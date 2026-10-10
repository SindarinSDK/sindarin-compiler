import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import check_compound_comparison_oracles as checker


class CompoundComparisonOracleTests(unittest.TestCase):
    def report(self, windows=False):
        raw = (ROOT / checker.RAW).with_suffix('.expected').read_text().encode()
        strict = {
            ('default', '-O0'): b'true\n3\nfalse\n3\ntrue\n3.50000\n',
            ('default', '-O1'): b'true\n3\nfalse\n3\ntrue\n3.50000\n',
            ('default', '-O2'): b'true\n1\ntrue\n1\ntrue\n1.00000\n',
            ('checked', '-O0'): b'true\n3\nfalse\n3\ntrue\n3.50000\n',
            ('checked', '-O1'): b'true\n3\nfalse\n3\ntrue\n3.50000\n',
            ('checked', '-O2'): b'true\n3\nfalse\n3\ntrue\n3.50000\n',
            ('unchecked', '-O0'): b'true\n1\ntrue\n1\ntrue\n1.00000\n',
            ('unchecked', '-O1'): b'true\n1\ntrue\n1\ntrue\n1.00000\n',
            ('unchecked', '-O2'): b'true\n1\ntrue\n1\ntrue\n1.00000\n',
        }
        cases = []
        for source in checker.SOURCES:
            for (mode, optimization), strict_output in strict.items():
                if source == checker.RAW:
                    output = raw
                elif source == checker.INTEGRAL:
                    output = (ROOT / source).with_suffix('.expected').read_text().encode()
                elif source == checker.INTEGRAL_STRICT:
                    output = (b'true\n1\nfalse\n0\n' if mode == 'unchecked' or
                              (mode == 'default' and optimization == '-O2') else
                              b'true\n3\nfalse\n3\n')
                else:
                    output = strict_output
                if windows:
                    output = output.replace(b'\n', b'\r\n')
                result = {'compile': {'status': 0}, 'run': {
                    'status': 0, 'stdout_hex': output.hex(), 'stderr_hex': ''}}
                cases.append({'source': source.replace('/', '\\') if windows else source,
                    'source_sha256': hashlib.sha256((ROOT / source).read_bytes()).hexdigest(),
                    'arithmetic_mode': mode, 'optimization': optimization,
                    'targets': {'c': copy.deepcopy(result), 'rust': copy.deepcopy(result)}})
        # Exercise actual JSON transport rather than relying on Python literals
        # to interpret the backslashes in Windows report identifiers.
        return json.loads(json.dumps({'passed': True, 'cases': cases}))

    def verify(self, report, windows=False):
        with contextlib.redirect_stdout(io.StringIO()):
            checker.verify(report, windows=windows)

    def test_windows_paths_and_exact_crlf_transport(self):
        self.verify(self.report(windows=True), windows=True)
        self.verify(self.report())

    def test_windows_streams_are_not_newline_normalized(self):
        report = self.report(windows=True)
        run = report['cases'][0]['targets']['c']['run']
        run['stdout_hex'] = bytes.fromhex(run['stdout_hex']).replace(b'\r\n', b'\n').hex()
        with self.assertRaisesRegex(ValueError, 'oracle failed'):
            self.verify(report, windows=True)

    def test_missing_case_and_separator_duplicate_are_rejected(self):
        report = self.report(windows=True)
        report['cases'].pop()
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            self.verify(report, windows=True)
        report = self.report(windows=True)
        duplicate = copy.deepcopy(report['cases'][0])
        duplicate['source'] = duplicate['source'].replace('\\', '/')
        report['cases'].append(duplicate)
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.verify(report, windows=True)

    def test_normalized_paths_still_require_unchanged_source(self):
        report = self.report(windows=True)
        report['cases'][0]['source_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'source changed'):
            self.verify(report, windows=True)

    def test_failed_compile_and_stderr_do_not_count_as_parity(self):
        for field in ('compile', 'stderr'):
            with self.subTest(field=field):
                report = self.report(windows=True)
                result = report['cases'][0]['targets']['rust']
                if field == 'compile':
                    result['compile']['status'] = 1
                else:
                    result['run']['stderr_hex'] = b'failure'.hex()
                with self.assertRaisesRegex(ValueError, 'oracle failed'):
                    self.verify(report, windows=True)


if __name__ == '__main__':
    unittest.main()
