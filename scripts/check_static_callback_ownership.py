#!/usr/bin/env python3
"""Verify retained static-call callbacks and release of argument temporaries."""
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile


ORACLES = {
    'tests/integration/test_fn_ref_static_method_arg.sn': b'world\n',
    'tests/integration/test_static_callback_temporary_ownership.sn':
        b'named named\ncaptured captured\nsecond\nfirst\n',
}


def execute(compiler, source, executable, optimization, mode, target, environment):
    command = [str(compiler), source, '--target', target, '--no-install',
               f'-O{optimization}', '-o', str(executable)]
    if mode != 'default':
        command.append('--' + mode)
    build = subprocess.run(command, capture_output=True, env=environment, timeout=120)
    case = {'source': source, 'source_sha256': hashlib.sha256(Path(source).read_bytes()).hexdigest(),
            'optimization': optimization, 'arithmetic_mode': mode, 'target': target,
            'compile_status': build.returncode, 'compile_stderr_hex': build.stderr.hex(),
            'passed': False}
    if build.returncode == 0:
        result = subprocess.run([str(executable)], capture_output=True, env=environment, timeout=10)
        expected = ORACLES[source]
        if os.name == 'nt':
            expected = expected.replace(b'\n', b'\r\n')
        case.update(run_status=result.returncode, stdout_hex=result.stdout.hex(),
                    stderr_hex=result.stderr.hex(),
                    expected_stdout_hex=expected.hex(),
                    passed=result.returncode == 0 and result.stdout == expected
                    and not result.stderr)
    if not case['passed']:
        print(json.dumps(case, sort_keys=True), flush=True)
    return case


def main():
    compiler = Path('bin/sn.exe' if os.name == 'nt' else 'bin/sn').resolve()
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'platform': platform.system(), 'cases': []}
    with tempfile.TemporaryDirectory(prefix='sn-static-callback-') as directory:
        executable = Path(directory) / 'program.exe'
        for source in ORACLES:
            for optimization in range(3):
                for mode in ('default', 'checked', 'unchecked'):
                    for target in ('c', 'rust'):
                        case = execute(compiler, source, executable, optimization, mode,
                                       target, os.environ.copy())
                        report['cases'].append(case)
                        print(f"{'PASS' if case['passed'] else 'FAIL'} {source} "
                              f"{target} -O{optimization} {mode}", flush=True)
                if platform.system() == 'Linux':
                    environment = os.environ.copy()
                    flags = (f'-g -O{optimization} -fsanitize=address,undefined '
                             '-fno-omit-frame-pointer')
                    environment.update(SN_CFLAGS=flags, SN_RELEASE_CFLAGS=flags,
                                       SN_LDLIBS='-fsanitize=address,undefined',
                                       ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                                       # Dead temporary pointers on the stack/registers can
                                       # hide the lost credit on aarch64. Require real owners.
                                       LSAN_OPTIONS='use_registers=0:use_stacks=0')
                    case = execute(compiler, source, executable, optimization, 'default',
                                   'c', environment)
                    case['sanitizers'] = ['address', 'undefined', 'leak']
                    case['lsan_options'] = environment['LSAN_OPTIONS']
                    report['cases'].append(case)
                    print(f"{'PASS' if case['passed'] else 'FAIL'} {source} "
                          f"-O{optimization} sanitizers", flush=True)
    report['passed'] = all(case['passed'] for case in report['cases'])
    report['count'] = len(report['cases'])
    destination = Path('.sn/static-callback-ownership.json')
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
