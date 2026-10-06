#!/usr/bin/env python3
"""Check frozen C/Rust closure array alias partition contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_required_three_aliases.sn': b'true\ntrue\n', 'tests/rust-native/scalar_alias_partitions_3.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_4.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_5.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_effects.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_mixed.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_static.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_alias_partitions_asval.sn': b'true\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_required_three_aliases.sn': '4ca2d9447c744c7502ea086fdc5c95f3a06895ef92e40e17bc3106f700d55f9d', 'tests/rust-native/scalar_alias_partitions_3.sn': '61f67251c917b8d08b6962233057296f8bb891333285f65191aea25405c8c390', 'tests/rust-native/scalar_alias_partitions_4.sn': '7fc89b2f67f65c766c896694e28f6529f57dc9b7aad4a42303658a5a8f176b5e', 'tests/rust-native/scalar_alias_partitions_5.sn': '7e1b2e0a982e454051d09218dfb793e1cd684b4d51f663efd5c1cac5d611db1e', 'tests/rust-native/scalar_alias_partitions_effects.sn': '202d22b935a9f2119e35da5e8adba5e642e568518f4f1128083e1cd4c1dc9c60', 'tests/rust-native/scalar_alias_partitions_mixed.sn': 'c543cc77ae70dfbe42750962045100c532577a5f1e4423afec8c6c0bbb08f8f2', 'tests/rust-native/scalar_alias_partitions_static.sn': '1633430cb442ce8ee5c2ad219cba8f6fefe9b944f912b33c6a024b277efae7b4', 'tests/rust-native/scalar_alias_partitions_asval.sn': '3395818c795304b3117e84e661ad8b045bd97e686af7b4c61cbe27724546b231'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 72 successful closure array alias partition cases')
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
    report['oracle_scope'] = 'All 5/15/52 identity partitions for 3/4/5 shared array formals, recursion, scalar argument effects, mixed array types, static duplicates and independent as-val entry snapshots. Independent true assertions for mutations, lengths and element order.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 72 independent closure array alias partition C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
