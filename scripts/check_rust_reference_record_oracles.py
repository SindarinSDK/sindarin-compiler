#!/usr/bin/env python3
"""Check complete mode coverage and independent reference-record output oracles."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_as_ref_array_return.sn': b'PASS\n',
    'tests/integration/test_as_ref_return_passthrough.sn': b'PASS\n',
    'tests/integration/test_as_ref_for_in_self_return.sn': b'PASS\n',
    'tests/integration/test_as_ref_queue_drain.sn': b'PASS\n',
    'tests/integration/test_return_ref_param.sn': b'a=42 b=42\nc=7 d=7 cache=7\nafter restash: a=42 c=7 e=99 cache=99\nPASS\n',
    'tests/integration/test_as_ref_struct_lit_owned_field.sn': b'PASS\n',
    'tests/integration/test_nil_compare_struct.sn': b'good: connected\nbad: nil\nnil == bad: true\n',
    'tests/integration/test_return_self.sn': b'setValue return self: PASS\nchained setValue: PASS\nchained setName: PASS\nAll return self tests passed!\n',
    'tests/integration/test_native_ref_field_pass_chain.sn': b'cycle 0: result=300, nf=10, ew=30\ncycle 1: result=600, nf=20, ew=60\ncycle 2: result=900, nf=30, ew=90\ncycle 3: result=1200, nf=40, ew=120\ncycle 4: result=1500, nf=50, ew=150\ncycle 5: result=1800, nf=60, ew=180\ncycle 6: result=2100, nf=70, ew=210\ncycle 7: result=2400, nf=80, ew=240\ncycle 8: result=2700, nf=90, ew=270\ncycle 9: result=3000, nf=100, ew=300\n',
    'tests/integration/test_native_ref_field_return.sn': b'Direct: tag=1, payload=2\nLocal h1: tag=30, payload=300\nLocal h2: tag=40, payload=400\nInside: a.tag=10, a.payload=100\nInside: b.tag=20, b.payload=200\nReturned h1: tag=10, payload=100\nReturned h2: tag=20, payload=200\n',
    'tests/integration/test_native_ref_method_forward_twice.sn': b'test1 model: w1=10 w2=20\nsafe: a=10 b=20\ntest1 after: w1=10 w2=20\ntest2 model: w1=30 w2=40\nforward: a=30 b=40\ntest2 after: w1=30 w2=40\nforward: a=30 b=40\ntest3 after: w1=30 w2=40\nPASS\n',
    'tests/rgen/reference_record_aliases.sn': b'11\nchanged 11 changed 11\nchanged 11 changed 11\nnew 99\n',
    'tests/rgen/reference_record_array_forwarding.sn': b'2\n9 2\n',
    'tests/rgen/reference_record_contains.sn': b'true\nfalse\n0\n-1\n',
    'tests/rgen/reference_record_nil_nested.sn': b'true\ntrue\ntrue\n2\n2\ntrue\n',
    'tests/rgen/reference_record_parameter_copies.sn': b'copy 2\nold 1\nlocal 2 local 2\n',
    'tests/rgen/reference_record_value_copies.sn': b'copy 2\nold 1\nlocal 2 old 1\n',
    'tests/rgen/resolved_operator_ref_receiver.sn': b'true\n',
    'tests/rgen/reference_record_self_copy_hooks.sn': b'copy\noriginal copied\noriginal 2 counted 1 counted 2\n',
}
ORIGINAL_EXPLORATORY = set()


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError(f'expected {len(required)} successful reference-record cases')
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
        if source in ORIGINAL_EXPLORATORY:
            if sidecar.exists():
                raise ValueError('the original exploratory sources have no output sidecars')
        elif sidecar.read_bytes() != expected:
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
    report['oracle_scope'] = (
        'Reference record identities through locals, array/field reads, calls, returns, '
        'global replacement, foreach, nested record construction and owner release; '
        'explicit value-parameter and copyOf content copies, retained local as-val '
        'alias behavior, nil values, identity-based array search and array forwarding. '
        'Value-record return-self copy hooks, including conditional returns and '
        'mutating hooks propagated through forwarding methods. '
        'C-invalid method-copy/return-self and copyOf(nil) probes are excluded.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(seen)} independent reference-record C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
