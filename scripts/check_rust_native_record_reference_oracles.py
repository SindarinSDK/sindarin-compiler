#!/usr/bin/env python3
"""Independent raw output and complete mode coverage for native record references."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/rust-native/value_record_reference.sn': b'42\n',
    'tests/rust-native/value_record_reference_chars.sn': b'30 y false\n42 p true\n5 x false 50 y\ntrue\n',
    'tests/rust-native/value_record_reference_nested.sn': b'3 x 30 y 2.00000 false\n3 z 30 y 2.00000 false\n',
    'tests/rust-native/value_record_reference_methods.sn': b'4\na\nb 5\na\nc\nd\n',
}
NATIVE_SOURCES = (
    'tests/rust-native/value_record_reference.sn.c',
    'tests/rust-native/value_record_reference_chars.sn.c',
)


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 36 successful native record reference cases')
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
        'Persistent C-layout storage for heap-free record references preserves '
        'C-retained addresses, source updates, same-place aliases and distinct '
        'places, nested records, all 256 character bytes, subsequent by-value '
        'returns/copies, and source method/postfix/compound/indexed mutations. '
        'Thread-managed, owned/pointer, packed/refcounted and user-copy records '
        'remain separate admission and ownership obligations.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 36 independent native record reference C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
