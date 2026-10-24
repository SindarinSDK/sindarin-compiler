#!/usr/bin/env python3
"""Check lazy scalar match comparisons against fixed C/Rust output oracles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ORACLES = {
    'tests/rgen/dynamic_scalar_match_effects.sn': b'212\n212\nsaved subject\n3\nselected\n10\n20\n7\n4\n',
    'tests/rgen/dynamic_scalar_match_types.sn': b'bool\nchar\nint32\nlong\nuint\nuint32\nbyte\nfloat\ndouble\n1\n2\n3\n',
    'tests/rgen/errors/statement_match_dynamic_pattern.sn': b'matched\n',
    'tests/rgen/errors/statement_match_bool_dynamic_pattern.sn': b'matched\n',
    'tests/rgen/errors/statement_match_float_dynamic_pattern.sn': b'dynamic\n',
    'tests/rgen/errors/value_match_dynamic_pattern.sn': b'10\n',
    'tests/rgen/errors/value_match_bool_dynamic_pattern.sn': b'true\n',
    'tests/rgen/errors/value_match_double_dynamic_pattern.sn': b'10\n',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    # Verify frozen historical sources and diagnostics before executing them.
    digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    for name, expected in ORACLES.items():
        source = ROOT / name
        marker = source.with_suffix('.parity.json')
        if marker.is_file():
            contract = json.loads(marker.read_text())
            if (digest(source) != contract['source_sha256'] or
                    digest(source.with_suffix('.expected')) != contract['legacy_oracle_sha256'] or
                    bytes.fromhex(contract['stdout_hex']) != expected or contract['exit'] != 0):
                raise ValueError('historical source/oracle changed: ' + name)
        elif source.with_suffix('.expected').read_bytes().replace(b'\r\n', b'\n') != expected:
            raise ValueError('fixture oracle changed: ' + name)
    report_path = ROOT / '.sn' / ('rust-parity-dynamic-scalar-match-sanitizers.json' if args.sanitize
                                 else 'rust-parity-dynamic-scalar-match.json')
    environment = os.environ.copy()
    if args.sanitize:
        environment.update(SN_CFLAGS=environment.get('SN_CFLAGS', '') +
                           ' -fsanitize=address,undefined -fno-omit-frame-pointer',
                           ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                           UBSAN_OPTIONS='halt_on_error=1')
    command = [sys.executable, 'scripts/check_rust_parity.py', *ORACLES,
               '--require-count', str(len(ORACLES)), '--output', str(report_path)]
    for mode in ('default', 'checked', 'unchecked'):
        command += ['--arithmetic-mode', mode]
    subprocess.run(command, cwd=ROOT, env=environment, check=True)
    report = json.loads(report_path.read_text())
    required = {(source, opt, mode) for source in ORACLES
                for opt in ('-O0', '-O1', '-O2') for mode in ('default', 'checked', 'unchecked')}
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        key = (source, case['optimization'], case['arithmetic_mode'])
        if key not in required or key in seen or not case['passed']:
            raise ValueError('missing, duplicate or unsuccessful match case: ' + str(key))
        seen.add(key)
        if hashlib.sha256((ROOT / source).read_bytes()).hexdigest() != case['source_sha256']:
            raise ValueError('executed source changed: ' + source)
        expected = ORACLES[source]
        if os.name == 'nt':
            expected = expected.replace(b'\n', b'\r\n')
        if set(case['targets']) != {'c', 'rust'}:
            raise ValueError('missing match target: ' + str(key))
        for target in case['targets'].values():
            result = target['run']
            if (target['compile']['status'] != 0 or result['status'] != 0 or
                    result['stdout_hex'] != expected.hex() or result['stderr_hex']):
                raise ValueError('match independent oracle mismatch: ' + str(key))
    if seen != required:
        raise ValueError('incomplete match mode/optimization coverage')
    report['independent_oracle_cases'] = len(seen)
    report['historical_sources_preserved'] = True
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(seen)} independent dynamic scalar match cases')


if __name__ == '__main__':
    main()
