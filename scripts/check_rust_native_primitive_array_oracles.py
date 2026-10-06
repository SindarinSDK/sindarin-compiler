#!/usr/bin/env python3
"""Check frozen C/Rust owned closure array rebinding contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_native_primitive_array_alias.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_return.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_copies.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_nil.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_operations.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_int32.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_uint.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_uint32.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_byte.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_bool.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_float.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_double.sn': b'true\ntrue\n', 'tests/rust-native/scalar_native_primitive_array_long.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_native_primitive_array_alias.sn': '2407d61e8ef96d8a814c1f65b0d39d8bad19a7134a78f143411411073389cdf9', 'tests/rust-native/scalar_native_primitive_array_bool.sn': '6dce6dd3b0b696dc274327201ce4da10b2b1dd31e6392765460accd3582af98e', 'tests/rust-native/scalar_native_primitive_array_byte.sn': 'bb83caf790d54b295b66e1291ea54b45c8c5d5f1451a9eaffe55bd24cebb2edf', 'tests/rust-native/scalar_native_primitive_array_copies.sn': '8f0cb9d746de6b33025eb64ada77b4ac61746266d620527bd9965369e8fd8bd1', 'tests/rust-native/scalar_native_primitive_array_double.sn': 'f0c90f3f09582a4175cc33bb7742030530f0fb2ed9bed3c3afa1e782c27aa0d6', 'tests/rust-native/scalar_native_primitive_array_float.sn': 'bbed7b6b4d55121f6d061dda3a0786ce7fbb7e0bac40e68cbb9a3181b12597b9', 'tests/rust-native/scalar_native_primitive_array_int32.sn': 'd3584fa2c355250dc532593a70d2c6696c6e10f2b5c5faefc97577fd76538e9d', 'tests/rust-native/scalar_native_primitive_array_long.sn': '2f09e4d92e5328541d330cee1a01ff430e6b1db7be7e682c96cede61e2897390', 'tests/rust-native/scalar_native_primitive_array_nil.sn': '3f9e95ca7d6d43b621b1a43745c376ed147fcd7ba3c8e1eefe4ac9e5880cecec', 'tests/rust-native/scalar_native_primitive_array_operations.sn': '58861aa1e9e15dc87ce0b4133e0a2f533754cc32b176c9533a6232755d9652c0', 'tests/rust-native/scalar_native_primitive_array_return.sn': '4232368d107660d800687c7b8cd4325174b06933833ed33bcb052d1accebd697', 'tests/rust-native/scalar_native_primitive_array_uint.sn': 'a78fde5b201b9f409b0c36b6ac3e43901960c6d87c19431006bb7fac120146fe', 'tests/rust-native/scalar_native_primitive_array_uint32.sn': '5b377ffa2747bb7672b13ccb800a41df01c7388889557c85e88f2882bee655ee'}
HELPER_SHA256 = {'tests/rust-native/scalar_native_primitive_array_alias.sn.c': '76b1ee5ce1f9a36274ce8fd4beb5d9c595467b1232ebef1e49e77a6546426b1e', 'tests/rust-native/scalar_native_primitive_array_bool.sn.c': 'ef093eeb2455e72182a9a861d9844684d5ae74909264aa50e7d13c2d0953c64c', 'tests/rust-native/scalar_native_primitive_array_byte.sn.c': '91c8f818794fe905b46e1344010b2e41f0aa23919d1066658e3cbb3a3a0adbec', 'tests/rust-native/scalar_native_primitive_array_copies.sn.c': '76b1ee5ce1f9a36274ce8fd4beb5d9c595467b1232ebef1e49e77a6546426b1e', 'tests/rust-native/scalar_native_primitive_array_double.sn.c': 'e5ee9ad0d3929d6726ee2bbf38b749e837e8c7d67a69934ac90b21962cc770fc', 'tests/rust-native/scalar_native_primitive_array_float.sn.c': '167e12b93a59eb55cc65d7e5c9fff0d027cd75d5fc6ecbffc6426d54c02de401', 'tests/rust-native/scalar_native_primitive_array_int32.sn.c': 'fda3edf0db1f26b3371ac13d1aa395ecfedc1374cec72a6ae6a0370cd16e78ba', 'tests/rust-native/scalar_native_primitive_array_long.sn.c': 'bfbba44208e72549c4547e8b1fda1f4b0f9127c1d6b4a46eacd6c839e7e1e5d2', 'tests/rust-native/scalar_native_primitive_array_nil.sn.c': 'a931344714befea13f25992c04e31b650633636a5fdcd65cfa2ee6e2b1da2823', 'tests/rust-native/scalar_native_primitive_array_operations.sn.c': '76b1ee5ce1f9a36274ce8fd4beb5d9c595467b1232ebef1e49e77a6546426b1e', 'tests/rust-native/scalar_native_primitive_array_return.sn.c': '1592fc11b51a1e1fe51d06506cc0d68d4d7274289d70752a0d72b58c1b7747db', 'tests/rust-native/scalar_native_primitive_array_uint.sn.c': 'c0fd985c5549a4233f5ede1f16cb730adf889b5e1ef7a887e0720f2f6823f32a', 'tests/rust-native/scalar_native_primitive_array_uint32.sn.c': 'e9e6da10f5d799923c983fc9efe4c0167aca8ba617a484d93a17b9629e428822'}


def verify(path):
    report = json.loads(path.read_text())
    for source, digest in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != digest:
            raise ValueError(f'native helper changed: {source}')
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 117 successful native primitive array cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if (hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source]
                or case['source_sha256'] != SOURCE_SHA256[source]):
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        if Path(source).with_suffix('.expected').read_bytes() != expected:
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
    report['native_source_sha256'] = HELPER_SHA256
    report['oracle_scope'] = 'Real C headers and buffers, repeated identity/native growth, owned native returns, independent copies, nil and operations across wire-compatible scalar widths.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 117 independent native primitive array C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
