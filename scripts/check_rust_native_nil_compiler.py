#!/usr/bin/env python3
"""Repeatedly compile the unchanged native nil-result regression; never retry away failures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import time

from check_rust_native_callback_oracles import ORACLES, SOURCE_SHA256

SOURCE = 'tests/rust-native/scalar_native_callback_native_nil_result.sn'


def crash_diagnostics(command, started, destination, environment):
    """Keep the failed status, collecting a separate debugger run only as evidence."""
    files = []
    destination.mkdir(parents=True, exist_ok=True)
    if platform.system() == 'Darwin':
        for root in (Path.home() / 'Library/Logs/DiagnosticReports',
                     Path('/Library/Logs/DiagnosticReports')):
            if not root.exists():
                continue
            for candidate in root.glob('sn*.ips'):
                try:
                    if candidate.stat().st_mtime < started:
                        continue
                    content = candidate.read_bytes()
                except OSError:
                    continue
                target = destination / (hashlib.sha256(content).hexdigest() + '.ips')
                target.write_bytes(content)
                files.append(str(target))
        debugger = shutil.which('lldb')
        if debugger:
            try:
                run = subprocess.run([debugger, '--batch', '-o', 'run', '-o',
                                      'thread backtrace all', '--', *command],
                                     capture_output=True, env=environment, timeout=120)
                target = destination / 'lldb.log'
                target.write_bytes(run.stdout + run.stderr)
                files.append(str(target))
            except subprocess.TimeoutExpired as error:
                target = destination / 'lldb.log'
                target.write_bytes((error.stdout or b'') + (error.stderr or b''))
                files.append(str(target))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', type=int, default=2)
    parser.add_argument('--compiler', type=Path,
                        default=Path('bin/sn.exe' if os.name == 'nt' else 'bin/sn'))
    parser.add_argument('--output', type=Path,
                        default=Path('.sn/rust-parity-native-nil-compiler.json'))
    args = parser.parse_args()
    if args.repeat < 1:
        parser.error('--repeat must be positive')
    source_hash = hashlib.sha256(Path(SOURCE).read_bytes()).hexdigest()
    if source_hash != SOURCE_SHA256[SOURCE]:
        raise ValueError('native nil-result source changed')
    if Path(SOURCE).with_suffix('.expected').read_bytes() != ORACLES[SOURCE]:
        raise ValueError('native nil-result oracle changed')
    compiler = args.compiler.resolve()
    expected = ORACLES[SOURCE]
    if os.name == 'nt':
        expected = expected.replace(b'\n', b'\r\n')
    environment = os.environ.copy()
    environment['SN_COMPILER_BACKTRACE'] = '1'
    if platform.system() == 'Darwin':
        environment.update(MallocScribble='1', MallocPreScribble='1')
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'source': SOURCE, 'source_sha256': source_hash,
              'platform': platform.system(), 'repeat': args.repeat, 'cases': [],
              'environment': {key: environment.get(key) for key in
                              ('SN_COMPILER_BACKTRACE', 'MallocScribble', 'MallocPreScribble')}}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sn-native-nil-compiler-') as folder:
        executable = Path(folder) / 'program.exe'
        for repeat in range(args.repeat):
            for level in range(3):
                for mode in ('default', 'checked', 'unchecked'):
                    command = [str(compiler), '--no-install', '--target', 'rust',
                               SOURCE, f'-O{level}', '-o', str(executable)]
                    if mode != 'default':
                        command.append('--' + mode)
                    started = time.time()
                    build = subprocess.run(command, capture_output=True,
                                           env=environment, timeout=120)
                    case = {'repeat': repeat, 'optimization': level,
                            'arithmetic_mode': mode, 'compile_status': build.returncode,
                            'compile_stdout_hex': build.stdout.hex(),
                            'compile_stderr_hex': build.stderr.hex(), 'passed': False}
                    if build.returncode == 0:
                        run = subprocess.run([str(executable)], capture_output=True,
                                             env=environment, timeout=10)
                        case.update(run_status=run.returncode, stdout_hex=run.stdout.hex(),
                                    stderr_hex=run.stderr.hex(), expected_stdout_hex=expected.hex(),
                                    passed=run.returncode == 0 and run.stdout == expected and not run.stderr)
                    if build.returncode < 0:
                        case['crash_diagnostics'] = crash_diagnostics(
                            command, started, Path('.sn/ci-reports/compiler-crashes/native-nil'), environment)
                    report['cases'].append(case)
                    report['passed'] = all(case['passed'] for case in report['cases'])
                    args.output.write_text(json.dumps(report, indent=2) + '\n')
                    print(f"{'PASS' if case['passed'] else 'FAIL'} native nil compiler "
                          f"repeat={repeat} -O{level} {mode}", flush=True)
                    if not case['passed']:
                        print(json.dumps(case), flush=True)
                        return 1
    print(f"PASS: {len(report['cases'])} repeated native nil compiler regressions")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
