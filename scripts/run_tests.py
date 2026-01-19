#!/usr/bin/env python3
"""
Unified cross-platform test runner for Sindarin compiler.

This script replaces the separate bash and PowerShell test runners with a single
Python implementation that works identically on Linux, macOS, and Windows.

Usage:
    python scripts/run_tests.py <test-type> [options]

Test types:
    unit              - Run unit tests (bin/tests executable)
    integration       - Run integration tests (tests/integration/*.sn)
    integration-errors - Run integration error tests (tests/integration/errors/*.sn)
    explore           - Run exploratory tests (tests/exploratory/test_*.sn)
    explore-errors    - Run exploratory error tests (tests/exploratory/errors/*.sn)
    sdk               - Run SDK tests (tests/sdk/test_*.sn)
    all               - Run all test suites

Options:
    --compiler PATH   - Path to compiler (default: bin/sn or bin/sn.exe)
    --timeout SEC     - Compile timeout in seconds (default: 10)
    --run-timeout SEC - Run timeout in seconds (default: 30)
    --exclude TESTS   - Comma-separated list of test names to exclude
    --verbose         - Show detailed output
    --no-color        - Disable colored output
    --parallel, -j N  - Run tests with N parallel workers (default: 1)
"""

import argparse
import glob
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import List, Optional, Tuple, Dict, Any

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
                     merge_stderr: bool = False) -> Tuple[int, str, str]:
    """Run a command with timeout, returning (exit_code, stdout, stderr).

    If merge_stderr is True, stderr is redirected to stdout (like bash 2>&1),
    and stderr in the return value will be empty.
    """
    try:
        if merge_stderr:
            # Merge stderr into stdout (like bash's 2>&1)
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=timeout,
                cwd=cwd,
                env=env
            )
            return result.returncode, result.stdout, ''
        else:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=cwd,
                env=env
            )
            return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, '', 'TIMEOUT'
    except Exception as e:
        return -1, '', str(e)


class TestConfig:
    """Configuration for a test type."""
    def __init__(self, test_dir: str, pattern: str, expect_compile_fail: bool, title: str):
        self.test_dir = test_dir
        self.pattern = pattern
        self.expect_compile_fail = expect_compile_fail
        self.title = title


TEST_CONFIGS = {
    'integration': TestConfig(
        'tests/integration', '*.sn', False, 'Integration Tests'
    ),
    'integration-errors': TestConfig(
        'tests/integration/errors', '*.sn', True, 'Integration Error Tests'
    ),
    'explore': TestConfig(
        'tests/exploratory', 'test_*.sn', False, 'Exploratory Tests'
    ),
    'explore-errors': TestConfig(
        'tests/exploratory/errors', '*.sn', True, 'Exploratory Error Tests'
    ),
    'sdk': TestConfig(
        'tests/sdk', '**/test_*.sn', False, 'SDK Tests'
    ),
}


