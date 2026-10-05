#!/usr/bin/env python3
"""Independent raw output and complete mode coverage for native C callables."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/integration/test_native_callback_typedef.sn': b'PASS\n',
 'tests/integration/test_qsort_callback.sn': b'Testing qsort-style native compa'
                                             b'rator callback\nPASS: Compara'
                                             b'tor callback executed correc'
                                             b'tly\n- Defined type Comparato'
                                             b'r = native fn(a: *void, b: *void'
                                             b'): int\n- Created native lamb'
                                             b'da matching Comparator signature'
                                             b'\n- Passed callback to native'
                                             b' function\n- Callback returne'
                                             b'd expected comparison result\n',
 'tests/integration/test_interop_callback.sn': b'=== Native Callback Interop '
                                               b'Test ===\n\n  Testing basic na'
                                               b'tive callback invocation...\n'
                                               b'    PASS: basic callback tes'
                                               b't\n  Testing multiple callbac'
                                               b'k invocations...\n    PASS: m'
                                               b'ultiple invocations test\n  T'
                                               b'esting callback return value'
                                               b' logic...\n    All callback l'
                                               b'ogic paths tested\n    PASS: '
                                               b'callback logic test\n  Testin'
                                               b'g predicate callback...\n    '
                                               b'PASS: predicate callback tes'
                                               b't\n  Testing status callback.'
                                               b'..\n    PASS: status callback'
                                               b' test\n\n=== All native callba'
                                               b'ck tests PASSED! ===\n',
 'tests/integration/test_interop_edge_cases.sn': b'=== Interop Edge Case Tests '
                                                 b'===\n\nTest 1: Nil pointer'
                                                 b' comparisons\n  ptr == ni'
                                                 b'l: PASS\n  ptr != nil whe'
                                                 b'n nil: PASS\n  Function t'
                                                 b'est_nil_equality: PASS\n '
                                                 b' Function test_nil_inequalit'
                                                 b'y: PASS\n\nTest 2: *char a'
                                                 b's val when NULL\n  Pointe'
                                                 b'r is nil: PASS\n  Safe un'
                                                 b'wrap returns guard value: PA'
                                                 b'SS\n\nTest 3: Mixed pointe'
                                                 b'r and primitive paramete'
                                                 b'rs\n  Nil pointer detecti'
                                                 b'on: PASS\n  Mixed with ni'
                                                 b'l and true flag: PASS\n\nT'
                                                 b'est 4: Callback with pointer'
                                                 b' parameters\n  Callback r'
                                                 b'eceived nil: PASS\n  Call'
                                                 b'back !nil check with nil: PA'
                                                 b'SS\n\nTest 5: Pointer-to-p'
                                                 b'ointer nil handling\n  **'
                                                 b'int == nil: PASS\n  Funct'
                                                 b'ion with **int nil: PASS'
                                                 b'\n\n=== All edge case test'
                                                 b's completed! ===\n',
 'tests/rust-native/native_callable_body.sn': b'2\n',
 'tests/rust-native/native_callable_islands.sn': b'rust 102 6\nrust_owned 11'
                                                 b'1 8\nnative 15 12 33 12 6'
                                                 b'\nthreads 16 18\nhelper_th'
                                                 b'read 13\nresult 125\n',
 'tests/rust-native/native_callable_globals.sn': b'globals 13 15\nreplaced 2'
                                                 b'0\n'}
HELPER_SHA256 = {'tests/rust-native/lib/native_callable_helpers.sn': '084e2198ea5e3f61bc050085f3d716f04238a04daaa05a150a75ebcb8e2a76ab'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 63 successful native callable cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != case['source_sha256']:
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
    report['oracle_scope'] = (
        'Unchanged native callback typedef, comparator and interop originals; '
        'promoted native closure body; mixed Rust/C nested and owned captures, '
        'imported shared and C-only helpers, resolved function references, '
        'repeated/grouped C threads, deferred C closure globals and replacement. '
        'All callables stay in the existing C closure ABI on the C side; only '
        'independently admitted public parameter/result types cross the bridge.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 63 independent native callable C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
