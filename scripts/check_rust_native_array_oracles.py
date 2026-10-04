#!/usr/bin/env python3
"""Frozen native-array oracles, unchanged sources, and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

FIXTURES = {'tests/integration/test_array_literal_arg_leak.sn': {'oracle_sha256': '750dfa8fdf57f31eaf331d7c2a38720aae66112b9aa1cd5658b453900b392362',
                                                      'source_sha256': '01d322c184bad61815f95c2192658c25051057a5a7fe9e225fb42fe903a80f66'},
 'tests/integration/test_self_method_forward_after_train.sn': {'oracle_sha256': '4122efbacb99bee36036a3637c33b5b31932f2370431b9a22533228196aaf169',
                                                               'source_sha256': '6304ce2dc0cf4f1e1c58fa474bcd721cbda449f0252ac0f4dea8ac8114358756'},
 'tests/rust-native/native_handle_arrays.sn': {'oracle_sha256': '3856d11cb71b9a11f90e27af8ea0c90d3dd7fbe6f26f07b806cb2cd784fbd519',
                                               'source_sha256': '2a9a9225f5eaf6ceed27543e3b155bc332615b05e034d636ce2a3bf21794ddaa'}}
NATIVE_HELPERS = {'tests/integration/test_array_literal_arg_leak_helper.c': '50a9040d2f289b094569fb7deb4ea9cfe74aa7cb616323aaece92efa65f2887e',
 'tests/integration/test_self_method_forward_after_train_helper.c': 'bc081b732ad65c15b19867e5c43f3d5652ea0fb0a3c81afccff5835cf9882f44',
 'tests/rust-native/native_handle_arrays.sn.c': 'd65ca7c65ed1f795f05abc50b3987c1beb63bf45aeb71f1fc0e0464e771d5241'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in FIXTURES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 27 successful native array cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        specification = FIXTURES[source]
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != specification['source_sha256'] or case['source_sha256'] != specification['source_sha256']:
            raise ValueError(f'source changed: {source}')
        expected = Path(source).with_suffix('.expected').read_bytes()
        if hashlib.sha256(expected).hexdigest() != specification['oracle_sha256']:
            raise ValueError(f'oracle changed: {source}')
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
    for source, expected_hash in NATIVE_HELPERS.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'native helper changed: {source}')
    if seen != required:
        raise ValueError('incomplete optimization/arithmetic coverage')
    report['independent_oracle_cases'] = len(seen)
    report['native_source_sha256'] = NATIVE_HELPERS
    report['oracle_scope'] = (
        'Canonical C native-reference array headers/data/element callbacks; unchanged array-literal cleanup and '
        'method/field/native training mutation originals; borrowed and duplicate native parameters, native '
        'reallocation with stable preceding record argument, ordinary default-array mutation, owned C array '
        'results/copies, scalar native fields, clear/slice, concat/reverse/insert/remove/pop with preserved inputs, borrowed foreach with cached length/current data, '
        'continue and owning element alias, declaration-scope array temporaries, nil transport and zero resources. '
        'Global/thread storage, other native array element families, qualifiers, callbacks and broader ownership '
        'remain required full parity work; len(nil) C undefined probe receives no credit.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 27 independent native array C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
