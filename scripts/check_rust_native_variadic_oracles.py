#!/usr/bin/env python3
"""Independent raw output and complete mode coverage for native variadic adapters."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/integration/test_interop_comprehensive.sn': b'=== Comprehensive C Inte'
                                                    b'rop Test ===\n\nTest 1'
                                                    b': Math library integrati'
                                                    b'on\n  PASS: Math library '
                                                    b'works\nTest 2: Variadic p'
                                                    b'rintf\n  String: hell'
                                                    b'o\n  Integer: 42\n  Do'
                                                    b'uble: 3.14\n  Mixed: scor'
                                                    b'e=95 (95.0%)\n  PASS: Var'
                                                    b'iadic printf works\nTest '
                                                    b'3: Opaque type operation'
                                                    b's\n  PASS: Opaque type wo'
                                                    b'rks\nTest 4: Out-paramete'
                                                    b'rs with as ref\n  PASS: O'
                                                    b'ut-parameters work\nTest '
                                                    b'5: Native callbacks\n  PA'
                                                    b'SS: Native callbacks wor'
                                                    b'k\nTest 6: Pointer operat'
                                                    b'ions\n  PASS: Pointer ope'
                                                    b'rations work\nTest 7: Com'
                                                    b'bined workflow (all feat'
                                                    b'ures together)\n  Combine'
                                                    b'd: sqrt(5.0)=2.2361, sca'
                                                    b'led=70\n  PASS: Combined '
                                                    b'workflow works\n\n=== '
                                                    b'All comprehensive intero'
                                                    b'p tests PASSED! ===\n',
 'tests/rust-native/native_variadic_promotions.sn': b'true\ntrue\ntrue\n',
 'tests/rust-native/native_variadic_ownership.sn': b'true\nowned\ntrue\ntrue'
                                                   b'\n10\ntrue\n',
 'tests/rust-native/native_variadic_records.sn': b'true\nmutated 19 written\n'
                                                 b'values 19 written 29 cop'
                                                 b'ied\nindependent written '
                                                 b'local\n',
 'tests/rust-native/native_variadic_contexts.sn': b'22\n0\n17\n30\n35\n41\n51\n'
                                                  b'threads 67 77\n',
 'tests/rust-native/native_variadic_unused.sn': b'seed 3\nmain\n'}
HELPER_SHA256 = {'tests/integration/test_variadic_helper.c': 'a9e5d49172c9ab337f94918a208fc8eb8ad228bc951662585f3a06a4526c660f',
 'tests/rust-native/lib/native_variadic_imports.sn': 'a97af7fd08a5920c2036b777d7c40c4c10de7e2f896bb8c77f1986e275ae5c8f',
 'tests/rust-native/lib/native_variadic_imports.sn.c': '27751275f5498c82dac131ad3bc60ff22a021ea7edf76dfe2f9f68a462a1e509',
 'tests/rust-native/native_variadic_promotions.sn.c': 'edae0b456218a12e2dbdca5add8ae45893cc052f10c9edf999f632108e5a8522',
 'tests/rust-native/native_variadic_ownership.sn.c': '70783862189e32d5422aa1ccae452e0680e0da8b85f2c9699809c2210e41b324',
 'tests/rust-native/native_variadic_records.sn.c': '7491abacb460435cb30688175b2304f5483817a3e29b30f74d7e942d5a60181f'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 54 successful native variadic cases')
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
    report['oracle_scope'] = 'Direct variadic C calls through fixed-signature adapters; actual C default promotions, scalar and pointer tails, imported aliases, globals and closures, joined threads, duplicate scalar/char/record references, managed results, owned record transfer, C variadic bodies and private initializers without public wrappers. Foreign Rust callback transport and indirect variadic callable values remain required.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 54 independent native variadic C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