class TestRunner:
    def __init__(self, compiler: str, compile_timeout: int = 10,
                 run_timeout: int = 30, excluded_tests: List[str] = None,
                 verbose: bool = False, parallel: int = 1):
        self.compiler = compiler
        self.compile_timeout = compile_timeout
        self.run_timeout = run_timeout
        self.excluded_tests = excluded_tests or []
        self.verbose = verbose
        self.parallel = parallel
        self.temp_dir = None
        self._progress_lock = threading.Lock()
        self._completed_count = 0
        self._total_count = 0

        # Setup environment
        self.env = os.environ.copy()
        # Set ASAN options to avoid leak detection issues
        if 'ASAN_OPTIONS' not in self.env:
            self.env['ASAN_OPTIONS'] = 'detect_leaks=0'

        # On Windows, add vcpkg DLL directories to PATH for runtime linking
        if is_windows():
            vcpkg_bins = [
                os.path.join('vcpkg', 'installed', 'x64-windows', 'bin'),
                os.path.join('vcpkg', 'installed', 'x64-mingw-dynamic', 'bin'),
            ]
            for vcpkg_bin in vcpkg_bins:
                if os.path.isdir(vcpkg_bin):
                    abs_bin = os.path.abspath(vcpkg_bin)
                    self.env['PATH'] = abs_bin + os.pathsep + self.env.get('PATH', '')

    def __enter__(self):
        self.temp_dir = tempfile.mkdtemp(prefix='sn_test_')
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.temp_dir and os.path.exists(self.temp_dir):
            shutil.rmtree(self.temp_dir, ignore_errors=True)

    def run_unit_tests(self) -> bool:
        """Run unit tests."""
        print()
        print(f"{Colors.BOLD}Unit Tests{Colors.NC}")
        print("=" * 60)

        exe_ext = get_exe_extension()
        test_binary = f'bin/tests{exe_ext}'

        if not os.path.isfile(test_binary):
            print(f"{Colors.RED}FAIL{Colors.NC}: Test binary not found: {test_binary}")
            return False

        # Use absolute path for Windows subprocess compatibility
        test_binary = os.path.abspath(test_binary)

        exit_code, stdout, stderr = run_with_timeout(
            [test_binary], self.run_timeout, env=self.env
        )

        if stdout:
            print(stdout)
        if stderr and exit_code != 0:
            print(stderr)

        print("-" * 60)
        if exit_code == 0:
            print(f"{Colors.GREEN}Unit tests passed{Colors.NC}")
            return True
        else:
            print(f"{Colors.RED}Unit tests failed{Colors.NC}")
            return False

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
                'details': None
            }
        else:
            expected_file = test_file.replace('.sn', '.expected')
            panic_file = test_file.replace('.sn', '.panic')

            if config.expect_compile_fail:
                status, reason, details = self._run_error_test_internal(
                    test_file, expected_file, exe_file
                )
            else:
                status, reason, details = self._run_positive_test_internal(
                    test_file, expected_file, panic_file, exe_file, test_type
                )

            result = {
                'test_name': test_name,
                'status': status,
                'reason': reason,
                'details': details
            }

        # Update progress counter
        with self._progress_lock:
            self._completed_count += 1
            if self.parallel > 1 and sys.stdout.isatty():
                # Show progress indicator for parallel runs (only on TTY)
                sys.stdout.write(f"\r  [{self._completed_count}/{self._total_count}] Running tests...    ")
                sys.stdout.flush()

        return result

    def run_sn_tests(self, test_type: str) -> bool:
        """Run Sindarin source file tests."""
        config = TEST_CONFIGS.get(test_type)
        if not config:
            print(f"Unknown test type: {test_type}")
            return False

        print()
        print(f"{Colors.BOLD}{config.title}{Colors.NC}")
        print("=" * 60)

        # Find test files
        pattern = os.path.join(config.test_dir, config.pattern)
        test_files = sorted(glob.glob(pattern, recursive=True))

        if not test_files:
            print(f"No test files found matching: {pattern}")
            return True

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
                test_name = info['test_name']
                print(f"  {test_name:45} ", end='', flush=True)
                result = self._run_single_test(info)
                results.append(result)
                # Print result immediately in sequential mode
                self._print_test_result(result)

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

        print()
        print("-" * 60)
        print(f"Results: {Colors.GREEN}{passed} passed{Colors.NC}, "
              f"{Colors.RED}{failed} failed{Colors.NC}, "
              f"{Colors.YELLOW}{skipped} skipped{Colors.NC}")

        return failed == 0

    def _print_test_result(self, result: Dict[str, Any], include_name: bool = False):
        """Print a single test result."""
        test_name = result['test_name']
        status = result['status']
        reason = result['reason']
        details = result['details']

        if include_name:
            print(f"  {test_name:45} ", end='')

        if status == 'pass':
            print(f"{Colors.GREEN}PASS{Colors.NC}")
        elif status == 'skip':
            print(f"{Colors.YELLOW}SKIP{Colors.NC} ({reason})")
        else:
            print(f"{Colors.RED}FAIL{Colors.NC} ({reason})")
            if self.verbose and details:
                for line in details[:3]:
                    print(f"    {line}")

    def _run_error_test_internal(self, test_file: str, expected_file: str,
                                    exe_file: str) -> Tuple[str, str, Optional[List[str]]]:
        """Run a test that should fail to compile. Returns (status, reason, details)."""
        if not os.path.isfile(expected_file):
            return ('skip', 'no .expected', None)

        # Try to compile (should fail)
        exit_code, stdout, stderr = run_with_timeout(
            [self.compiler, test_file, '-o', exe_file, '-l', '1'],
            self.compile_timeout, env=self.env
        )

        if exit_code == 0:
            return ('fail', 'should not compile', None)

        # Check error message
        with open(expected_file, 'r') as f:
            expected_error = f.readline().strip()

        if expected_error in stderr:
            return ('pass', '', None)
        else:
            details = [
                f"Expected: {expected_error}",
                f"Got:      {stderr.split(chr(10))[0] if stderr else '(empty)'}"
            ]
            return ('fail', 'wrong error', details)

    def _run_positive_test_internal(self, test_file: str, expected_file: str,
                                     panic_file: str, exe_file: str,
                                     test_type: str) -> Tuple[str, str, Optional[List[str]]]:
        """Run a test that should compile and run successfully. Returns (status, reason, details)."""
        has_expected = os.path.isfile(expected_file)
        expects_panic = os.path.isfile(panic_file)

        # For non-explore tests, require .expected file
        if not has_expected and test_type not in ('explore', 'sdk'):
            return ('skip', 'no .expected', None)

        # Standard compilation (use #pragma source for C helper files)
        compile_cmd = [self.compiler, test_file, '-o', exe_file, '-l', '1', '-O0']
        if not is_windows():
            compile_cmd.append('-g')
        exit_code, stdout, stderr = run_with_timeout(
            compile_cmd, self.compile_timeout, env=self.env
        )

        if exit_code != 0:
            details = stderr.split('\n')[:3] if stderr else None
            return ('fail', 'compile error', details)

        # Run with merged stdout/stderr (like bash's 2>&1)
        run_timeout = 5 if test_type == 'integration' else self.run_timeout
        exit_code, output, timeout_marker = run_with_timeout(
            [exe_file], run_timeout, env=self.env, merge_stderr=True
        )

        # Check for expected panic
        if expects_panic:
            if exit_code == 0:
                return ('fail', 'expected panic', None)
        else:
            if exit_code != 0:
                if timeout_marker == 'TIMEOUT':
                    return ('fail', 'timeout', output.split('\n')[:3] if output else None)
                else:
                    details = output.split('\n')[:3] if output else None
                    return ('fail', f'exit code: {exit_code}', details)

        # Compare output if expected file exists
        if has_expected:
            with open(expected_file, 'r') as f:
                expected_output = f.read()

            # Normalize line endings for cross-platform comparison (CRLF -> LF)
            normalized_output = output.replace('\r\n', '\n').replace('\r', '\n')
            normalized_expected = expected_output.replace('\r\n', '\n').replace('\r', '\n')

            if normalized_output == normalized_expected:
                return ('pass', '', None)
            else:
                details = [
                    f"Expected: {expected_output.split(chr(10))[0]}",
                    f"Got:      {output.split(chr(10))[0] if output else '(empty)'}"
                ]
                return ('fail', 'output mismatch', details)

        return ('pass', '', None)


