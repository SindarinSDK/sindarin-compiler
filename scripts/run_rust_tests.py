#!/usr/bin/env python3
"""Rust-private regression runner.

Use run_tests.py for the unchanged tagged C harness. This runner owns Rust
.opt/panic/exit/UTF-8 handling and fake-rustc lifecycle checks only.
"""

import argparse
import atexit
import glob
import json
import os
import platform
import signal
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import List, Optional, Tuple, Dict, Any

# Global reference for signal handler cleanup
_active_runner: Optional['TestRunner'] = None


def cleanup_orphaned_temp_dirs():
    """Remove any orphaned sn_test_* temp directories from previous runs."""
    temp_base = tempfile.gettempdir()
    pattern = os.path.join(temp_base, 'sn_test_*')
    orphaned = glob.glob(pattern)

    if orphaned:
        print(f"Cleaning up {len(orphaned)} orphaned test directories...")
        for path in orphaned:
            try:
                shutil.rmtree(path, ignore_errors=True)
            except Exception:
                pass  # Best effort cleanup


def _signal_handler(signum, frame):
    """Handle interrupt signals to ensure cleanup."""
    global _active_runner
    if _active_runner and _active_runner.temp_dir:
        print(f"\nInterrupted, cleaning up...")
        shutil.rmtree(_active_runner.temp_dir, ignore_errors=True)
    sys.exit(1)


def setup_signal_handlers():
    """Setup signal handlers for graceful cleanup on interruption."""
    signal.signal(signal.SIGINT, _signal_handler)
    signal.signal(signal.SIGTERM, _signal_handler)
    if hasattr(signal, 'SIGBREAK'):  # Windows
        signal.signal(signal.SIGBREAK, _signal_handler)


# ANSI color codes
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[0;33m'
    BLUE = '\033[0;34m'
    BOLD = '\033[1m'
    NC = '\033[0m'  # No Color

    @classmethod
    def disable(cls):
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.BOLD = ''
        cls.NC = ''


def is_windows() -> bool:
    return platform.system() == 'Windows'


def get_exe_extension() -> str:
    return '.exe' if is_windows() else ''


def find_compiler(specified_path: Optional[str] = None) -> str:
    """Find the compiler executable."""
    if specified_path:
        # Always return absolute path for Windows subprocess compatibility
        return os.path.abspath(specified_path)

    exe_ext = get_exe_extension()
    candidates = [
        f'bin/sn{exe_ext}',
        f'./bin/sn{exe_ext}',
    ]

    for candidate in candidates:
        if os.path.isfile(candidate):
            # Always return absolute path for Windows subprocess compatibility
            return os.path.abspath(candidate)

    raise FileNotFoundError("Could not find compiler. Specify with --compiler")


def run_with_timeout(cmd: List[str], timeout: int, cwd: Optional[str] = None,
                     env: Optional[dict] = None,
                     merge_stderr: bool = False) -> Tuple[int, str, str, Optional[str]]:
    """Run a command with timeout, returning exit code, text streams, and decode error.

    If merge_stderr is True, stderr is redirected to stdout (like bash 2>&1),
    and stderr in the return value will be empty. Sindarin compiler and generated
    program output is UTF-8 regardless of the host locale, so capture bytes and
    decode strictly. Invalid external-tool bytes get a display-safe escaped copy
    plus a separate error that callers must handle before making assertions.
    """
    def decode_stream(data: bytes, stream_name: str) -> Tuple[str, Optional[str]]:
        try:
            return data.decode('utf-8'), None
        except UnicodeDecodeError as error:
            display = data.decode('utf-8', errors='backslashreplace')
            detail = (f'{stream_name} is not valid UTF-8 at byte {error.start}: '
                      f'{error.reason}')
            return display, detail

    try:
        if merge_stderr:
            # Merge stderr into stdout (like bash's 2>&1)
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=timeout,
                cwd=cwd,
                env=env
            )
            stdout, decode_error = decode_stream(result.stdout, 'merged subprocess output')
            return result.returncode, stdout, '', decode_error
        else:
            result = subprocess.run(
                cmd,
                capture_output=True,
                timeout=timeout,
                cwd=cwd,
                env=env
            )
            stdout, stdout_error = decode_stream(result.stdout, 'subprocess stdout')
            stderr, stderr_error = decode_stream(result.stderr, 'subprocess stderr')
            decode_error = '; '.join(error for error in (stdout_error, stderr_error) if error)
            return result.returncode, stdout, stderr, decode_error or None
    except subprocess.TimeoutExpired:
        return -1, '', 'TIMEOUT', None
    except Exception as e:
        return -1, '', str(e), None


def parse_rustc_capture(capture_file: str) -> List[List[bytes]]:
    """Parse SN_FAKE_RUSTC_CAPTURE output into a list of argv records.

    The fake rustc writes binary-safe records as INVOCATION, ARGC <count>, and
    then ARG <byte-count> followed by that exact many argument bytes and one
    record-separator newline. Each record preserves exact raw argument bytes,
    including embedded newlines and bytes that are invalid in a platform text
    encoding.
    """
    records: List[List[bytes]] = []
    if not os.path.isfile(capture_file):
        return records

    data = Path(capture_file).read_bytes()
    offset = 0

    def read_line() -> bytes:
        nonlocal offset
        end = data.find(b'\n', offset)
        if end < 0:
            raise ValueError('truncated fake rustc capture header')
        line = data[offset:end]
        offset = end + 1
        return line

    while offset < len(data):
        if read_line() != b'INVOCATION':
            raise ValueError('invalid fake rustc capture invocation marker')
        argc_header = read_line().split(b' ', 1)
        if len(argc_header) != 2 or argc_header[0] != b'ARGC':
            raise ValueError('invalid fake rustc capture argc header')
        try:
            argc = int(argc_header[1])
        except ValueError as exc:
            raise ValueError('invalid fake rustc capture argc') from exc
        if argc < 0:
            raise ValueError('negative fake rustc capture argc')

        record: List[bytes] = []
        for _ in range(argc):
            arg_header = read_line().split(b' ', 1)
            if len(arg_header) != 2 or arg_header[0] != b'ARG':
                raise ValueError('invalid fake rustc capture argument header')
            try:
                length = int(arg_header[1])
            except ValueError as exc:
                raise ValueError('invalid fake rustc capture argument length') from exc
            if length < 0 or offset + length >= len(data):
                raise ValueError('truncated fake rustc capture argument')
            raw_arg = data[offset:offset + length]
            offset += length
            if data[offset] != ord('\n'):
                raise ValueError('missing fake rustc capture argument separator')
            offset += 1
            record.append(raw_arg)
        records.append(record)
    return records


def assert_rustc_invocation(records: List[List[bytes]], output_file: str) -> Tuple[str, Optional[List[str]]]:
    """Verify the fake rustc was invoked correctly. Returns (reason, details).

    A clean pass yields ('', None); a problem yields ('<reason>', details).
    """
    version_records = [r for r in records if b'--version' in r]
    if not version_records:
        return 'no --version toolchain-check record', None

    build_records = [r for r in records if b'--edition=2021' in r]
    if not build_records:
        return 'no --edition=2021 build record', None
    build = build_records[0]

    details: List[str] = []
    if b'--edition=2021' not in build:
        details.append('build record is missing the --edition=2021 argument')
    if not any(arg.endswith(b'.rs') for arg in build):
        details.append('build record is missing a .rs source argument')
    if b'-o' not in build:
        details.append('build record is missing the -o flag')
    else:
        idx = build.index(b'-o')
        actual = build[idx + 1] if idx + 1 < len(build) else None
        expected = os.fsencode(output_file)
        if actual != expected:
            details.append(f'-o target is {actual!r}, expected {expected!r}')

    if details:
        return 'invocation record mismatch', details
    return '', None


def assert_rustc_build_flags(records: List[List[bytes]], expected_flags: List[bytes],
                             output_file: str) -> Tuple[str, Optional[List[str]]]:
    """Verify the complete ordered flag region before the Rust source argv."""
    reason, details = assert_rustc_invocation(records, output_file)
    if reason:
        return reason, details
    build = next(r for r in records if b'--edition=2021' in r)
    edition_index = build.index(b'--edition=2021')
    source_index = next(i for i, arg in enumerate(build) if arg.endswith(b'.rs'))
    actual_flags = build[edition_index + 1:source_index]
    if actual_flags != expected_flags:
        return ('unexpected rustc build flags',
                [f'flags before generated source are {actual_flags!r}, expected {expected_flags!r}'])
    return '', None


