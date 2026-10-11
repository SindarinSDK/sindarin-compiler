"""Execute actual platform dependency staging without requiring cross compilers."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class JsonDependencyStaging(unittest.TestCase):
    def test_json_archive_is_staged_by_every_platform_branch(self):
        source = (ROOT / 'CMakeLists.txt').read_text()
        block = source.split('    # Copy libraries based on platform\n', 1)[1]
        block = block.split('    message(STATUS "  Dependencies copied to:', 1)[0]
        for platform in ('Linux', 'Darwin', 'Windows'):
            with self.subTest(platform=platform), tempfile.TemporaryDirectory(prefix='sn dependency staging ') as folder:
                root = Path(folder)
                installed = root / 'installed/lib'
                installed.mkdir(parents=True)
                archive = installed / 'libjson-c.a'
                archive.write_bytes(b'!<arch>\njson-c archive fixture\n')
                staged = root / 'bundle/deps'
                script = root / 'stage.cmake'
                script.write_text(
                    f'set(SINDARIN_LIBS_DIR [==[{installed.parent.as_posix()}]==])\n'
                    f'set(SN_DEPS_DIR [==[{staged.as_posix()}]==])\n'
                    f'set(WIN32 {"TRUE" if platform == "Windows" else "FALSE"})\n'
                    f'set(APPLE {"TRUE" if platform == "Darwin" else "FALSE"})\n' + block)
                result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stdout.decode() + result.stderr.decode())
                self.assertEqual((staged / 'lib/libjson-c.a').read_bytes(), archive.read_bytes())


if __name__ == '__main__':
    unittest.main()
