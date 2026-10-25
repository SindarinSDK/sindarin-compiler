#!/usr/bin/env python3
"""Verify native array field storage, element families and copy/header identity."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from native_array_field_controls import family_controls
from native_array_field_effects import SOURCE, BACKING, EXPECTED

ROOT = Path(__file__).resolve().parents[1]


def verify(quick=False):
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    report = {'passed': False, 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'cases': [], 'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')},
              'scripts_sha256': {n: hashlib.sha256((ROOT / 'scripts' / n).read_bytes()).hexdigest()
                                for n in ('check_rust_native_array_fields.py', 'native_array_field_controls.py', 'native_array_field_contexts.py', 'native_array_field_effects.py')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('rust-native-array-fields' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    def checked(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        return {'status': p.returncode, 'stdout_hex': p.stdout.hex(), 'stderr_hex': p.stderr.hex()}
    with tempfile.TemporaryDirectory(prefix='sn-native-field-families-') as tmp:
        app = Path(tmp)
        for name, source, lines in family_controls() + [('copy-and-index-effects', SOURCE, EXPECTED)]:
            (app / 'main.sn').write_text(source)
            (app / 'main.sn.c').write_text(BACKING if name == 'copy-and-index-effects' else '')
            expected = lines if isinstance(lines, bytes) else b'true\n' * lines
            if os.name == 'nt': expected = expected.replace(b'\n', b'\r\n')
            for opt, mode in modes:
                case = {'name': name, 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                        'opt': opt, 'mode': mode, 'expected_stdout_hex': expected.hex(), 'targets': {}}
                for target in ('c', 'rust'):
                    exe = app / (target + '.exe')
                    command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
                    if mode != 'default': command.append('--' + mode)
                    result = {'compile': checked(command, app)}
                    if result['compile']['status'] == 0: result['run'] = checked([exe], app)
                    case['targets'][target] = result
                report['cases'].append(case)
                output.write_text(json.dumps(report, indent=2) + '\n')
                for target, result in case['targets'].items():
                    if result['compile']['status'] or result['run']['status'] or result['run']['stdout_hex'] != expected.hex() or result['run']['stderr_hex']:
                        raise ValueError(f'{name}/{opt}/{mode}/{target}: {result!r}')
                print(f'PASS {name}/{opt}/{mode}', flush=True)
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(report["cases"])} C/Rust native array field pairs')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
