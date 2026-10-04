#!/usr/bin/env python3
"""Check complete mode coverage and independent owned-record output oracles."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/rgen/owned_record_nested_deep_references.sn': b'index new\nindex new\nindex new\nnew 2 new 3\nold 1\n',
    'tests/rgen/owned_record_nested_default_references.sn': b'new 2 new 2\nold 1\n',
    'tests/integration/test_composite_borrow_mutation.sn': b'test_composite_borrow_mutation: PASS\n',
    'tests/integration/test_composite_temp_arg.sn': b'test1 direct pass: PASS:i1:alpha\ntest2 local var: FAIL:i2:0.30000\ntest3 direct pass 2: PASS:i3:gamma\ntest4 tags: default\ntest_composite_temp_arg: PASS\n',
    'tests/integration/test_constrained_generics.sn': b'Test 1 passed: SortedList<Score>\nTest 2 passed: SortedList<int> (primitive auto-satisfies)\nTest 3 passed: SortedList<str> (primitive auto-satisfies)\nTest 4 passed: IndexedSet<Player> (multi-constraint)\nTest 5 passed: KeyValue<UserId, str> (mixed constraints)\nTest 6 passed: KeyValue<str, int> (primitive auto-satisfies)\nTest 7 passed: maxOf<Score> (constrained generic function)\nAll constrained generics tests passed!\n',
    'tests/integration/test_generic_static_call.sn': b'Test 1 passed: Wrapper<int>.create(42)\nTest 2 passed: Wrapper<str>.create("hello")\nTest 3 passed: Wrapper<int>.createWithCount(99, 5)\nTest 4 passed: Container<Tag>.of(Tag{name:"alpha"})\nTest 5 passed: Wrapper<Tag>.create(inner)\nAll generic static call tests passed!\n',
    'tests/integration/test_generics_nested_inference.sn': b'Case 1a passed: unwrap<int> from var\nCase 1b passed: unwrap<str> from var\nCase 2a passed: runOne<CrusherAdapter>\nCase 2b passed: runOne<WidgetAdapter>\nCase 3 passed: firstOf/secondOf<int, str>\nCase 4 passed: innerFirst<int, str> (doubly nested)\nCase 5 passed: takeBoth<int>\nCase 6 passed: Engine literal with nested Worker<CrusherAdapter>\nCase 7 passed: copy from Engine.worker into Worker<CrusherAdapter>\nCase 8 passed: Engine field init from buildWorker<CrusherAdapter>\nAll nested-inference tests passed!\n',
    'tests/integration/test_struct_self_method_call.sn': b'getValue: 5\ndoubled: 10\nquadrupled: 20\ndescribe: calc: 5\ndescribeDoubled: calc: 10\ngetText: [  hello  ]\ntrimmed: [hello]\nupper: [  HELLO  ]\ntrimmedUpper: [HELLO]\ngetLength: 9\nisEmpty: false\nisNotEmpty: true\ndescribe: TextProcessor(HELLO)\nempty.isEmpty: true\nempty.describe: TextProcessor(empty)\nv1.toString: 1.2.3-beta\nv2.toString: 2.0.0\nv1.lt(v2): true\nv2.gt(v1): true\nv1.eq(v1): true\nbumped: 2.0.0\nbumpedMinor: 1.3.0\n--- Method chaining ---\nv1.bumpMajor().bumpMinor(): 2.1.0\nv1.bumpMinor()x3: 1.5.0\nv1.bumpMajor()x2: 3.0.0\n',
    'tests/exploratory/test_match_as_val_ref.sn': b'=== Match As Val/Ref Tests ===\n\ntest_match_as_val_basic: PASS\ntest_match_as_val_all_modes: PASS\ntest_match_as_ref_modify: PASS\ntest_match_as_ref_all_modes: PASS\ntest_match_selector_ref: PASS\ntest_match_native_as_ref: PASS\ntest_match_transform_native_ref: PASS\ntest_match_val_preserves_array: PASS\ntest_match_on_struct_method_result: PASS\n\nAll match as val/ref tests passed!\n',
    'tests/exploratory/test_struct_self_method_call.sn': b'--- Range ---\nr1: first: [0, 10]\nr2: second: [5, 15]\nr3: [20, 30]\nr1.size: 10\n\n--- Chain ---\nchain.e: 15\nchain.result: chain=15\n\n--- Point3D ---\np1: p1(1, 2, 3)\np2: p2(4, 5, 6)\np1.magnitudeSquared: 14\np1.dot(p2): 32\np1.add(p2): p1(5, 7, 9)\np1.scale(3): p1(3, 6, 9)\nadd then scale(2): p1(10, 14, 18)\n\n--- StringOps ---\nwrapped: [  Hello World  ]\ntrimmedValue: [Hello World]\nupperValue: [  HELLO WORLD  ]\nlowerValue: [  hello world  ]\ntrimmedWrapped: [Hello World]\nvalueLength: 15\nhasValue: true\ndescribe: StringOps([Hello World])\n\nAll exploratory self-method-call tests PASSED!\n',
    'tests/rgen/owned_record_alias_array.sn': b'changed 3\nchanged 3 3 9\nchanged 3 3\ncopy 4 88 7\n',
    'tests/rgen/owned_record_reference_replace.sn': b'new 42\nnew 42\n',
    'tests/rgen/owned_record_field_operations.sn': b'rhs a\n1 8 2\n1\nsecond a!\na! 2\n9 2 5\n0\na! 2 2\n',
    'tests/rgen/owned_record_float_fields.sn': b'12\ntrue\n2\n1212\n2\n121212\n2\ntrue\ntrue\ntrue\ntrue\nfalse\ntrue\ntrue\n4 2\n',
    'tests/rgen/owned_record_nested_fields.sn': b'new 2 17 array 5\nold 1\n',
    'tests/rgen/owned_record_field_forwarding.sn': b'callback 1\n3 3\nnew 2\n3 7 9 new source\ncallback 3\n3 2\n2 9 4\n',
    'tests/rgen/owned_record_method_fields.sn': b'modified 4 42 99 7\n',
    'tests/rgen/owned_record_index_callback.sn': b'index\nvalue\nindex\nword\n1 42 7 old second new\n',
    'tests/rgen/owned_record_char_fields.sn': b'65 66\n65\n65\n',
    'tests/rgen/owned_record_nullable_array_fields.sn': b'true\ntrue\nfalse\n1 9\n',
    'tests/integration/test_as_ref_struct_param.sn': (
        b'Test 1: after increment: 1\nTest 1: after 2 more increments: 3\n'
        b'Test 2: after addAmount(10): 13\nTest 3: after reset: 0\n'
        b'Test 4: name = test\nTest 5: after scale(4): (8,12)\n'
        b'Test 6: after translate(1,2): (9,14)\n'
        b'Test 7: after increment+addAmount+reset: 0\n'
        b'\nAll as-ref-struct-param tests passed!\n'),
    'tests/integration/test_val_struct_shared_array.sn': b'name: test\nitems: 3\noriginal: 3\ncopy: hello\ndone\n',
    'tests/exploratory/test_gcc_edge_structs.sn': (
        b'=== Struct Edge Case Tests ===\n\nKeyword fields sum: 45\n'
        b'l5.value: 50\nl5.inner.value: 40\nl5.inner.inner.value: 30\n'
        b'l5.inner.inner.inner.value: 20\nl5.inner.inner.inner.inner.value: 10\n'
        b'Large struct sum: 240\nString fields: a b c\n'
        b'Other fields: 1.00000 2.00000 true false 255 X\n'
        b'ints length: 5\nstrs length: 2\nname: test\n'
        b'after push - ints: 6, strs: 3\n'
        b"Counter 'test': 3\nCounter 'test': 2\ngetCount: 2\nCounter 'test': 0\n"
        b'original: original count=5\nmodified: modified count=105\n'
        b"Counter 'first': 1\nCounter 'second': 2\nCounter 'third': 3\n"
        b'Array length: 3\n\n=== All Struct Tests Done ===\n'),
    'tests/exploratory/test_struct_config.sn': (
        b'=== Configuration Struct Tests ===\n\n'
        b'Test 1: Configuration with all defaults\n  PASS: All defaults applied correctly\n'
        b'Test 2: Partial configuration override\n  PASS: Partial override works correctly\n'
        b'Test 3: Full custom configuration\n  PASS: Full custom configuration works\n'
        b'Test 4: Database configuration pattern\n  PASS: Database configuration works\n'
        b'Test 5: Runtime configuration modification\n  PASS: Runtime modification works\n'
        b'Test 6: Configuration copy independence\n  PASS: Configuration copy is independent\n'
        b'Test 7: Configuration passed to function\n  Server: localhost:3000\n'
        b'  Max connections: 100\n  Timeout: 120s\n  Logging: true\n'
        b'  PASS: Configuration passed to function correctly\n'
        b'\n=== All Configuration Tests PASSED! ===\n'),
}
ORIGINAL_EXPLORATORY = {
    'tests/exploratory/test_gcc_edge_structs.sn',
    'tests/exploratory/test_struct_self_method_call.sn',
    'tests/exploratory/test_struct_config.sn',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 216 successful owned-record cases')
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
        'Owned value-record parameter aliases, value copies, whole-record replacement, '
        'array mutation/forwarding/recursion, reentrant callbacks, nested stores, '
        'floating byte storage, negative-index evaluation, method calls and nil state. '
        'Canonical C-invalid regression sources are excluded from parity credit.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 216 independent owned-record C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