def assert_rustc_toolchain_diagnostic(stderr: str, rustc_path: str) -> Optional[List[str]]:
    """Verify the 'Rust toolchain unavailable' diagnostic.

    Returns None when both diagnostic lines are present, else a details list.
    """
    expected = [
        f"Error: Rust compiler '{rustc_path}' is not installed or not in PATH.",
        "Set SN_RUSTC to a different compiler, or use --emit-rust.",
    ]
    details = [line for line in expected if line not in stderr]
    return details or None


def find_single_build_dir(case_dir: str, target: str,
                          source_basename: str) -> Tuple[Optional[Path], Optional[str]]:
    """Find the sole build directory and require <source basename>_<numeric PID>."""
    build_dirs = list(Path(case_dir, '.sn', 'build', target).glob(f'{source_basename}_*'))
    if len(build_dirs) != 1:
        return None, f'expected one {target} build directory, found {len(build_dirs)}'
    build_dir = build_dirs[0]
    pid = build_dir.name[len(source_basename) + 1:]
    if not build_dir.is_dir() or not pid or not pid.isascii() or not pid.isdecimal():
        return None, (f'expected {target} build directory named {source_basename}_<numeric PID>, '
                      f'found {build_dir.name!r}')
    return build_dir, None


def format_subprocess_failure(stdout: str, stderr: str) -> str:
    """Return both subprocess streams so platform compiler failures are actionable."""
    return f'stdout:\n{stdout.strip() or "<empty>"}\nstderr:\n{stderr.strip() or "<empty>"}'


def append_shell_fragment(existing: str, fragment: str) -> str:
    """Append a raw shell fragment without altering any inherited content."""
    return f'{existing} {fragment}' if existing else fragment


class TestConfig:
    """Configuration for a test type."""
    def __init__(self, test_dir: str, pattern: str, expect_compile_fail: bool, title: str):
        self.test_dir = test_dir
        self.pattern = pattern
        self.expect_compile_fail = expect_compile_fail
        self.title = title


TEST_CONFIGS = {
    'rgen': TestConfig('tests/rgen', '*.sn', False, 'Rust Generation Tests'),
    'rgen-errors': TestConfig('tests/rgen/errors', '*.sn', True, 'Rust Generation Error Tests'),
    'rust-native-tagged': TestConfig(
        '.', '', False, 'Tagged Native Scalar/Source/Link Parity Tests'),
    'rust-native-extra': TestConfig(
        'tests/rust-native', 'scalar_*.sn', False, 'Post-Tag Rust Native Extra Coverage'),
    'rust-native-origin': TestConfig(
        'tests/rust-native', 'imported_*.sn', False, 'Rust Native Imported-Origin Extra Coverage'),
    'rust-native-errors': TestConfig(
        'tests/rust-native/errors', '*.sn', True, 'Rust Native Scalar Error Tests'),
}

