#!/usr/bin/env python3
"""Check frozen C/Rust managed iterator contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/integration/test_iterator_protocol.sn': b'Test 1 passed: Range iteration (0+1+2+3+4 = 10)\nTest 2 passed: Range(3,6) iteration (3+4+5 = 12)\nTest 3 passed: Empty range (no iterations)\nTest 4 passed: PointList iteration (struct elements)\nTest 5 passed: Nested iterator loops\nTest 6 passed: Mixed array and iterator for-each\nTest 7 passed: break inside iterator loop\nTest 8 passed: Sequence<int> (generic iterator)\nTest 9 passed: Sequence<str> (generic iterator)\nAll iterator protocol tests passed!\n', 'tests/integration/test_generic_multi_param_iter.sn': b'Test 1 passed: SimpleMap<int, int> iteration\nTest 2 passed: SimpleMap<str, int> iteration\nTest 3 passed: SimpleMap<int, str> iteration\nAll multi-param generic iterator tests passed!\n', 'tests/rust-native/scalar_iterator_array_control.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_iterator_owned_control.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_iterator_owned_state.sn': b'1\n', 'tests/rust-native/scalar_iterator_owned_string.sn': b'owned\n', 'tests/rust-native/scalar_iterator_reference_control.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_iterator_temporary_collections.sn': b'true\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/integration/test_iterator_protocol.sn': 'ca12c7619d659cccf6c687ac3f40930ff67dadb747047bf0c3fc22d3c463386f', 'tests/integration/test_generic_multi_param_iter.sn': '04b55d4ab4fca01f8a2ee734aa3d282c71a2544bbc3cb5dccf0570265652c131', 'tests/rust-native/scalar_iterator_array_control.sn': 'bfc4269ea9824899f0cef9674280b0926f3ece607a7bb8f6a457a66be20a83a9', 'tests/rust-native/scalar_iterator_owned_control.sn': '65c98ce92cff460c8b43b349964d38ba5e326deac307e176f2e08ec6ef144a91', 'tests/rust-native/scalar_iterator_owned_state.sn': '4463bde94c7f93eb4540fc763e0e9dab073ec7b04b634393f8f7b678cad7ce62', 'tests/rust-native/scalar_iterator_owned_string.sn': '23d1d09445013a648dd12b25064f1cc4eaaf8afb9d4b86f682c4b02802d614ac', 'tests/rust-native/scalar_iterator_reference_control.sn': '33ce0073a877411642aa5721db26ec0d89455e4575438689ae3549759129e675', 'tests/rust-native/scalar_iterator_temporary_collections.sn': '595ea6d1b56c5f95343ff10f383ebf1e1fa8c3d40d00202e6f8cfd319bbda61d'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 72 successful managed iterator cases')
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
    report['oracle_scope'] = ('Iterator protocol with scalar, string, array, value-record and reference-record '
                             'elements and managed iterator/iterable state; original generic programs, '
                             'nested loops, empty input, mutation, early return, break and continue, '
                             'and once-only returned reference iterable evaluation.')
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 72 independent managed iterator C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
