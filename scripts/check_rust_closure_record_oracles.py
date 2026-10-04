#!/usr/bin/env python3
"""Verify frozen closure record outputs and complete optimization/arithmetic coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_lambda_val_struct_str_field.sn': b'f(s) = hi\n',
    'tests/integration/test_lambda_val_struct_str_interp.sn': b'message=n=3 below minimum 5\n',
    'tests/integration/test_lambda_capture_outlives_scope.sn': b'hello from a captured local string | state-prefix | count=7\nhello from a captured local string | state-prefix | count=7\ncounter: 101\ncounter: 102\ncounter: 103\n',
    'tests/integration/test_lambda_capture_struct.sn': b'main: source=hello addr=world batch=200 interval=10000 bufLen=0\ninvokeIt: 50 iterations OK\n',
    'tests/rgen/closure_values_ref_struct.sn': b'1\n',
    'tests/rust/closure-values/closure_values_owned_struct_escaping.sn': b'seed:7:2\nseed:7:2\n',
    'tests/rgen/closure_owned_record_parameter_mutation.sn': b'11 12 12 before!!\n',
    'tests/rgen/closure_owned_record_parameter_aliases.sn': b'read 11 before!\n12 12 before!\n',
    'tests/rgen/closure_owned_record_captured_borrow.sn': b'inside 11 before\ninside 11 before\n11 11\n',
    'tests/rgen/closure_owned_record_captured_array.sn': b'inside 11 before 101\ninside 11 before 102\n11 11\n',
    'tests/rgen/closure_reference_record_capture_mutation.sn': b'11 12 12 alive\n',
    'tests/rgen/closure_record_assignment_value.sn': b'11 11 alive\n',
    'tests/rgen/closure_reference_record_parameter_aliases.sn': b'11 11 alive\n',
    'tests/rgen/closure_reference_record_captured_method.sn': b'11 12 12 alive\n',
    'tests/rgen/closure_nested_record_array_capture.sn': b'inside 11 before 101\ninside 11 before 102\n11 11\n',
    'tests/rgen/closure_reference_record_parameter_method.sn': b'11 12 12 alive\n',
    'tests/rgen/closure_owned_record_parameter_reassignment.sn': b'11 11 after\n',
    'tests/rgen/closure_owned_record_direct_capture_mutation.sn': b'inside 11 before 101\ninside 11 before 102\n11 11\n',
    'tests/rgen/closure_nested_record_direct_capture_mutation.sn': b'inside 11 before 101\ninside 11 before 102\n11 11\n',
    'tests/integration/test_as_ref_struct_lit_owned_field.sn': b'PASS\n',
    'tests/integration/test_import_fn_field_chained.sn': b'Creating router\nAdding route\nCreating request\nHandling request\nStatus: 200\nBody: Home\nTesting not found\nStatus: 404\nBody: Not Found\nAll tests passed!\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError(f'expected {len(required)} successful closure-record cases')
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
        sidecar = Path(source).with_suffix('.expected')
        if sidecar.read_bytes() != expected:
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
    report["independent_oracle_cases"] = len(seen)
    report["oracle_scope"] = (
        "Default owned record parameter mutation and repeated aliases; whole-record "
        "reassignment and consumed scalar field results; per-call captured scalar copies "
        "with persistent array owners, including nested records; escaping lifetimes; "
        "reference record capture/parameter identity and methods. No C-undefined probe credit."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {len(seen)} independent closure-record C/Rust oracles")


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
