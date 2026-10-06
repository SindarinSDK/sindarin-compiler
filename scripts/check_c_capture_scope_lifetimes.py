#!/usr/bin/env python3
"""Check capture scope cleanup with ASAN, UBSAN and leak detection on Linux."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from check_rust_capture_scope_oracles import ORACLES, SOURCE_SHA256


def main():
    compiler = Path('bin/sn').resolve()
    environment = os.environ.copy()
    environment['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
    environment['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'sanitizers': ['address', 'undefined', 'leak'], 'cases': []}
    with tempfile.TemporaryDirectory(prefix='sn-array-return-lifetimes-') as directory:
        for source, expected in ORACLES.items():
            digest = hashlib.sha256(Path(source).read_bytes()).hexdigest()
            if digest != SOURCE_SHA256[source]:
                raise ValueError(f'source changed: {source}')
            for optimization in range(3):
                flags = (f'-g -O{optimization} -fsanitize=address,undefined '
                         '-fno-omit-frame-pointer')
                environment['SN_CFLAGS'] = flags
                environment['SN_RELEASE_CFLAGS'] = flags
                environment['SN_LDLIBS'] = '-fsanitize=address,undefined'
                output = str(Path(directory) / 'program')
                command = [str(compiler), source, '--target', 'c', '--no-install',
                           f'-O{optimization}', '-o', output]
                build = subprocess.run(command, capture_output=True,
                                       env=environment, timeout=120)
                case = {'source': source, 'source_sha256': digest,
                        'optimization': f'-O{optimization}', 'flags': flags,
                        'compile_status': build.returncode,
                        'compile_stdout_hex': build.stdout.hex(),
                        'compile_stderr_hex': build.stderr.hex(), 'passed': False}
                if build.returncode == 0:
                    run = subprocess.run([output], capture_output=True,
                                         env=environment, timeout=10)
                    case.update(run_status=run.returncode,
                                stdout_hex=run.stdout.hex(), stderr_hex=run.stderr.hex(),
                                passed=run.returncode == 0 and run.stdout == expected
                                and not run.stderr)
                report['cases'].append(case)
                print(('PASS' if case['passed'] else 'FAIL') +
                      f' {source} -O{optimization} sanitizers', flush=True)
    report['passed'] = all(case['passed'] for case in report['cases'])
    report['independent_oracle_cases'] = len(report['cases'])
    destination = Path('.sn/c-capture-scope-lifetimes.json')
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
