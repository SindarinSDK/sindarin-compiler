#!/usr/bin/env python3
"""Independent raw output and complete mode coverage for native record references."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/integration/test_struct_as_ref.sn': b"Testing 'as ref' struct parameter passing:\n\n1. O"
                                            b'riginal point: (5.00000, 10.00000)\n   After update_p'
                                            b'oint: (15.00000, 30.00000)\n\n2. Before scaling: ('
                                            b'2.00000, 3.00000)\n   After scale by 5: (10.00000, 15'
                                            b'.00000)\n\n3. Before increment: count=0\n   After 3'
                                            b' increments: count=3\n\n4. Starting point: (1.0000'
                                            b'0, 1.00000)\n   After two updates: (21.00000, 41.0000'
                                            b"0)\n\nAll 'as ref' struct tests PASSED!\n",
 'tests/rust-native/value_record_reference_strings.sn': b'initial true true first leaf\nwritten tru'
                                                        b'e true rust rust_leaf\nnative 20 z native'
                                                        b' native_leaf true\ncopy 20 native native_'
                                                        b'leaf 120 copy copied_leaf\nnil true true '
                                                        b'true\nrestored true true restored aga'
                                                        b'in\n',
 'tests/rust-native/value_record_strings_values.sn': b'values 5 original 15 native_copy 8 made loca'
                                                     b'l_copy\nconsume 13 26 12\nwrite written lo'
                                                     b'cal_copy\nempty 0 true\nfilled filled\nmeth'
                                                     b'od written!\ncompound written!?\narrays ar'
                                                     b'ray written!? written!?\n',
 'tests/rust-native/value_record_strings_threads.sn': b'first 7 first\nagain 9 again\ngroup 11 lef'
                                                      b't 12 right\nparameter 21 touched 20 origi'
                                                      b'nal\n',
 'tests/rust-native/value_record_owned_field.sn': b'owned\n',
 'tests/rust-native/value_record_thread_storage.sn': b'8\n'}
NATIVE_SOURCES = ('tests/rust-native/value_record_reference_strings.sn.c',)


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 54 successful native managed record cases')
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
    report['native_source_sha256'] = {
        source: hashlib.sha256(Path(source).read_bytes()).hexdigest()
        for source in NATIVE_SOURCES
    }
    report['oracle_scope'] = (
        'Unchanged original managed record reference; canonical source address, '
        'nested C char/string layouts checked against actual generated C headers, '
        'C string replacement and NULL, native field pointer identity, Rust field '
        'writes and compound/method mutations, by-value/as-val parameters, '
        'native returns, independent record/array copies, repeated and grouped '
        'thread result joins and thread parameter copy ownership.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 54 independent native managed record C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