class TestRunner:
    def __init__(self, compiler: str, compile_timeout: int = 10,
                 run_timeout: int = 30, excluded_tests: List[str] = None,
                 verbose: bool = False, parallel: int = 1, filter_pattern: str = None):
        self.compiler = compiler
        self.compile_timeout = compile_timeout
        self.run_timeout = run_timeout
        self.excluded_tests = excluded_tests or []
        self.verbose = verbose
        self.parallel = parallel
        self.filter_pattern = filter_pattern
        self.temp_dir = None
        self._progress_lock = threading.Lock()
        self._completed_count = 0
        self._total_count = 0

        # Setup environment
        self.env = os.environ.copy()
        # Set ASAN options to avoid leak detection issues
        if 'ASAN_OPTIONS' not in self.env:
            self.env['ASAN_OPTIONS'] = 'detect_leaks=1'

        # Add library paths for runtime linking
        if is_windows():
            # Windows: add vcpkg DLL directories to PATH
            vcpkg_bins = [
                os.path.join('vcpkg', 'installed', 'x64-windows', 'bin'),
                os.path.join('vcpkg', 'installed', 'x64-mingw-dynamic', 'bin'),
                os.path.join('bin', 'deps', 'lib'),
            ]
            for vcpkg_bin in vcpkg_bins:
                if os.path.isdir(vcpkg_bin):
                    abs_bin = os.path.abspath(vcpkg_bin)
                    self.env['PATH'] = abs_bin + os.pathsep + self.env.get('PATH', '')
        else:
            # Linux/macOS: add library directories to LD_LIBRARY_PATH/DYLD_LIBRARY_PATH
            lib_paths = [
                os.path.join('bin', 'deps', 'lib'),
                os.path.join('vcpkg_installed', 'x64-linux-dynamic', 'lib'),
                os.path.join('vcpkg_installed', 'arm64-osx', 'lib'),
                os.path.join('vcpkg_installed', 'x64-osx', 'lib'),
                os.path.join('vcpkg', 'installed', 'x64-linux-dynamic', 'lib'),
                os.path.join('vcpkg', 'installed', 'arm64-osx', 'lib'),
                os.path.join('vcpkg', 'installed', 'x64-osx', 'lib'),
            ]
            existing_paths = []
            for lib_path in lib_paths:
                if os.path.isdir(lib_path):
                    existing_paths.append(os.path.abspath(lib_path))

            if existing_paths:
                ld_path_var = 'DYLD_LIBRARY_PATH' if sys.platform == 'darwin' else 'LD_LIBRARY_PATH'
                current_path = self.env.get(ld_path_var, '')
                new_paths = os.pathsep.join(existing_paths)
                if current_path:
                    self.env[ld_path_var] = new_paths + os.pathsep + current_path
                else:
                    self.env[ld_path_var] = new_paths

    def __enter__(self):
        global _active_runner
        self.temp_dir = tempfile.mkdtemp(prefix='sn_test_')
        _active_runner = self
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        global _active_runner
        if self.temp_dir and os.path.exists(self.temp_dir):
            shutil.rmtree(self.temp_dir, ignore_errors=True)
        _active_runner = None

    @staticmethod
    def _optimization_args(test_file: str) -> Tuple[Optional[List[str]], Optional[str]]:
        """Return a fixture's optimizer flags; tests default to -O0 for stable snapshots."""
        opt_file = os.path.splitext(test_file)[0] + '.opt'
        if not os.path.isfile(opt_file):
            return ['-O0'], None
        try:
            with open(opt_file, 'r', encoding='ascii') as f:
                mode = f.read().strip()
        except OSError as e:
            return None, f'cannot read .opt sidecar {opt_file}: {e}'
        if mode == 'default':
            return [], None
        if mode in ('-O0', '-O1', '-O2'):
            return [mode], None
        return None, (f'invalid .opt sidecar in {opt_file}: expected default, -O0, -O1, '
                      f'or -O2; got {mode!r}')


    def _run_single_test(self, test_info: Dict[str, Any]) -> Dict[str, Any]:
        """Run a single test and return result dict. Thread-safe."""
        test_file = test_info['test_file']
        test_name = test_info['test_name']
        config = test_info['config']
        test_type = test_info['test_type']
        exe_file = test_info['exe_file']

        # Check if test is excluded
        if test_name in self.excluded_tests:
            result = {
                'test_name': test_name,
                'status': 'skip',
                'reason': 'excluded',
                'details': None,
                'elapsed': 0.0
            }
        else:
            panic_file = test_file.replace('.sn', '.panic')

            start_time = time.perf_counter()

            if test_type == 'rgen':
                expected_file = os.path.splitext(test_file)[0] + '.expected.rs'
                rs_file = exe_file + '.rs'
                status, reason, details = self._run_rgen_test_internal(
                    test_file, expected_file, rs_file, exe_file
                )
            elif test_type == 'rgen-errors':
                expected_file = os.path.splitext(test_file)[0] + '.expected'
                rs_file = exe_file + '.rs'
                status, reason, details = self._run_rgen_error_test_internal(
                    test_file, expected_file, rs_file
                )
            elif test_type == 'rust-native-tagged':
                expected_file = test_file.replace('.sn', '.expected')
                if test_file == 'tests/cgen/pragma_source.sn':
                    expected_file = None
                status, reason, details = self._run_rust_native_parity_test_internal(
                    test_file, expected_file, exe_file)
            elif test_type in ('rust-native-extra', 'rust-native-origin'):
                expected_file = test_file.replace('.sn', '.expected')
                status, reason, details = self._run_rust_native_target_test_internal(
                    test_file, expected_file, exe_file)
            elif test_type == 'rust-native-errors':
                expected_file = test_file.replace('.sn', '.expected')
                status, reason, details = self._run_rust_native_error_test_internal(
                    test_file, expected_file, exe_file)

            elapsed = time.perf_counter() - start_time

            result = {
                'test_name': test_name,
                'status': status,
                'reason': reason,
                'details': details,
                'elapsed': elapsed
            }

        # Update progress counter
        with self._progress_lock:
            self._completed_count += 1
            if self.parallel > 1 and sys.stdout.isatty():
                # Show progress indicator for parallel runs (only on TTY)
                sys.stdout.write(f"\r  [{self._completed_count}/{self._total_count}] Running tests...    ")
                sys.stdout.flush()

        return result

    def run_sn_tests(self, test_type: str) -> Tuple[bool, float]:
        """Run Sindarin source file tests. Returns (passed, elapsed_seconds)."""
        config = TEST_CONFIGS.get(test_type)
        if not config:
            print(f"Unknown test type: {test_type}")
            return False, 0.0

        print()
        print(f"{Colors.BOLD}{config.title}{Colors.NC}")
        print("=" * 60)
        suite_start = time.perf_counter()

        # Find test files
        pattern = os.path.join(config.test_dir, config.pattern)
        if test_type == 'rust-native-tagged':
            test_files = [
                'tests/cgen/pragma_source.sn',
                'tests/integration/test_native_math.sn',
                'tests/integration/test_native_with_body.sn',
            ]
        else:
            test_files = sorted(glob.glob(pattern, recursive=True))
        # Normalize to forward slashes — the compiler expects Unix-style paths
        # (Windows glob returns backslashes which break build dir creation)
        test_files = [f.replace('\\', '/') for f in test_files]

        # Apply filter if specified
        if self.filter_pattern:
            test_files = [f for f in test_files if self.filter_pattern in os.path.basename(f)]

        if not test_files:
            print(f"No test files found matching: {pattern}")
            return True, 0.0

        exe_ext = get_exe_extension()

        # Build test info list
        test_infos = []
        for idx, test_file in enumerate(test_files):
            rel_path = os.path.relpath(test_file, config.test_dir)
            test_name = os.path.splitext(rel_path)[0]
            # Use unique exe name with index to avoid conflicts in parallel runs
            exe_basename = f"test_{idx}_{os.path.basename(test_file).replace('.sn', '')}"
            exe_file = os.path.join(self.temp_dir, f"{exe_basename}{exe_ext}")

            test_infos.append({
                'test_file': test_file,
                'test_name': test_name,
                'config': config,
                'test_type': test_type,
                'exe_file': exe_file,
                'index': idx
            })

        # Reset progress counters
        self._completed_count = 0
        self._total_count = len(test_infos)

        # Run tests (parallel or sequential)
        if self.parallel > 1:
            print(f"  Running {len(test_infos)} tests with {self.parallel} workers...")
            with ThreadPoolExecutor(max_workers=self.parallel) as executor:
                futures = {executor.submit(self._run_single_test, info): info for info in test_infos}
                results = []
                for future in as_completed(futures):
                    results.append(future.result())
            # Clear progress line
            print("\r" + " " * 60 + "\r", end='')
            # Sort results by original index to maintain order
            results.sort(key=lambda r: next(i['index'] for i in test_infos if i['test_name'] == r['test_name']))
        else:
            # Sequential execution (original behavior with live output)
            results = []
            for info in test_infos:
                result = self._run_single_test(info)
                results.append(result)
                # Print result immediately in sequential mode (only failures/skips)
                self._print_test_result(result, include_name=True)

        # Print results (for parallel mode, print all at end)
        passed = 0
        failed = 0
        skipped = 0

        if self.parallel > 1:
            for result in results:
                self._print_test_result(result, include_name=True)

        # Count results
        for result in results:
            if result['status'] == 'pass':
                passed += 1
            elif result['status'] == 'skip':
                skipped += 1
            else:
                failed += 1

        suite_elapsed = time.perf_counter() - suite_start

        print()
        print("-" * 60)
        print(f"Results: {Colors.GREEN}{passed} passed{Colors.NC}, "
              f"{Colors.RED}{failed} failed{Colors.NC}, "
              f"{Colors.YELLOW}{skipped} skipped{Colors.NC}"
              f"  ({self._format_elapsed(suite_elapsed)})")

        return failed == 0, suite_elapsed

    def run_rust_toolchain_tests(self) -> Tuple[bool, float]:
        """Run the Rust toolchain and Rust generated-artifact lifecycle suite.

        Uses the sn_fake_rustc fixture and the SN_RUSTC / SN_FAKE_RUSTC_*
        environment variables to verify how the Rust target invokes rustc.
        """
        print()
        print(f"{Colors.BOLD}Rust Toolchain and Artifact Lifecycle Tests{Colors.NC}")
        print("=" * 60)
        suite_start = time.perf_counter()

        exe_ext = get_exe_extension()
        fake_rustc_src = os.path.abspath(os.path.join('bin', f'sn_fake_rustc{exe_ext}'))
        test_file = os.path.abspath('tests/rust-toolchain/basic.sn')
        native_test_file = os.path.abspath('tests/rust-native/scalar_bridge.sn')
        native_debug_test_file = os.path.abspath(
            'tests/integration/test_native_with_body.sn')

        if not os.path.isfile(fake_rustc_src):
            print(f"{Colors.RED}FAIL{Colors.NC}: fake rustc fixture not found: {fake_rustc_src}")
            return False, time.perf_counter() - suite_start
        if not os.path.isfile(test_file):
            print(f"{Colors.RED}FAIL{Colors.NC}: test fixture not found: {test_file}")
            return False, time.perf_counter() - suite_start
        if not os.path.isfile(native_test_file):
            print(f"{Colors.RED}FAIL{Colors.NC}: native fixture not found: {native_test_file}")
            return False, time.perf_counter() - suite_start
        if not os.path.isfile(native_debug_test_file):
            print(f"{Colors.RED}FAIL{Colors.NC}: tagged native fixture not found: "
                  f"{native_debug_test_file}")
            return False, time.perf_counter() - suite_start

        # Case 0: pin locale-independent strict UTF-8 subprocess decoding and
        # explicit, display-safe failure for invalid external diagnostic bytes.
        # Case 1: copy the fixture into a sub-path containing spaces, capture
        # the rustc invocations, and verify the --version/build records plus
        # default, SN_RUSTFLAGS, debug, and profile argv flag boundaries.
        # Case 2: point SN_RUSTC at a nonexistent path and require the exact
        # toolchain-unavailable diagnostic plus a nonzero exit.
        # Case 3: use the spaced fixture with SN_FAKE_RUSTC_EXIT set nonzero
        # (all invocations fail, so the --version check fails first) and
        # require the toolchain-unavailable diagnostic plus a nonzero exit.
        # Case 4: force the version exit to 0 and the build exit to 3 via
        # SN_FAKE_RUSTC_VERSION_EXIT / SN_FAKE_RUSTC_BUILD_EXIT, prove the
        # --version and build invocations were both captured, and pin the
        # exact 'Error: rustc failed to build generated source' diagnostic text,
        # and verify that its generated Rust source remains available.
        # Case 5: verify Rust successful-build cleanup and --keep-generated
        # retention in a private working directory.
        cases = [
            {'name': 'subprocess_utf8_boundary', 'kind': 'subprocess_utf8'},
            {'name': 'rustc_invocation_records', 'kind': 'records'},
            {'name': 'pure_rust_has_no_c_dependency', 'kind': 'pure_no_c'},
            {'name': 'native_emit_requires_bundle', 'kind': 'native_emit'},
            {'name': 'native_c_link_driver_contract', 'kind': 'native_link'},
            {'name': 'native_debug_sanitizer_link', 'kind': 'native_debug'},
            {'name': 'native_compile_and_link_failures', 'kind': 'native_failures'},
            {'name': 'missing_rustc', 'kind': 'missing'},
            {'name': 'failing_rustc', 'kind': 'failing'},
            {'name': 'failing_rustc_build', 'kind': 'failing_build'},
            {'name': 'rust_generated_artifact_lifecycle', 'kind': 'rust_lifecycle'},
            {'name': 'rust_native_artifact_lifecycle', 'kind': 'native_lifecycle'},
        ]

        results = []
        with tempfile.TemporaryDirectory(prefix='sn_rustc_') as temp_dir:
            spaced_dir = os.path.join(temp_dir, "fake rustc 'dir' &")
            os.makedirs(spaced_dir)
            spaced_rustc = os.path.join(spaced_dir, f'sn_fake_rustc{exe_ext}')
            shutil.copy2(fake_rustc_src, spaced_rustc)
            capture_file = os.path.join(temp_dir, 'capture.log')
            capture_file_build = os.path.join(temp_dir, 'capture_build.log')
            capture_file_conflict = os.path.join(temp_dir, 'capture_conflict.log')
            synthetic_capture_file = os.path.join(temp_dir, 'synthetic_capture.log')
            output_file = os.path.join(temp_dir, f'basic_output{exe_ext}')
            missing_rustc = os.path.join(temp_dir, 'definitely_not_a_rustc')

            for case in cases:
                env = self.env.copy()
                case_start = time.perf_counter()
                cmd = [self.compiler, test_file, '--target', 'rust', '-o', output_file,
                       '-l', '3', '--no-install']

                if case['kind'] == 'subprocess_utf8':
                    details = []
                    semantic_assertion_runs = 0
                    valid_cmd = [sys.executable, '-c',
                                 'import os; os.write(1, bytes([240, 144, 128, 128]))']
                    exit_code, stdout, stderr, decode_error = run_with_timeout(
                        valid_cmd, self.compile_timeout, env=env)
                    if decode_error:
                        details.append(f'valid UTF-8 subprocess output failed decoding: {decode_error}')
                    else:
                        semantic_assertion_runs += 1
                        if exit_code != 0 or stdout != '\U00010000' or stderr:
                            details.append('valid UTF-8 subprocess output was not decoded exactly')

                    invalid_cmd = [sys.executable, '-c',
                                   'import os, sys; os.write(2, bytes([255]) + b"external"); sys.exit(7)']
                    exit_code, stdout, stderr, decode_error = run_with_timeout(
                        invalid_cmd, self.compile_timeout, env=env)
                    if decode_error:
                        # The escaped copy is diagnostic-only; semantic assertions must
                        # never consume replacement or display-safe text.
                        if 'subprocess stderr is not valid UTF-8 at byte 0' not in decode_error:
                            details.append(f'invalid UTF-8 probe has the wrong decode error: {decode_error!r}')
                        if exit_code != 7:
                            details.append(f'invalid UTF-8 probe lost exit code 7: {exit_code}')
                        if stdout:
                            details.append(f'invalid UTF-8 probe unexpectedly wrote stdout: {stdout!r}')
                        if stderr != '\\xffexternal':
                            details.append(f'invalid UTF-8 diagnostic is not display-safe: {stderr!r}')
                    else:
                        semantic_assertion_runs += 1
                        details.append('invalid UTF-8 probe lacks explicit decode error')

                    if semantic_assertion_runs != 1:
                        details.append('decode-error output reached semantic assertions')

                    results.append({'name': case['name'],
                                   'status': 'pass' if not details else 'fail',
                                   'reason': '' if not details else 'UTF-8 boundary assertions unmet',
                                   'details': details or None,
                                   'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'records':
                    env['SN_RUSTC'] = spaced_rustc
                    details = []

                    # Exercise the parser directly with an argument that could
                    # otherwise be mistaken for multiple line-delimited records.
                    synthetic_bytes = (b'embedded newline\nARGC 999\nINVOCATION\n'
                                       b'marker-like value\xff')
                    Path(synthetic_capture_file).write_bytes(
                        b'INVOCATION\nARGC 1\nARG ' + str(len(synthetic_bytes)).encode('ascii') +
                        b'\n' + synthetic_bytes + b'\n')
                    try:
                        if parse_rustc_capture(synthetic_capture_file) != [[synthetic_bytes]]:
                            details.append('synthetic capture: raw newline/marker/invalid-UTF-8 argument was not returned intact')
                    except ValueError as exc:
                        details.append(f'synthetic capture: invalid capture: {exc}')

                    def check_build(label: str, extra_args: List[str],
                                    rustflags: Optional[str], expected_flags: List[bytes]) -> None:
                        case_env = env.copy()
                        if os.path.exists(capture_file):
                            os.unlink(capture_file)
                        case_env['SN_FAKE_RUSTC_CAPTURE'] = capture_file
                        if rustflags is not None:
                            case_env['SN_RUSTFLAGS'] = rustflags
                        else:
                            case_env.pop('SN_RUSTFLAGS', None)
                        exit_code, _stdout, stderr, decode_error = run_with_timeout(
                            cmd + extra_args, self.compile_timeout, env=case_env
                        )
                        if decode_error:
                            details.append(f'{label}: subprocess output decode error: {decode_error}; '
                                           f'stderr: {stderr!r}')
                            return
                        if exit_code != 0:
                            details.append(f'{label}: compile exit {exit_code}: {stderr.splitlines()[:3]!r}')
                            return
                        try:
                            records = parse_rustc_capture(capture_file)
                        except ValueError as exc:
                            details.append(f'{label}: invalid capture: {exc}')
                            return
                        reason, assertion_details = assert_rustc_build_flags(
                            records, expected_flags, output_file)
                        if reason:
                            details.append(f'{label}: {reason}' +
                                           (f' ({"; ".join(assertion_details)})'
                                            if assertion_details else ''))

                    check_build('default', [], None, [b'-C', b'opt-level=3'])
                    check_build('SN_RUSTFLAGS', [], '-C target-cpu=native',
                                [b'-C', b'opt-level=3', b'-C', b'target-cpu=native'])
                    check_build('-g', ['-g'], None,
                                [b'-C', b'debuginfo=2', b'-C', b'opt-level=0'])
                    check_build('-p', ['-p'], None,
                                [b'-C', b'debuginfo=1', b'-C', b'opt-level=3',
                                 b'-C', b'force-frame-pointers=yes'])

                    conflict_env = env.copy()
                    conflict_env['SN_FAKE_RUSTC_CAPTURE'] = capture_file_conflict
                    exit_code, _stdout, stderr, decode_error = run_with_timeout(
                        cmd + ['-g', '-p'], self.compile_timeout, env=conflict_env
                    )
                    expected_diagnostic = 'Error: -p (profile) and -g (debug) cannot be used together'
                    if decode_error:
                        details.append(f'-g -p: subprocess output decode error: {decode_error}; '
                                       f'stderr: {stderr!r}')
                    elif exit_code == 0:
                        details.append('-g -p: expected a nonzero compiler exit')
                    elif (stderr or '').strip() != expected_diagnostic:
                        details.append(f'-g -p: diagnostic is {(stderr or "").strip()!r}, '
                                       f'expected {expected_diagnostic!r}')
                    try:
                        conflict_records = parse_rustc_capture(capture_file_conflict)
                        if [r for r in conflict_records if b'--edition=2021' in r]:
                            details.append('-g -p: rustc build invocation was captured')
                    except ValueError as exc:
                        details.append(f'-g -p: invalid capture: {exc}')

                    status = 'pass' if not details else 'fail'
                    reason = '' if not details else 'invocation record assertions unmet'
                    results.append({'name': case['name'], 'status': status, 'reason': reason,
                                   'details': details or None, 'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'native_link':
                    details = []
                    if is_windows():
                        details.append('Windows C-driver argv capture remains a required platform validation')
                    else:
                        wrapper_dir = os.path.join(temp_dir, "native cc 'driver' &")
                        os.makedirs(wrapper_dir, exist_ok=True)
                        wrapper = os.path.join(wrapper_dir, 'cc capture')
                        cc_capture = os.path.join(temp_dir, 'native_cc_capture.log')
                        Path(wrapper).write_text(
                            '#!/bin/sh\n'
                            '{\n'
                            '  printf "%s\\n" INVOCATION\n'
                            '  for arg do printf "ARG %s\\n" "$arg"; done\n'
                            '} >> "$SN_NATIVE_CC_CAPTURE"\n'
                            'exec "$SN_NATIVE_REAL_CC" "$@"\n',
                            encoding='utf-8')
                        os.chmod(wrapper, 0o700)
                        real_cc = shutil.which('gcc') or shutil.which('clang') or shutil.which('cc')
                        if not real_cc:
                            details.append('no existing C compiler found for native link capture')
                        else:
                            link_env = env.copy()
                            link_env['SN_CC'] = wrapper
                            link_env['SN_NATIVE_REAL_CC'] = real_cc
                            link_env['SN_NATIVE_CC_CAPTURE'] = cc_capture
                            link_env['SN_RELEASE_CFLAGS'] = '-O1 -DSN_MODE_LINK_MARKER'
                            link_env['SN_CFLAGS'] = '-DSN_CFLAGS_LINK_MARKER'
                            link_env['SN_LDLIBS'] = '-Wl,--defsym,SN_LDLIBS_LINK_MARKER=1'
                            link_env['SN_LDFLAGS'] = '-Wl,--defsym,SN_LDFLAGS_LINK_MARKER=1'
                            native_output = os.path.join(temp_dir, f'native_link_output{exe_ext}')
                            exit_code, stdout, stderr, decode_error = run_with_timeout(
                                [self.compiler, native_test_file, '--target', 'rust',
                                 '-o', native_output, '-l', '3', '--no-install'],
                                self.compile_timeout, env=link_env)
                            if decode_error:
                                details.append(f'subprocess output decode error: {decode_error}')
                            elif exit_code != 0:
                                details.append('native link capture compile failed:\n' +
                                               format_subprocess_failure(stdout, stderr))
                            elif not os.path.isfile(cc_capture):
                                details.append('configured C compiler wrapper was not invoked')
                            else:
                                invocations = []
                                current = None
                                for line in Path(cc_capture).read_text(encoding='utf-8').splitlines():
                                    if line == 'INVOCATION':
                                        current = []
                                        invocations.append(current)
                                    elif line.startswith('ARG ') and current is not None:
                                        current.append(line[4:])
                                links = [argv for argv in invocations if '-c' not in argv]
                                if len(links) != 1:
                                    details.append(f'expected one final C-driver link, got {len(links)}')
                                else:
                                    argv = links[0]
                                    if '-nodefaultlibs' in argv:
                                        details.append(
                                            'rustc suppressed configured C-driver default libraries')
                                    required_prefix = ['-O1', '-DSN_MODE_LINK_MARKER', '-w',
                                                       '-Werror=implicit-function-declaration',
                                                       '-std=c11', '-D_GNU_SOURCE',
                                                       '-DSN_CFLAGS_LINK_MARKER']
                                    try:
                                        indices = [argv.index(token) for token in required_prefix]
                                        if indices != sorted(indices):
                                            details.append('mode/strict/CFLAGS final-link ordering changed')
                                    except ValueError as exc:
                                        details.append(f'missing final-link prefix token: {exc}')
                                    requested = ['-lm', '-lssl', '-ldl']
                                    requested_at = next(
                                        (i for i in range(len(argv) - len(requested) + 1)
                                         if argv[i:i + len(requested)] == requested), None)
                                    if requested_at is None:
                                        details.append('ordered default/multi-token @link region missing')
                                    try:
                                        ldlibs_at = argv.index('-Wl,--defsym,SN_LDLIBS_LINK_MARKER=1')
                                        ldflags_at = argv.index('-Wl,--defsym,SN_LDFLAGS_LINK_MARKER=1')
                                        if requested_at is not None and not requested_at < ldlibs_at < ldflags_at:
                                            details.append('@link/SN_LDLIBS/SN_LDFLAGS ordering changed')
                                    except ValueError as exc:
                                        details.append(f'missing configured final-link token: {exc}')
                            if not details:
                                exit_code, _output, timeout_marker, decode_error = run_with_timeout(
                                    [native_output], self.run_timeout, env=link_env,
                                    merge_stderr=True)
                                if decode_error or timeout_marker == 'TIMEOUT' or exit_code != 0:
                                    details.append('captured native executable did not run successfully')
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'native C-link-driver assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'native_debug':
                    details = []
                    native_debug_output = os.path.join(
                        temp_dir, f'native_debug_output{exe_ext}')
                    exit_code, stdout, stderr, decode_error = run_with_timeout(
                        [self.compiler, native_debug_test_file, '--target', 'rust',
                         '-g', '-o', native_debug_output, '-l', '3', '--no-install'],
                        max(self.compile_timeout, 120) if is_windows()
                        else self.compile_timeout,
                        env=env)
                    if decode_error:
                        details.append(f'subprocess output decode error: {decode_error}')
                    elif exit_code != 0:
                        details.append('tagged native debug compile failed:\n' +
                                       format_subprocess_failure(stdout, stderr))
                    else:
                        exit_code, output, timeout_marker, decode_error = run_with_timeout(
                            [native_debug_output], self.run_timeout, env=env,
                            merge_stderr=True)
                        if decode_error:
                            details.append(f'native debug run decode error: {decode_error}')
                        elif timeout_marker == 'TIMEOUT':
                            details.append('native debug executable timed out')
                        elif exit_code != 0:
                            details.append(f'native debug executable exited {exit_code}: '
                                           f'{output!r}')
                        else:
                            expected_path = os.path.splitext(native_debug_test_file)[0] + '.expected'
                            expected = Path(expected_path).read_text(encoding='utf-8')
                            normalized = output.replace('\r\n', '\n').replace('\r', '\n')
                            if normalized != expected:
                                details.append(f'native debug output is {normalized!r}, '
                                               f'expected {expected!r}')
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'native debug assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'pure_no_c':
                    pure_env = env.copy()
                    pure_env['SN_RUSTC'] = spaced_rustc
                    pure_env['SN_FAKE_RUSTC_VERSION_EXIT'] = '0'
                    pure_env['SN_FAKE_RUSTC_BUILD_EXIT'] = '0'
                    pure_env['SN_CC'] = os.path.join(temp_dir, 'C compiler must not run')
                    exit_code, stdout, stderr, decode_error = run_with_timeout(
                        cmd, self.compile_timeout, env=pure_env)
                    details = []
                    if decode_error:
                        details.append(f'subprocess output decode error: {decode_error}')
                    elif exit_code != 0:
                        details.append('pure Rust build acquired a C dependency:\n' +
                                       format_subprocess_failure(stdout, stderr))
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'pure Rust isolation assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'native_emit':
                    emitted = os.path.join(temp_dir, 'native_single.rs')
                    emit_env = env.copy()
                    emit_env['SN_CC'] = os.path.join(temp_dir, 'C compiler must not run')
                    exit_code, _stdout, stderr, decode_error = run_with_timeout(
                        [self.compiler, native_test_file, '--emit-rust', '-o', emitted,
                         '-l', '1', '--no-install'], self.compile_timeout, env=emit_env)
                    diagnostic = ('Error: --emit-rust cannot represent native C bodies, headers, '
                                  'sources, or link options; build the Rust target executable instead')
                    details = []
                    if decode_error:
                        details.append(f'subprocess output decode error: {decode_error}')
                    elif exit_code == 0:
                        details.append('native --emit-rust unexpectedly succeeded')
                    elif diagnostic not in stderr:
                        details.append(f'native --emit-rust diagnostic missing: {stderr!r}')
                    if os.path.exists(emitted):
                        details.append('native --emit-rust left a partial Rust source')
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'native emit assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'native_failures':
                    details = []
                    missing_header = 'sn_native_intentional_missing_header.h'
                    compile_env = env.copy()
                    compile_env['SN_CFLAGS'] = f'-include {missing_header}'
                    failed_compile_output = os.path.join(temp_dir,
                                                         f'native_compile_failure{exe_ext}')
                    exit_code, stdout, stderr, decode_error = run_with_timeout(
                        [self.compiler, native_test_file, '--target', 'rust',
                         '-o', failed_compile_output, '-l', '3', '--no-install'],
                        self.compile_timeout, env=compile_env)
                    if decode_error:
                        details.append(f'native compile failure decode error: {decode_error}')
                    elif exit_code == 0:
                        details.append('forced native C compile failure unexpectedly succeeded')
                    elif missing_header not in stderr or 'failed to compile' not in stderr:
                        details.append('native C compile failure lost the underlying diagnostic:\n' +
                                       format_subprocess_failure(stdout, stderr))
                    if os.path.exists(failed_compile_output):
                        details.append('native C compile failure left an executable')
                    if not is_windows():
                        invalid_link = '--sn-native-intentional-link-failure'
                        link_env = env.copy()
                        link_env['SN_LDFLAGS'] = f'-Wl,{invalid_link}'
                        failed_link_output = os.path.join(temp_dir,
                                                         f'native_link_failure{exe_ext}')
                        exit_code, stdout, stderr, decode_error = run_with_timeout(
                            [self.compiler, native_test_file, '--target', 'rust',
                             '-o', failed_link_output, '-l', '3', '--no-install'],
                            self.compile_timeout, env=link_env)
                        if decode_error:
                            details.append(f'native link failure decode error: {decode_error}')
                        elif exit_code == 0:
                            details.append('forced native final-link failure unexpectedly succeeded')
                        elif (invalid_link not in stderr or
                              'rustc failed to link generated Rust and native C objects' not in stderr):
                            details.append('native final-link failure lost its diagnostic:\n' +
                                           format_subprocess_failure(stdout, stderr))
                        if os.path.exists(failed_link_output):
                            details.append('native final-link failure left an executable')
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'native failure assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'missing':
                    env['SN_RUSTC'] = missing_rustc
                    exit_code, _stdout, stderr, decode_error = run_with_timeout(
                        cmd, self.compile_timeout, env=env
                    )
                    if decode_error:
                        results.append({'name': case['name'], 'status': 'fail',
                                       'reason': 'subprocess output decode error',
                                       'details': [decode_error, stderr],
                                       'elapsed': time.perf_counter() - case_start})
                    elif exit_code == 0:
                        results.append({'name': case['name'], 'status': 'fail',
                                       'reason': 'expected a nonzero compiler exit',
                                       'details': None, 'elapsed': time.perf_counter() - case_start})
                    else:
                        details = assert_rustc_toolchain_diagnostic(stderr, missing_rustc)
                        results.append({'name': case['name'],
                                       'status': 'pass' if details is None else 'fail',
                                       'reason': '' if details is None else 'missing toolchain diagnostic',
                                       'details': details, 'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'failing':
                    env['SN_RUSTC'] = spaced_rustc
                    env['SN_FAKE_RUSTC_EXIT'] = '3'
                    exit_code, _stdout, stderr, decode_error = run_with_timeout(
                        cmd, self.compile_timeout, env=env
                    )
                    if decode_error:
                        results.append({'name': case['name'], 'status': 'fail',
                                       'reason': 'subprocess output decode error',
                                       'details': [decode_error, stderr],
                                       'elapsed': time.perf_counter() - case_start})
                    elif exit_code == 0:
                        results.append({'name': case['name'], 'status': 'fail',
                                       'reason': 'expected a nonzero compiler exit',
                                       'details': None, 'elapsed': time.perf_counter() - case_start})
                    else:
                        details = assert_rustc_toolchain_diagnostic(stderr, spaced_rustc)
                        results.append({'name': case['name'],
                                       'status': 'pass' if details is None else 'fail',
                                       'reason': '' if details is None else 'missing toolchain diagnostic',
                                       'details': details, 'elapsed': time.perf_counter() - case_start})

                elif case['kind'] == 'failing_build':
                    env['SN_RUSTC'] = spaced_rustc
                    env['SN_FAKE_RUSTC_VERSION_EXIT'] = '0'
                    env['SN_FAKE_RUSTC_BUILD_EXIT'] = '3'
                    env['SN_FAKE_RUSTC_CAPTURE'] = capture_file_build
                    details = []
                    for keep_generated in (False, True):
                        label = '--keep-generated' if keep_generated else 'default'
                        case_dir = os.path.join(temp_dir, f'failing-build-{int(keep_generated)}')
                        os.makedirs(case_dir)
                        failed_output = os.path.join(case_dir, f'basic_output{exe_ext}')
                        failed_cmd = [self.compiler, test_file, '--target', 'rust', '-o', failed_output,
                                      '-l', '3', '--no-install']
                        if keep_generated:
                            failed_cmd.append('--keep-generated')
                        exit_code, _stdout, stderr, decode_error = run_with_timeout(
                            failed_cmd, self.compile_timeout, cwd=case_dir, env=env)
                        if decode_error:
                            details.append(f'{label}: subprocess output decode error: {decode_error}; '
                                           f'stderr: {stderr!r}')
                            continue
                        if exit_code == 0:
                            details.append(f'{label}: expected a nonzero compiler exit')
                            continue
                        diag = 'Error: rustc failed to build generated source'
                        if diag not in (stderr or ''):
                            details.append(f'{label}: missing exact diagnostic text '
                                           "'Error: rustc failed to build generated source'")
                        build_dir, reason = find_single_build_dir(case_dir, 'rust', 'basic')
                        if reason:
                            details.append(f'{label}: {reason}')
                        elif not (build_dir / 'main.rs').is_file():
                            details.append(f'{label}: failed Rust build removed generated main.rs')
                    records = parse_rustc_capture(capture_file_build)
                    if not [r for r in records if b'--version' in r]:
                        details.append('no --version toolchain-check record captured')
                    if not [r for r in records if b'--edition=2021' in r]:
                        details.append('no --edition=2021 build record captured')
                    results.append({'name': case['name'],
                                   'status': 'pass' if not details else 'fail',
                                   'reason': '' if not details else 'failing build assertions unmet',
                                   'details': details or None, 'elapsed': time.perf_counter() - case_start})

                elif case['kind'] in ('rust_lifecycle', 'native_lifecycle'):
                    details = []
                    for keep_generated in (False, True):
                        label = '--keep-generated' if keep_generated else 'default cleanup'
                        case_dir = os.path.join(temp_dir,
                                                f'{case["kind"]}-{int(keep_generated)}')
                        os.makedirs(case_dir)
                        output = os.path.join(case_dir, f'basic_output{exe_ext}')
                        case_env = env.copy()
                        source_file = (native_test_file if case['kind'] == 'native_lifecycle'
                                       else test_file)
                        source_basename = ('scalar_bridge' if case['kind'] == 'native_lifecycle'
                                           else 'basic')
                        if case['kind'] == 'rust_lifecycle':
                            case_env['SN_RUSTC'] = spaced_rustc
                        command = [self.compiler, source_file, '--target', 'rust',
                                   '-o', output, '-l', '3', '--no-install']
                        if keep_generated:
                            command.append('--keep-generated')
                        exit_code, stdout, stderr, decode_error = run_with_timeout(
                            command, self.compile_timeout, cwd=case_dir, env=case_env)
                        if decode_error or exit_code != 0:
                            details.append(f'{label}: compilation failed: {decode_error}\n'
                                           f'{format_subprocess_failure(stdout, stderr)}')
                            continue
                        build_dir, reason = find_single_build_dir(
                            case_dir, 'rust', source_basename)
                        if reason:
                            details.append(f'{label}: {reason}')
                        else:
                            if keep_generated:
                                required = [build_dir / 'main.rs']
                                if case['kind'] == 'native_lifecycle':
                                    required.append(build_dir / 'sn_types.h')
                                if not all(path.is_file() for path in required):
                                    details.append(f'{label}: Rust generated bundle was not retained')
                                if case['kind'] == 'native_lifecycle':
                                    proxy_name = ('sn_rust_linker_proxy.cmd' if is_windows()
                                                  else 'sn_rust_linker_proxy.sh')
                                    if not (build_dir / proxy_name).is_file():
                                        details.append(f'{label}: native linker proxy was not retained')
                                    if not list(build_dir.glob('sn_native_bridge_*.c')):
                                        details.append(f'{label}: native C body source was not retained')
                            else:
                                generated = [build_dir / 'main.rs']
                                if case['kind'] == 'native_lifecycle':
                                    generated.append(build_dir / 'sn_types.h')
                                    generated.extend(build_dir.glob('sn_native_bridge_*.c'))
                                    generated.extend(build_dir.glob('sn_rust_linker_proxy.*'))
                                if any(path.exists() for path in generated):
                                    details.append(f'{label}: Rust generated artifacts were not removed')
                            if case['kind'] == 'native_lifecycle':
                                native_objects = list(build_dir.glob('*.o'))
                                if len(native_objects) != 4:
                                    details.append(
                                        f'{label}: expected four native objects (one generated body, '
                                        'one deduplicated repeated source, and two distinct same-basename '
                                        f'sources), got {[path.name for path in native_objects]!r}')
                    results.append({'name': case['name'],
                                    'status': 'pass' if not details else 'fail',
                                    'reason': '' if not details else 'artifact lifecycle assertions unmet',
                                    'details': details or None,
                                    'elapsed': time.perf_counter() - case_start})

                self._print_rustc_case_result(results[-1])

        passed = sum(1 for r in results if r['status'] == 'pass')
        failed = sum(1 for r in results if r['status'] == 'fail')
        suite_elapsed = time.perf_counter() - suite_start
        print()
        print("-" * 60)
        print(f"Results: {Colors.GREEN}{passed} passed{Colors.NC}, "
               f"{Colors.RED}{failed} failed{Colors.NC}"
               f"  ({self._format_elapsed(suite_elapsed)})")
        return failed == 0, suite_elapsed

    def _print_rustc_case_result(self, result: Dict[str, Any]):
        """Print the outcome of a single rust-toolchain case."""
        elapsed_str = self._format_elapsed(result.get('elapsed', 0.0))
        if result['status'] == 'pass':
            print(f"  {result['name']:45} {Colors.GREEN}PASS{Colors.NC}  ({elapsed_str})")
            return
        print(f"  {result['name']:45} {Colors.RED}FAIL{Colors.NC} ({result['reason']})")
        if result.get('details'):
            for line in result['details']:
                print(f"    {line}")

    @staticmethod
    def _format_elapsed(elapsed: float) -> str:
        """Format elapsed time for display."""
        if elapsed >= 1.0:
            return f"{elapsed:.2f}s"
        else:
            return f"{elapsed * 1000:.0f}ms"

    def _print_test_result(self, result: Dict[str, Any], include_name: bool = False):
        """Print a single test result. Passing tests are silent."""
        test_name = result['test_name']
        status = result['status']
        reason = result['reason']
        details = result['details']
        elapsed = result.get('elapsed', 0.0)

        if status == 'pass':
            return  # Silent on pass

        if include_name:
            print(f"  {test_name:45} ", end='')

        time_str = f"  ({self._format_elapsed(elapsed)})" if elapsed > 0 else ""

        if status == 'skip':
            print(f"{Colors.YELLOW}SKIP{Colors.NC} ({reason})")
        else:
            print(f"{Colors.RED}FAIL{Colors.NC} ({reason}){time_str}")
            if details:
                for line in details[:50]:
                    print(f"    {line}")


    def _run_rgen_test_internal(self, test_file: str, expected_file: str,
                                 rs_file: str, exe_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Compare generated Rust, then build and run it through the Rust target."""
        if not os.path.isfile(expected_file):
            return ('skip', 'no .expected.rs', None)

        optimization_args, optimization_error = self._optimization_args(test_file)
        if optimization_error:
            return ('fail', 'invalid .opt sidecar', [optimization_error])

        compile_cmd = [self.compiler, test_file, '--emit-rust', '-o', rs_file,
                       '-l', '1', *optimization_args, '--no-install']
        exit_code, stdout, stderr, decode_error = run_with_timeout(
            compile_cmd, self.compile_timeout, env=self.env
        )
        if decode_error:
            return ('fail', 'subprocess output decode error',
                    [decode_error, format_subprocess_failure(stdout, stderr)])
        if exit_code != 0:
            details = stderr.split('\n')[:50] if stderr else None
            return ('fail', 'Rust emission error', details)
        if not os.path.isfile(rs_file):
            return ('fail', 'no Rust output', None)

        with open(rs_file, 'r', encoding='utf-8') as generated, \
                open(expected_file, 'r', encoding='utf-8') as expected:
            generated_rs = generated.read().replace('\r\n', '\n').replace('\r', '\n')
            expected_rs = expected.read().replace('\r\n', '\n').replace('\r', '\n')
        if generated_rs != expected_rs:
            details = []
            generated_lines = generated_rs.split('\n')
            expected_lines = expected_rs.split('\n')
            for index in range(max(len(generated_lines), len(expected_lines))):
                actual = generated_lines[index] if index < len(generated_lines) else '<missing>'
                wanted = expected_lines[index] if index < len(expected_lines) else '<missing>'
                if actual != wanted and len(details) < 30:
                    details.extend([f"  line {index + 1}:", f"    expected: {wanted}",
                                    f"    got:      {actual}"])
            return ('fail', 'Rust code mismatch', details)

        compile_cmd = [self.compiler, test_file, '--target', 'rust', '-o', exe_file,
                       '-l', '1', *optimization_args, '--no-install']
        # Four parallel rustc processes can exceed the general 60-second compile
        # limit on cold Windows runners even though each compilation succeeds.
        rust_compile_timeout = (max(self.compile_timeout, 120)
                                if is_windows() else self.compile_timeout)
        exit_code, stdout, stderr, decode_error = run_with_timeout(
            compile_cmd, rust_compile_timeout, env=self.env
        )
        if decode_error:
            return ('fail', 'subprocess output decode error',
                    [decode_error, format_subprocess_failure(stdout, stderr)])
        if exit_code != 0:
            details = stderr.split('\n')[:50] if stderr else None
            return ('fail', 'Rust binary compile error', details)

        panic_file = os.path.splitext(test_file)[0] + '.panic'
        exit_file = os.path.splitext(test_file)[0] + '.exit'
        expects_panic = os.path.isfile(panic_file)
        if expects_panic and os.path.isfile(exit_file):
            return ('fail', f'conflicting .panic and .exit sidecars in {panic_file} and {exit_file}', None)

        expected_exit = 0
        if os.path.isfile(exit_file):
            with open(exit_file, 'r', encoding='ascii') as f:
                raw = f.read().strip()
            if not raw:
                return ('fail', f'empty .exit sidecar in {exit_file}', None)
            try:
                expected_exit = int(raw, 10)
            except ValueError:
                return ('fail', f'invalid .exit sidecar in {exit_file}: {raw!r}', None)

        args_file = os.path.splitext(test_file)[0] + '.args'
        run_args = []
        if os.path.isfile(args_file):
            with open(args_file, 'r', encoding='utf-8') as f:
                raw = f.read().strip()
            if not raw:
                return ('fail', f'empty .args sidecar in {args_file}', None)
            try:
                parsed = json.loads(raw)
            except ValueError as e:
                return ('fail', f'invalid .args sidecar in {args_file}: {e}', None)
            if not isinstance(parsed, list):
                return ('fail', f'invalid .args sidecar in {args_file}: expected a JSON array', None)
            for item in parsed:
                if not isinstance(item, str):
                    return ('fail', f'invalid .args sidecar in {args_file}: all elements must be strings', None)
                if '\x00' in item:
                    return ('fail', f'invalid .args sidecar in {args_file}: string contains embedded NUL', None)
            run_args = parsed

        exit_code, output, timeout_marker, decode_error = run_with_timeout(
            [exe_file] + run_args, self.run_timeout, env=self.env, merge_stderr=True
        )
        if decode_error:
            return ('fail', 'subprocess output decode error', [decode_error, output])
        if timeout_marker == 'TIMEOUT':
            return ('fail', 'timeout', output.split('\n')[:20] if output else None)
        if expects_panic:
            # A nonempty marker is the wrapper's launch/runner exception detail.
            # A child killed by a POSIX signal can also return -1, with no marker.
            if timeout_marker:
                details = [f'run error: {timeout_marker}']
                if output:
                    details += output.split('\n')[:20]
                return ('fail', 'run error', details)
            if exit_code == 0:
                return ('fail', 'expected panic', None)
        elif exit_code != expected_exit:
            details = [f"expected exit code: {expected_exit}", f"actual exit code: {exit_code}"]
            if output:
                details.extend(output.split('\n')[:20])
            return ('fail', 'exit code mismatch', details)

        output_file = os.path.splitext(test_file)[0] + '.expected'
        if os.path.isfile(output_file):
            with open(output_file, 'r', encoding='utf-8') as expected:
                expected_output = expected.read().replace('\r\n', '\n').replace('\r', '\n')
            normalized_output = output.replace('\r\n', '\n').replace('\r', '\n')
            if normalized_output != expected_output:
                return ('fail', 'output mismatch', [f"expected: {expected_output!r}",
                                                    f"got:      {normalized_output!r}"])
        return ('pass', '', None)

    def _run_rgen_error_test_internal(self, test_file: str, expected_file: str,
                                      rs_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Verify that an unsupported Rust construct fails during emission."""
        if not os.path.isfile(expected_file):
            return ('skip', 'no .expected', None)

        exit_code, stdout, stderr, decode_error = run_with_timeout(
            [self.compiler, test_file, '--emit-rust', '-o', rs_file,
             '-l', '1', '-O0', '--no-install'],
            self.compile_timeout, env=self.env
        )
        if decode_error:
            return ('fail', 'subprocess output decode error',
                    [decode_error, format_subprocess_failure(stdout, stderr)])
        if exit_code == 0:
            return ('fail', 'Rust emission should fail', None)

        with open(expected_file, 'r', encoding='utf-8') as expected:
            expected_error = expected.readline().strip()
        if expected_error in stderr:
            return ('pass', '', None)
        details = [f"Expected: {expected_error}", "Got:"]
        details.extend(f"  {line}" for line in stderr.split('\n')[:15])
        return ('fail', 'wrong error', details)

    def _run_rust_native_parity_test_internal(
            self, test_file: str, expected_file: Optional[str],
            exe_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Compile and execute one unchanged tagged source with both backends."""
        if expected_file is not None and not os.path.isfile(expected_file):
            return ('skip', 'no .expected', None)
        if expected_file is None:
            expected_output = ''
        else:
            with open(expected_file, 'r', encoding='utf-8') as expected:
                expected_output = expected.read().replace('\r\n', '\n').replace('\r', '\n')

        outputs = {}
        for target in ('c', 'rust'):
            target_exe = f'{exe_file}.{target}{get_exe_extension()}'
            exit_code, stdout, stderr, decode_error = run_with_timeout(
                [self.compiler, test_file, '--target', target, '-o', target_exe,
                 '-l', '1', '--no-install'], self.compile_timeout, env=self.env)
            if decode_error:
                return ('fail', f'{target} subprocess output decode error',
                        [decode_error, format_subprocess_failure(stdout, stderr)])
            if exit_code != 0:
                return ('fail', f'{target} compile error',
                        stderr.split('\n')[:50] if stderr else None)
            exit_code, output, timeout_marker, decode_error = run_with_timeout(
                [target_exe], self.run_timeout, env=self.env, merge_stderr=True)
            if decode_error:
                return ('fail', f'{target} run output decode error', [decode_error, output])
            if timeout_marker == 'TIMEOUT':
                return ('fail', f'{target} run timeout', output.split('\n')[:20] if output else None)
            if exit_code != 0:
                details = [f'exit code: {exit_code}']
                if output:
                    details.extend(output.split('\n')[:20])
                return ('fail', f'{target} run error', details)
            outputs[target] = output.replace('\r\n', '\n').replace('\r', '\n')
        if outputs['c'] != outputs['rust']:
            return ('fail', 'C/Rust output mismatch',
                    [f"C:    {outputs['c']!r}", f"Rust: {outputs['rust']!r}"])
        if outputs['rust'] != expected_output:
            return ('fail', 'output mismatch',
                    [f'expected: {expected_output!r}', f"got:      {outputs['rust']!r}"])
        return ('pass', '', None)

    def _run_rust_native_target_test_internal(
            self, test_file: str, expected_file: str,
            exe_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Compile and execute a post-tag Rust-native fixture with its oracle."""
        if not os.path.isfile(expected_file):
            return ('skip', 'no .expected', None)
        with open(expected_file, 'r', encoding='utf-8') as expected:
            wanted = expected.read().replace('\r\n', '\n').replace('\r', '\n')
        base = os.path.splitext(test_file)[0]
        expected_exit = 0
        exit_file = base + '.exit-code'
        if os.path.isfile(exit_file):
            try:
                with open(exit_file, 'r', encoding='utf-8') as expected:
                    expected_exit = int(expected.read().strip())
            except (OSError, ValueError) as error:
                return ('fail', 'invalid .exit-code', [str(error)])
        targets = ('c', 'rust') if os.path.isfile(base + '.c-parity') else ('rust',)
        outputs = {}
        for target in targets:
            target_exe = f'{exe_file}.{target}{get_exe_extension()}'
            exit_code, stdout, stderr, decode_error = run_with_timeout(
                [self.compiler, test_file, '--target', target, '-o', target_exe,
                 '-l', '1', '--no-install'], self.compile_timeout, env=self.env)
            if decode_error:
                return ('fail', f'{target} subprocess output decode error',
                        [decode_error, format_subprocess_failure(stdout, stderr)])
            if exit_code != 0:
                return ('fail', f'{target} compile error',
                        stderr.split('\n')[:50] if stderr else None)
            exit_code, output, timeout_marker, decode_error = run_with_timeout(
                [target_exe], self.run_timeout, env=self.env, merge_stderr=True)
            if decode_error:
                return ('fail', f'{target} run output decode error',
                        [decode_error, output])
            if timeout_marker == 'TIMEOUT':
                return ('fail', f'{target} run timeout',
                        output.split('\n')[:20] if output else None)
            if exit_code != expected_exit:
                return ('fail', f'{target} run exit code: {exit_code}',
                        [f'expected: {expected_exit}',
                         *(output.split('\n')[:20] if output else [])])
            outputs[target] = output.replace('\r\n', '\n').replace('\r', '\n')
            if outputs[target] != wanted:
                return ('fail', f'{target} output mismatch',
                        [f'expected: {wanted!r}', f'got:      {outputs[target]!r}'])
        if len(outputs) == 2 and outputs['c'] != outputs['rust']:
            return ('fail', 'C/Rust output mismatch',
                    [f"C:    {outputs['c']!r}", f"Rust: {outputs['rust']!r}"])
        return ('pass', '', None)

    def _run_rust_native_error_test_internal(
            self, test_file: str, expected_file: str,
            exe_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Require the precise Rust native scalar validation diagnostic."""
        if not os.path.isfile(expected_file):
            return ('skip', 'no .expected', None)
        exit_code, stdout, stderr, decode_error = run_with_timeout(
            [self.compiler, test_file, '--target', 'rust', '-o', exe_file,
             '-l', '1', '--no-install'], self.compile_timeout, env=self.env)
        if decode_error:
            return ('fail', 'subprocess output decode error',
                    [decode_error, format_subprocess_failure(stdout, stderr)])
        if exit_code == 0:
            return ('fail', 'Rust native compilation should fail', None)
        with open(expected_file, 'r', encoding='utf-8') as expected:
            expected_error = expected.readline().strip()
        if expected_error in stderr:
            return ('pass', '', None)
        details = [f'Expected: {expected_error}', 'Got:']
        details.extend(f'  {line}' for line in stderr.split('\n')[:15])
        return ('fail', 'wrong error', details)


def main():
    parser = argparse.ArgumentParser(
        description='Unified cross-platform test runner for Sindarin compiler'
    )
    parser.add_argument('test_type', nargs='?', default='all',
                        choices=['rgen', 'rgen-errors', 'rust-native-tagged',
                                 'rust-native-extra', 'rust-native-origin',
                                 'rust-native-errors', 'rust-toolchain', 'all'],
                       help='Type of tests to run')
    parser.add_argument('--compiler', '-c', help='Path to compiler executable')
    parser.add_argument('--timeout', type=int, default=60,
                       help='Compile timeout in seconds')
    parser.add_argument('--run-timeout', type=int, default=30,
                       help='Run timeout in seconds')
    parser.add_argument('--exclude', help='Comma-separated list of tests to exclude')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Show detailed output')
    parser.add_argument('--no-color', action='store_true',
                       help='Disable colored output')
    parser.add_argument('--parallel', '-j', type=int, default=(os.cpu_count() or 2),
                       help=f'Number of parallel test workers (default: {(os.cpu_count() or 2)})')
    parser.add_argument('--no-cleanup', action='store_true',
                       help='Skip cleanup of orphaned temp directories')
    parser.add_argument('--filter', '-f', help='Only run tests matching this substring')

    args = parser.parse_args()

    # Setup signal handlers for graceful cleanup
    setup_signal_handlers()

    # Clean up orphaned temp directories from previous runs
    if not args.no_cleanup:
        cleanup_orphaned_temp_dirs()

    # Handle color
    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    # Handle Windows color support
    if is_windows():
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except:
            Colors.disable()

    # Parse excluded tests
    excluded = []
    if args.exclude:
        excluded = [t.strip() for t in args.exclude.split(',')]

    # Also check environment variable
    env_exclude = os.environ.get('SN_EXCLUDE_TESTS', '')
    if env_exclude:
        excluded.extend(env_exclude.split())

    try:
        compiler = find_compiler(args.compiler)
    except FileNotFoundError as e:
        print(f"{Colors.RED}Error:{Colors.NC} {e}")
        sys.exit(1)

    print(f"Compiler: {compiler}")
    print(f"Platform: {platform.system()}")
    if args.parallel > 1:
        print(f"Parallel: {args.parallel} workers")

    all_passed = True
    total_elapsed = 0.0

    with TestRunner(compiler, args.timeout, args.run_timeout,
                    excluded, args.verbose, args.parallel, args.filter) as runner:
        if args.test_type == 'all':
            for test_type in ['rgen', 'rgen-errors', 'rust-native-tagged',
                              'rust-native-extra', 'rust-native-origin',
                              'rust-native-errors']:
                passed, elapsed = runner.run_sn_tests(test_type)
                all_passed &= passed
                total_elapsed += elapsed
            passed, elapsed = runner.run_rust_toolchain_tests()
            all_passed &= passed
            total_elapsed += elapsed
        elif args.test_type == 'rust-toolchain':
            passed, elapsed = runner.run_rust_toolchain_tests()
            all_passed = passed
            total_elapsed = elapsed
        else:
            passed, elapsed = runner.run_sn_tests(args.test_type)
            all_passed = passed
            total_elapsed = elapsed

    print()
    time_str = TestRunner._format_elapsed(total_elapsed)
    if all_passed:
        print(f"{Colors.GREEN}{Colors.BOLD}All tests passed!{Colors.NC}  (total: {time_str})")
        sys.exit(0)
    else:
        print(f"{Colors.RED}{Colors.BOLD}Some tests failed!{Colors.NC}  (total: {time_str})")
        sys.exit(1)


if __name__ == '__main__':
    main()
