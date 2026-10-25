#!/usr/bin/env python3
"""Verify unchanged integer match gaps, C expression widths and compiler boundary adapters."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from integer_match_controls import ORIGINALS, CONTROL, BACKING, EXPECTED

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
    inputs.append(('independent-native-boundaries', CONTROL.encode(), BACKING.encode(), EXPECTED))
    report = {'passed': False, 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'scripts_sha256': {n: hashlib.sha256((ROOT / 'scripts' / n).read_bytes()).hexdigest()
                                for n in ('check_rust_integer_match_patterns.py', 'integer_match_controls.py')},
              'original_sources': len(ORIGINALS), 'cases': [],
              'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('rust-integer-match-patterns' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    def run(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        return {'status': p.returncode, 'stdout_hex': p.stdout.hex(), 'stderr_hex': p.stderr.hex()}
    with tempfile.TemporaryDirectory(prefix='sn-integer-match-patterns-') as tmp:
        app = Path(tmp)
        for name, source, backing, expected in inputs:
            if os.name == 'nt': expected = expected.replace(b'\n', b'\r\n')
            for opt, mode in modes:
                case = {'name': name, 'source_sha256': hashlib.sha256(source).hexdigest(),
                        'backing_sha256': hashlib.sha256(backing).hexdigest(),
                        'opt': opt, 'mode': mode, 'expected_stdout_hex': expected.hex(), 'targets': {}}
                for target in ('c', 'rust'):
                    # Process IDs can be reused while earlier generated files
                    # remain. Each compile gets its own evidence directory.
                    case_app = app / f'case-{len(report["cases"])}-{target}'
                    case_app.mkdir()
                    (case_app / 'main.sn').write_bytes(source)
                    (case_app / 'main.sn.c').write_bytes(backing)
                    exe = case_app / (target + '.exe')
                    command = [compiler, 'main.sn', '--no-install', '--keep-generated', '--target', target, opt, '-o', exe]
                    if mode != 'default': command.append('--' + mode)
                    result = {'compile': run(command, case_app)}
                    if result['compile']['status'] == 0: result['run'] = run([exe], case_app)
                    result['C_boundary_helpers'] = [p.read_text() for p in sorted(case_app.rglob('sn_integer_matches.c'))]
                    case['targets'][target] = result
                report['cases'].append(case)
                output.write_text(json.dumps(report, indent=2) + '\n')
                for target, result in case['targets'].items():
                    if result['compile']['status'] or result['run']['status'] or result['run']['stdout_hex'] != expected.hex() or result['run']['stderr_hex']:
                        raise ValueError(f'{name}/{opt}/{mode}/{target}: {result!r}')
                if name == 'independent-native-boundaries' and not case['targets']['rust']['C_boundary_helpers']:
                    raise ValueError('Compiler-dependent comparisons must retain their generated C support')
                print(f'PASS {name}/{opt}/{mode}', flush=True)
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(report["cases"])} C/Rust integer match literal pairs')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
