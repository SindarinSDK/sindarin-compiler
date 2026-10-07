"""Pin the two real output producers in the provenance ownership audit."""
from pathlib import Path
import subprocess
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
from check_rust_interface_lifetimes import audit_passed


class InterfaceLifetimeStreamTests(unittest.TestCase):
    def result(self, stdout, stderr=b'metadata_entries:0\n', status=0):
        return subprocess.CompletedProcess([], status, stdout, stderr)

    def test_windows_crt_stdout_and_rust_stderr(self):
        self.assertTrue(audit_passed(self.result(b'true\r\n'), windows=True))

    def test_unix_stdout_and_rust_stderr(self):
        self.assertTrue(audit_passed(self.result(b'true\n'), windows=False))

    def test_metadata_leak_or_failed_exit_still_fails(self):
        for windows, stdout in [(True, b'true\r\n'), (False, b'true\n')]:
            self.assertFalse(audit_passed(self.result(stdout, b'metadata_entries:1\n'), windows))
            self.assertFalse(audit_passed(self.result(stdout, status=1), windows))

    def test_wrong_stream_bytes_are_not_normalized(self):
        self.assertFalse(audit_passed(self.result(b'true\n'), windows=True))
        self.assertFalse(audit_passed(self.result(b'true\r\n', b'metadata_entries:0\r\n'), windows=True))
        self.assertFalse(audit_passed(self.result(b'true\n', b'warning\nmetadata_entries:0\n'), windows=False))


if __name__ == '__main__':
    unittest.main()
