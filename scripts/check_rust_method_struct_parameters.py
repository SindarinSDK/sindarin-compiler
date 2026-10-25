#!/usr/bin/env python3
"""Verify plain value-record method parameter loans, local copies and indexed places."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from method_struct_parameter_controls import CONTROLS

ORIGINALS = [('tests/rgen/errors/instance_as_ref_struct_parameter.sn', '88f82352d4542e936d2736f018aa116a52a002c076cb110ecb17d570dece7138', '49c1fd66c9edbca3ebfc6c5a5c438882036d5da7923883e0730d4cee7424fb13', '330a'), ('tests/rgen/errors/mutating_instance_as_val_struct_parameter.sn', '2d6cb36524eab77b1aec2a71347810625426a16b94c01606ed9ae93bc09ace18', '5197e05befc5bbf61c3753004223fc3ff61ebfd8523db565f197a5972e9b89f8', '330a')]

ROOT = Path(__file__).resolve().parents[1]


def verify(quick=False):
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    inputs = []
    for rel, source_sha, oracle_sha, stdout_hex in ORIGINALS:
        path = ROOT / rel
        assert hashlib.sha256(path.read_bytes()).hexdigest() == source_sha, rel
        assert hashlib.sha256(path.with_suffix('.expected').read_bytes()).hexdigest() == oracle_sha, rel
        inputs.append((rel, path.read_bytes(), b'', bytes.fromhex(stdout_hex)))
    inputs.extend((name, source.encode(), backing.encode(), expected) for name, source, backing, expected in CONTROLS)
    report = {'passed': False, 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'scripts_sha256': {n: hashlib.sha256((ROOT / 'scripts' / n).read_bytes()).hexdigest()
                                for n in ('check_rust_method_struct_parameters.py', 'method_struct_parameter_controls.py')},
              'original_sources': len(ORIGINALS), 'cases': [],
              'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('rust-method-struct-parameters' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    def run(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        return {'status': p.returncode, 'stdout_hex': p.stdout.hex(), 'stderr_hex': p.stderr.hex()}
    with tempfile.TemporaryDirectory(prefix='sn-method-struct-parameters-') as tmp:
        app = Path(tmp)
        for name, source, backing, expected in inputs:
            (app / 'main.sn').write_bytes(source)
            (app / 'main.sn.c').write_bytes(backing)
            if os.name == 'nt': expected = expected.replace(b'\n', b'\r\n')
            for opt, mode in modes:
                case = {'name': name, 'source_sha256': hashlib.sha256(source).hexdigest(),
                        'backing_sha256': hashlib.sha256(backing).hexdigest(),
                        'opt': opt, 'mode': mode, 'expected_stdout_hex': expected.hex(), 'targets': {}}
                for target in ('c', 'rust'):
                    exe = app / (target + '.exe')
                    command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
                    if mode != 'default': command.append('--' + mode)
                    result = {'compile': run(command, app)}
                    if result['compile']['status'] == 0: result['run'] = run([exe], app)
                    case['targets'][target] = result
                report['cases'].append(case)
                output.write_text(json.dumps(report, indent=2) + '\n')
                for target, result in case['targets'].items():
                    if result['compile']['status'] or result['run']['status'] or result['run']['stdout_hex'] != expected.hex() or result['run']['stderr_hex']:
                        raise ValueError(f'{name}/{opt}/{mode}/{target}: {result!r}')
                print(f'PASS {name}/{opt}/{mode}', flush=True)
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(report["cases"])} C/Rust method struct parameter pairs')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
