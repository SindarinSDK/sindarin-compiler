#!/usr/bin/env python3
"""Check frozen C/Rust closure array method alias contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_method_aliases.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_aliases_static.sn': b'true\ntrue\n', 'tests/rust-native/scalar_method_alias_partitions.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_alias_effects.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_alias_reference.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_alias_stored_receiver.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_alias_asval_mixed.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_method_alias_static_duplicate.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_method_aliases.sn': '2e65e2602fa2819758c9b75116f9f01a4b56dec3cbb4449bd4b5862471a5c529', 'tests/rust-native/scalar_method_aliases_static.sn': '4ac1f8c75f459d811899b15a1d5ebfdb56cc9873e1c4d8ce9e5d02002146ad1f', 'tests/rust-native/scalar_method_alias_partitions.sn': 'd97f1a279215061ece46d2d0cb57b868c4144f4477c0e9ea24c0f2d6ce64c52d', 'tests/rust-native/scalar_method_alias_effects.sn': '8f36e80e522b268a16f1acc0e3dfa430596c682066632a212323d42d42014797', 'tests/rust-native/scalar_method_alias_reference.sn': '58a9d30ce3d6b5693859b08504b89ec5234270dbc7cbba8503b8a5afb7ad6ca8', 'tests/rust-native/scalar_method_alias_stored_receiver.sn': '0fe358faa81e15cc6c96f1dd1ae6839aba6ccba0085c3412a31eee7982f88d68', 'tests/rust-native/scalar_method_alias_asval_mixed.sn': 'f2bcbbbdb2074246375af81ae743b747c1adbc589ff856539a768783133f52a1', 'tests/rust-native/scalar_method_alias_static_duplicate.sn': '4ba8919e6437be9161b70e04dca530487f6517fcc345c9a8b8d95acebbd0c5e2'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 72 successful closure array method alias cases')
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
    report['oracle_scope'] = 'Instance/static methods: all 15 four-formal alias partitions for both recursive method families; primitive/reference/stored receivers; once-only argument effects; mixed array types, static duplicates and named as-val entry snapshots.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 72 independent closure array method alias C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
