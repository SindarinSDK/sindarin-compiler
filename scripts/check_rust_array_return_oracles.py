#!/usr/bin/env python3
"""Check frozen C/Rust borrowed-array return contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_array_return_parameters.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_array_return_expressions.sn': b'true\ntrue\n', 'tests/rust-native/scalar_array_return_scopes.sn': b'true\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_array_return_parameters.sn': '73ab804c61f0ed0dcb520dc4883a2741660e6f00eab4707ace606f1fd9e1c959', 'tests/rust-native/scalar_array_return_expressions.sn': '0fab108a43e880d7d448ce293dddf8bf9eb33e85d98acc65946fe0e21fdc5d31', 'tests/rust-native/scalar_array_return_scopes.sn': 'ab96434c26e45aec294017d8b1120d515f1d3ca367b35cc29e4a5ac4daea1e43'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 27 successful borrowed-array return cases')
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
    report['oracle_scope'] = ('Independent owned array results from borrowed named-function and lambda parameters, expression bodies, nested arrays, nested parameter scopes, and owned local return transfer.')
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 27 independent borrowed-array return C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
