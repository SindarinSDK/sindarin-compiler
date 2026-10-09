import http.server
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import threading
import unittest


SETUP = Path(__file__).resolve().parents[2] / 'scripts/ci/install_unix_deps.sh'
REQUIRED = ('include/json-c/json.h', 'include/git2.h', 'include/yaml.h',
            'lib/libjson-c.a', 'lib/libgit2.a', 'lib/libyaml.a', 'lib/libssh2.a',
            'lib/libssl.a', 'lib/libcrypto.a', 'lib/libpcre2-8.a',
            'lib/libhttp_parser.a', 'lib/libz.a')


@unittest.skipIf(os.name == 'nt', 'The Unix dependency installer is used only on Linux/macOS')
class UnixDependencySetupTests(unittest.TestCase):
    def run_installer(self, body, env=None):
        with tempfile.TemporaryDirectory() as directory:
            checkout = Path(directory)
            (checkout / 'scripts').mkdir()
            (checkout / 'scripts/install.sh').write_text('set -eu\n' + body)
            return subprocess.run(['bash', str(SETUP), str(checkout)],
                                  capture_output=True, timeout=30,
                                  env=os.environ | (env or {}))

    def test_installer_failure_is_not_reported_as_success(self):
        result = self.run_installer('exit 17\n')
        self.assertEqual(result.returncode, 17)

    def test_missing_build_inputs_fail_even_if_installer_returns_success(self):
        result = self.run_installer('exit 0\n')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b'Dependency installation is incomplete: include/json-c/json.h',
                      result.stderr)

    @unittest.skipUnless(shutil.which('curl'), 'Download recovery requires curl')
    def test_connection_resets_are_retried_before_validating_build_inputs(self):
        class Handler(http.server.BaseHTTPRequestHandler):
            attempts = 0

            def do_GET(self):
                Handler.attempts += 1
                if Handler.attempts < 3:
                    self.connection.shutdown(socket.SHUT_RDWR)
                    self.connection.close()
                    return
                self.send_response(200)
                self.send_header('Content-Length', '8')
                self.end_headers()
                self.wfile.write(b'complete')

            def log_message(self, *args):
                pass

        server = http.server.HTTPServer(('127.0.0.1', 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            body = 'curl -fSL "$CI_TEST_URL" -o download\n'
            body += 'test "$(cat download)" = complete\n'
            body += 'case "$(uname -s)" in Linux) platform=linux ;; Darwin) platform=darwin ;; esac\n'
            for relative in REQUIRED:
                body += f'mkdir -p "libs/$platform/{Path(relative).parent.as_posix()}"\n'
                body += f'printf x > "libs/$platform/{relative}"\n'
            result = self.run_installer(body, {'CI_TEST_URL': f'http://127.0.0.1:{server.server_port}/libs'})
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual(Handler.attempts, 3)
        finally:
            server.shutdown()
            server.server_close()
            worker.join(timeout=5)
