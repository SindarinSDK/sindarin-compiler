#!/usr/bin/env python3
"""Verify frozen as-val array outputs, sources, helpers and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/exploratory/test_as_val_array_copy.sn': b'Test as val array copy seman'
                                                b'tics:\n\nTest 1 - as val p'
                                                b'arameter:\n  Original before:'
                                                b' [1, 2, 3]\n  Original after:'
                                                b' [1, 2, 3]\n\nTest 2 - as '
                                                b'val variable:\n  Source: [10,'
                                                b' 20, 30]\n  Copy: [0, 20, 30,'
                                                b' 40]\n\nTest 3 - Multiple '
                                                b'operations on copy:\n  Data ('
                                                b'original): [5, 5, 5]\n  Worki'
                                                b'ng (copy): [0, 1, 5, 5]\n'
                                                b'\nAll as val copy tests passe'
                                                b'd!\n',
 'tests/rust-native/native_as_val_arrays.sn': b'true\ntrue\ntrue\ntrue\ntrue'
                                              b'\ntrue\ntrue\ntrue\ntrue\ntru'
                                              b'e\ntrue\n',
 'tests/rust-native/native_as_val_array_contexts.sn': b'true\n' * 13,
 'tests/rust-native/native_as_val_array_types.sn': b'true\n' * 12,
 'tests/exploratory/test_array_of_lambdas.sn': b'1015-5All tests passed!'}
SOURCE_SHA256 = {'tests/exploratory/test_as_val_array_copy.sn': '38b57e48ddf3f76708d6de2fec0248773b5daa2965d88b40302b21f1b3f1a5ba',
 'tests/rust-native/native_as_val_arrays.sn': '6215ba6b3c53c0e07b699682c941cd78b74bd5f19ef4a10c769e6850448da6fb',
 'tests/rust-native/native_as_val_array_contexts.sn': '2f56d83ad2c5df2fb637a939ef0eb3e57c15993fe024f97a691f6982c5a22158',
 'tests/rust-native/native_as_val_array_types.sn': '6d50e8687bf29172e03f26adc2af42d47bd2306f4b220df7a7939eb2c6acc9a8',
 'tests/exploratory/test_array_of_lambdas.sn': '5aa0cd73fa47ae8dc76171755e67c90dfbc339f84877895576780d28ae20b5c6'}
HELPER_SHA256 = {}

def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 45 successful as-val array cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source] or case['source_sha256'] != SOURCE_SHA256[source]:
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        oracle_path = Path(source).with_suffix('.expected')
        if oracle_path.read_bytes() != expected:
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
    for source, expected_hash in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'helper source changed: {source}')
    report['helper_source_sha256'] = HELPER_SHA256
    report['oracle_scope'] = 'Documented array as-val copies at ordinary function and method entry, independent duplicate inputs, mixed borrowed/copied parameters, strings/nested/owned-value-record deep copies, reference-record identity and callable element ownership, nil/empty distinction, returns/forwarding, joined workers, escaping value-snapshot captures, all-arguments-before-entry-copy timing and C-defined push value-before-index ordering. The original C parameter-sharing bug is repaired to the documented contract; historical wrong output is not used as an oracle. Broader effectful alias combinations, qualified callable parameters, SDK/native arrays and lifetime families remain full-goal work.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 45 independent as-val array C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
