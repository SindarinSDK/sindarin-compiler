#!/usr/bin/env python3
"""Check frozen C/Rust scalar snapshot reference contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_snapshot_reference_reference.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_aliases.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_native.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_character.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_floating.sn': b'true\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_boolean.sn': b'true\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_closure.sn': b'true\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_native_character.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_mixed.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_snapshot_reference_byte.sn': b'true\ntrue\n', 'tests/rgen/closure_values_capture_ref_forward.sn': b''}
SOURCE_SHA256 = {'tests/rust-native/scalar_snapshot_reference_reference.sn': '56362bbaea7347ee4d7de6344222606c786d94076c221a50693db7befbf8ff30', 'tests/rust-native/scalar_snapshot_reference_aliases.sn': 'fafadfc92e700d933d206ab0be758aa1f3afb7f7737ee6dc733fa2c2d3824a26', 'tests/rust-native/scalar_snapshot_reference_native.sn': '6dd309ef7cc51e183deb8465a2f541ad67f4168380ebc9ec0733674bd7a00e0e', 'tests/rust-native/scalar_snapshot_reference_character.sn': '1bc8c841db220a1be3a25283dfcaa8b637d255ca36fb693f2e48ca119d2e3fc3', 'tests/rust-native/scalar_snapshot_reference_floating.sn': '72c1922df763ec9a915583c6f637842e625fa01749002cded17ba1c700c7f1d5', 'tests/rust-native/scalar_snapshot_reference_boolean.sn': '6294c64b4c73192cc99777b7cec511bc2c30c7aa99c8b2616e9f78c8dc0089aa', 'tests/rust-native/scalar_snapshot_reference_closure.sn': '95d42daa02b16f3fbf615e62cc87a4680d8b017f8975d377b1b7afc2c81ff378', 'tests/rust-native/scalar_snapshot_reference_nested.sn': '32347c4d67b907a5d5c7026c6ea6f75922920b000faab1b58e5d6d3e9592a5fc', 'tests/rust-native/scalar_snapshot_reference_native_character.sn': 'e85ec1f6af914e478003d80704437731a0e4d2051b7232377b5c471fcec17b23', 'tests/rust-native/scalar_snapshot_reference_mixed.sn': 'aa1edcfcf4d87534bcae58894d98bf949bcf9a36af0ffc1b581e2b35c4385276', 'tests/rust-native/scalar_snapshot_reference_byte.sn': 'fdb75a1623e4f7e8adf0d132dffbe48d2bdac4762d259df8f2b38a58ae5fd66e', 'tests/rgen/closure_values_capture_ref_forward.sn': '7e126a54bcfae4f3002228edf81eb42bbfb16ae0d1af717fca48e9b7e11b536b'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 108 successful scalar snapshot reference cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if (hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source]
                or case['source_sha256'] != SOURCE_SHA256[source]):
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        if Path(source).with_suffix('.expected').read_bytes() != expected:
            raise ValueError(f'stale fixture oracle: {source}')
        if os.name == 'nt':
            expected = expected.replace(b'\n', b'\r\n')
        if set(case['targets']) != {'c', 'rust'}:
            raise ValueError(f'missing target: {identity}')
        for target in ('c', 'rust'):
            result = case['targets'][target]
            if result['compile']['status'] != 0 or result['run']['status'] != 0:
                raise ValueError(f'unsuccessful execution: {identity} {target}')
            if result['run']['stdout_hex'] != expected.hex() or result['run']['stderr_hex']:
                raise ValueError(f'independent output mismatch: {identity} {target}')
    if seen != required:
        raise ValueError('incomplete optimization/arithmetic coverage')
    report['independent_oracle_cases'] = len(seen)
    report['oracle_scope'] = 'Per-invocation scalar snapshots forwarded by reference, repeated same/distinct aliases, ordinary/native/closure calls, char/bool/byte/float widths, nested capture epochs and mixed caller references.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 108 independent scalar snapshot reference C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
