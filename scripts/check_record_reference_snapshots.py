#!/usr/bin/env python3
"""Check closure owners across record snapshots, replacement and nullable transport."""
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile

from check_rust_interface_oracles import PROBE_ORACLES, PROBE_SHA256

ORACLES = {
    'tests/integration/test_lambda_capture_record_ref_snapshot.sn': b'true\ntrue\ntrue\n',
    'tests/integration/test_managed_record_reference_snapshots.sn':
        b'1:original:2\n7:escaped:9\n3:changed:4\n5:later:6\n',
}

SOURCE_SHA256 = {'tests/integration/test_lambda_capture_record_ref_snapshot.sn': 'b53682939ad5f1e8ac6eec43b3af252da4cd004b1bbf8145d05ce87513e83d5b', 'tests/integration/test_managed_record_reference_snapshots.sn': '8980d4e55a85b1d9c94fa6d72d540c5e9db08d43975180a3dbdf254c73a26b04'}

for source in ('tests/rust-interfaces/named_callable_assignment.sn',
               'tests/rust-interfaces/nullable_callable_transport.sn'):
    ORACLES[source] = PROBE_ORACLES[source]
    SOURCE_SHA256[source] = PROBE_SHA256[source]


def execute(compiler, source, executable, target, optimization, mode, sanitized):
    environment = os.environ.copy()
    if sanitized:
        environment.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                           LSAN_OPTIONS='use_registers=0:use_stacks=0')
        if target == 'c':
            flags = (f'-O{optimization} -g -fsanitize=address,undefined '
                     '-fno-omit-frame-pointer')
            environment.update(SN_CFLAGS=flags, SN_RELEASE_CFLAGS=flags,
                               SN_LDLIBS='-fsanitize=address,undefined')
        else:
            environment.update(RUSTC_BOOTSTRAP='1', SN_RUSTFLAGS='-Zsanitizer=address')
    command = [str(compiler), source, '--target', target, '--no-install',
               f'-O{optimization}', '-o', str(executable)]
    if mode != 'default':
        command.append('--' + mode)
    build = subprocess.run(command, capture_output=True, env=environment, timeout=120)
    case = {'source': source, 'source_sha256': hashlib.sha256(Path(source).read_bytes()).hexdigest(),
            'target': target, 'optimization': optimization, 'arithmetic_mode': mode,
            'sanitized': sanitized, 'compile_status': build.returncode,
            'compile_stderr_hex': build.stderr.hex(), 'passed': False}
    expected = ORACLES[source].replace(b'\n', b'\r\n') if os.name == 'nt' else ORACLES[source]
    if build.returncode == 0:
        result = subprocess.run([str(executable)], capture_output=True, env=environment, timeout=10)
        case.update(run_status=result.returncode, stdout_hex=result.stdout.hex(),
                    stderr_hex=result.stderr.hex(), expected_stdout_hex=expected.hex(),
                    passed=result.returncode == 0 and result.stdout == expected and not result.stderr)
    print(f"{'PASS' if case['passed'] else 'FAIL'} {source} {target} "
          f"-O{optimization} {mode}{' sanitizers' if sanitized else ''}", flush=True)
    if not case['passed']:
        print(json.dumps(case, sort_keys=True), flush=True)
    return case


def main():
    compiler = Path('bin/sn.exe' if os.name == 'nt' else 'bin/sn').resolve()
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'platform': platform.system(), 'cases': []}
    with tempfile.TemporaryDirectory(prefix='sn-record-reference-captures-') as directory:
        executable = Path(directory) / 'program.exe'
        for source in ORACLES:
            if hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source]:
                raise ValueError(f"source changed: {source}")
            for optimization in range(3):
                for target in ('c', 'rust'):
                    for mode in ('default', 'checked', 'unchecked'):
                        report['cases'].append(execute(compiler, source, executable, target,
                                                       optimization, mode, False))
                    if platform.system() == 'Linux':
                        report['cases'].append(execute(compiler, source, executable, target,
                                                       optimization, 'default', True))
    report['passed'] = all(case['passed'] for case in report['cases'])
    report['count'] = len(report['cases'])
    destination = Path('.sn/record-reference-snapshots.json')
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
