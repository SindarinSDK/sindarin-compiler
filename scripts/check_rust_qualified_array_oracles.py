#!/usr/bin/env python3
"""Check frozen C/Rust qualified closure array copy contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_qualified_array_closure.sn': b'true\ntrue\n', 'tests/rust-native/scalar_qualified_array_alias.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_qualified_array_return.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_qualified_array_nested.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_qualified_array_capture.sn': b'true\ntrue\n', 'tests/rust-native/scalar_qualified_array_hygiene.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_qualified_array_strings.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_qualified_array_closure.sn': 'f456051f0a6161dec5e15fbea519207d27279abe81657e611488d0cd1099e6c2', 'tests/rust-native/scalar_qualified_array_alias.sn': 'dd27799f42896091764dc5b8f6da7f00e670930891107f2473d60f9d4eee1430', 'tests/rust-native/scalar_qualified_array_return.sn': '5b182c9624401bc83992f36654da2a5a2e0c708ce4d456b033ff0dafc339587d', 'tests/rust-native/scalar_qualified_array_nested.sn': 'f5fe0c160e1b9cb1cc35ac91aea236545ca2458420d401d9698e2c920940caa5', 'tests/rust-native/scalar_qualified_array_capture.sn': '80ad61009ae76e9997cf30921bcec0606dc30e337b4a45e7ba366dd618e3bdf8', 'tests/rust-native/scalar_qualified_array_hygiene.sn': 'dc98823a193f6d9929981d21da8a00351ba29d7ea91225b2c85a95d0ee5d98a1', 'tests/rust-native/scalar_qualified_array_strings.sn': '62178d331ec22777c016e89c33c539e1fbbc2c8884928d9e97ffed5abac4c0b2'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 63 successful qualified closure array copy cases')
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
    report['oracle_scope'] = 'Per-invocation array copies, aliased formals, independent returned owners, nested arrays, escaping captures, helper hygiene and owned string elements.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 63 independent qualified closure array copy C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
