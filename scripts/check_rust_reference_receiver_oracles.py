#!/usr/bin/env python3
"""Check frozen C/Rust closure array method alias contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_reference_row_owner.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_native_reference_row.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_native_closure_owners.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_native_nested_closure.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_native_temporary_closure.sn': b'true\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_reference_row_owner.sn': '573d5c37ae5ac3b914b7d1b996b1ea24060cbc92112a23cffcb9f055ec1f9b8f', 'tests/rust-native/scalar_native_reference_row.sn': '5a5d492c20d3c68577fed75f846f5517764a054ae473df0a7f4567c2d4696802', 'tests/rust-native/scalar_native_closure_owners.sn': '3063f2655babe47d28d0d8104d138fec850c484a988b91f78d7df68ddda96bad', 'tests/rust-native/scalar_native_nested_closure.sn': '2b30240653bcfe32c54f7108230bbf0f16fbecc1742fb4d84f3799ae17b549c2', 'tests/rust-native/scalar_native_temporary_closure.sn': 'a476e1e59bf1f99049c3073b4d77694a5c852cb8ef6e84428c6a007c1e81760e'}
HELPER_SHA256 = {'tests/rust-native/scalar_native_reference_row.sn.c': '7b66ec7038ea1e9981a1becd22cfeab237dcd82a05f0fe739bbf74332ef14b12', 'tests/rust-native/scalar_native_closure_owners.sn.c': 'd7aad44913b28488f5491c72fb873f28efff91c41c79f2846b4754917fe0fd6c', 'tests/rust-native/scalar_native_nested_closure.sn.c': '47e8244d5a7471ca8c9dd590c7573bce976575bbb78b616caac01cc5db57ec09', 'tests/rust-native/scalar_native_temporary_closure.sn.c': 'd7aad44913b28488f5491c72fb873f28efff91c41c79f2846b4754917fe0fd6c'}


def verify(path):
    report = json.loads(path.read_text())
    for source, digest in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != digest:
            raise ValueError('native helper changed: ' + source)
    report['native_source_sha256'] = HELPER_SHA256
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 45 successful closure array method alias cases')
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
    report['oracle_scope'] = 'Reference-record physical receiver identity, canonical native headers, borrowed object parameters, copied/returned arrays, escaping snapshots, nested rows, native reference counts and declaration temporary lifetimes.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 45 independent closure array method alias C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