def main():
    parser = argparse.ArgumentParser(
        description='Unified cross-platform test runner for Sindarin compiler'
    )
    parser.add_argument('test_type', nargs='?', default='all',
                       choices=['unit', 'integration', 'integration-errors',
                               'explore', 'explore-errors', 'sdk', 'all'],
                       help='Type of tests to run')
    parser.add_argument('--compiler', '-c', help='Path to compiler executable')
    parser.add_argument('--timeout', type=int, default=10,
                       help='Compile timeout in seconds')
    parser.add_argument('--run-timeout', type=int, default=30,
                       help='Run timeout in seconds')
    parser.add_argument('--exclude', help='Comma-separated list of tests to exclude')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Show detailed output')
    parser.add_argument('--no-color', action='store_true',
                       help='Disable colored output')
    parser.add_argument('--parallel', '-j', type=int, default=os.cpu_count() or 4,
                       help=f'Number of parallel test workers (default: {os.cpu_count() or 4})')

    args = parser.parse_args()

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

    with TestRunner(compiler, args.timeout, args.run_timeout,
                    excluded, args.verbose, args.parallel) as runner:
        if args.test_type == 'all':
            # Run all test types
            all_passed &= runner.run_unit_tests()
            for test_type in ['integration', 'integration-errors',
                             'explore', 'explore-errors', 'sdk']:
                all_passed &= runner.run_sn_tests(test_type)
        elif args.test_type == 'unit':
            all_passed = runner.run_unit_tests()
        else:
            all_passed = runner.run_sn_tests(args.test_type)

    print()
    if all_passed:
        print(f"{Colors.GREEN}{Colors.BOLD}All tests passed!{Colors.NC}")
        sys.exit(0)
    else:
        print(f"{Colors.RED}{Colors.BOLD}Some tests failed!{Colors.NC}")
        sys.exit(1)


if __name__ == '__main__':
    main()
