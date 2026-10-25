#!/usr/bin/env python3
"""Check borrowed string concatenation arguments and preserve C evaluation order."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile

from string_call_argument_controls import SOURCE, EXPECTED, ORDER_SOURCE, ORDER_BACKING, ORDER_REFERENCE_MAIN

ROOT = Path(__file__).resolve().parents[1]


def verify(quick=False):
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    report = {'passed': False, 'cases': [], 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'scope': 'C/Rust caller and callee string ownership; C-only order preservation against an independent reference compiled with the same C compiler',
              'scripts_sha256': {n: hashlib.sha256((ROOT / 'scripts' / n).read_bytes()).hexdigest()
                                for n in ('check_rust_string_call_arguments.py', 'string_call_argument_controls.py')},
              'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('rust-string-call-arguments' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)

    def run(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        return {'command': [str(x) for x in command], 'status': p.returncode,
                'stdout_hex': p.stdout.hex(), 'stderr_hex': p.stderr.hex()}

    def execute(app, source, backing, target, opt, mode):
        (app / 'main.sn').write_text(source)
        (app / 'main.sn.c').write_text(backing)
        exe = app / (target + '.exe')
        command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
        if mode != 'default': command.append('--' + mode)
        result = {'compile': run(command, app)}
        if result['compile']['status'] == 0: result['run'] = run([exe], app)
        return result

    def clean(result, expected):
        return result['compile']['status'] == 0 and (
            result['run']['status'], result['run']['stdout_hex'], result['run']['stderr_hex']) == (0, expected.hex(), '')

    cc = shlex.split(os.environ.get('SN_CC') or ('clang' if os.name == 'nt' or platform.system() == 'Darwin' else 'gcc'))
    runtime = ROOT / 'bin/lib' / ('clang' if os.name == 'nt' else 'gcc') / 'libsn_runtime_min.a'
    with tempfile.TemporaryDirectory(prefix='sn-string-call-arguments-') as tmp:
        app = Path(tmp)
        for opt, mode in modes:
            expected = EXPECTED.replace(b'\n', b'\r\n') if os.name == 'nt' else EXPECTED
            case = {'name': 'caller-and-callee-loans', 'opt': opt, 'mode': mode,
                    'source_sha256': hashlib.sha256(SOURCE.encode()).hexdigest(), 'targets': {}}
            for target in ('c', 'rust'):
                case['targets'][target] = execute(app, SOURCE, '', target, opt, mode)
            case['passed'] = all(clean(r, expected) for r in case['targets'].values())
            case['expected_stdout_hex'] = expected.hex()
            report['cases'].append(case)
            output.write_text(json.dumps(report, indent=2) + '\n')
            if not case['passed']: raise ValueError(json.dumps(case))

            reference = app / 'reference.c'
            reference.write_text(ORDER_BACKING + ORDER_REFERENCE_MAIN)
            exe = app / 'reference.exe'
            command = cc + ['-std=c11', '-D_GNU_SOURCE', '-DSTRING_ARGUMENT_REFERENCE', opt,
                            '-I' + str(ROOT / 'bin/include/runtime'), str(reference), str(runtime), '-lm', '-o', str(exe)]
            command += shlex.split(os.environ.get('SN_CFLAGS', ''))
            ref = {'compile': run(command, app)}
            if ref['compile']['status'] == 0: ref['run'] = run([exe], app)
            result = execute(app, ORDER_SOURCE, ORDER_BACKING, 'c', opt, mode)
            case = {'name': 'C-argument-order', 'opt': opt, 'mode': mode, 'reference': ref, 'generated': result}
            expected = bytes.fromhex(ref.get('run', {}).get('stdout_hex', ''))
            case['passed'] = bool(expected) and clean(ref, expected) and clean(result, expected)
            report['cases'].append(case)
            output.write_text(json.dumps(report, indent=2) + '\n')
            if not case['passed']: raise ValueError(json.dumps(case))
            print('PASS', opt, mode, flush=True)
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(report["cases"])} string argument ownership/order cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
