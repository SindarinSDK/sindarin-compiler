#!/usr/bin/env python3
"""Check frozen C/Rust closure array method alias contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_captured_record_method.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_captured_record_array_method.sn': b'202\n203\n1\nx\n1\n', 'tests/rust-native/scalar_captured_nested_method.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_captured_method_return.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_record_print_ownership.sn': b'valuevalue\ntrue\nvalue\ntrue\nreference\ntrue\nindexed\ntrue\ntemporary\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_captured_record_method.sn': '3cfea9b8e89d722911c34b941b52eed1f84487ad9fb02c2b9a1826bb1e921382', 'tests/rust-native/scalar_captured_record_array_method.sn': '01bcce8e37dd3751a47ed46d1e164fa9cae8293f07801f7b6fb383db342511b5', 'tests/rust-native/scalar_captured_nested_method.sn': '69b7ed10a004a9e31dadd394d1e0aa337ca2ee836d04892c14f90edaf4275cdc', 'tests/rust-native/scalar_captured_method_return.sn': 'dc9bc64b0aad34f29feec2c75a470b8b8c9cfe5a16a73b28aa8c58ca424426a4', 'tests/rust-native/scalar_record_print_ownership.sn': 'cc0d78eccef720fdf52d8186c70c10e3017b0ca81acc8dba9d0220788fa77cc4'}


def verify(path):
    report = json.loads(path.read_text())
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
    report['oracle_scope'] = 'Captured value-record methods: per-call scalar snapshots, private persistent arrays, nested receivers, owned returns, and borrowed C string member printing through stable value/reference/index/nested owners versus owned temporaries.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 45 independent closure array method alias C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
