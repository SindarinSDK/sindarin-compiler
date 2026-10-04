#!/usr/bin/env python3
"""Independent raw output and complete mode coverage for by-value native records."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_thread_native_fn_struct_return.sn': b'r1=10\n',
    'tests/rust-native/native_record_values.sn': b'7 a true\n107 x false\n17 a true\n',
    'tests/rust-native/native_record_hygiene.sn': b'z 91\n',
    'tests/rust-native/value_record_bridge.sn': (
        b'0xFF\n-123\ntrue\n1.50000\n7000000000\n-8000000000\n'
        b'9000000000\n4000000000\n0xFE\n2.50000\n'
        b'7000000000 7000000010 128 false\nrecord\n0xFF\n2 128 254\ntrue\n'
    ),
    'tests/exploratory/test_gcc_edge_interop.sn': (
        b'=== Native/Interop Edge Case Tests ===\n\n'
        b'sin(pi/2) = 1.00000\ncos(0) = 1.00000\nsqrt(144) = 12.00000\n'
        b'pow(2,10) = 1024.00000\nfabs(-42.5) = 42.50000\n'
        b'floor(3.7) = 3.00000\nceil(3.2) = 4.00000\nsqrt(25) = 5.00000\n'
        b'Complex native expr: 0.57787\nsqrt(16)==4: PASS\n'
        b'Sum of sqrt(1..10): 22.46828\nSin values count: 10\n'
        b'First sin: 0.00000\nLast sin: 0.78333\n'
        b'Duplicate includes test: PASS\ntoInt(\'12345\') = 12345\n'
        b'toDouble(\'3.14159\') = 3.14159\nMathResult: value=7.00000 valid=1\n'
        b'\n=== All Interop Tests Done ===\n'
    ),
}
NATIVE_SOURCES = (
    'tests/integration/test_thread_native_fn_struct_return.sn.c',
    'tests/rust-native/value_record_bridge.sn.c',
)


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 45 successful native record cases')
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
        if source == 'tests/exploratory/test_gcc_edge_interop.sn':
            if oracle_path.exists():
                raise ValueError('the original exploratory source has no output sidecar')
        elif oracle_path.read_bytes() != expected:
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
        'Heap-free native and ordinary value records cross the original C ABI '
        'by value, including nested records, default/as-val copies, all scalar '
        'widths, all 256 character bytes, bool, mixed managed results and aliased '
        'scalar character references, hygienic names and native thread results. '
        'Record references, owned or pointer fields, packed/refcounted records '
        'and user-defined copy hooks remain unsupported.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 45 independent by-value native record C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
