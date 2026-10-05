#!/usr/bin/env python3
"""Check frozen C/Rust zero-record contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {
    'tests/rust-native/scalar_zero_record_fields.sn': b'true\n' * 8,
    'tests/rust-native/scalar_zero_record_local_scopes.sn': b'true\n' * 2,
    'tests/rust-native/scalar_zero_record_transports.sn': b'true\n' * 7,
}
SOURCE_SHA256 = {'tests/rust-native/scalar_zero_record_fields.sn': '3cf2cedc8779cad32b520eecb7f2ceafd022652730b3aca665d8782628a838fd',
 'tests/rust-native/scalar_zero_record_local_scopes.sn': '66bf144320183b39e01425ccc79392797c01686aa68aa38151fb2012d05f9345',
 'tests/rust-native/scalar_zero_record_transports.sn': '4a595a7ada56c67b8e8f77b326d86eeb4160ea5748db5af3723f2d8f30207448'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 27 successful zero-record cases')
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
    report['oracle_scope'] = ('Uninitialized value-record locals contain zero scalar and nested '
                             'record fields and nil managed fields. Source field defaults apply '
                             'to explicit literals, and zero values retain existing copy, return, '
                             'array, closure and scope ownership semantics.')
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 27 independent zero-record C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
