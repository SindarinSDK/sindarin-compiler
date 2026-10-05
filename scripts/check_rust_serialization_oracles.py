#!/usr/bin/env python3
"""Verify frozen serialization outputs, sources, helpers and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/integration/test_serializable.sn': b'test_encode_address: PASS\nencoded: {"name":"Alice","'
                                           b'age":30,"score":9.5,"active":true,"address":{"street'
                                           b'":"456 Oak Ave","city":"LA"},"tags":["dev","lead"]}\n'
                                           b'test_encode_person: PASS\ntest_decode_address: PASS\nt'
                                           b'est_decode_person: PASS\ntest_roundtrip: PASS\ntest_en'
                                           b'codeArray: PASS\ntest_decodeArray: PASS\ntest_array_ro'
                                           b'undtrip: PASS\ntest_alias_encode: PASS\ntest_alias_dec'
                                           b'ode: PASS\ntest_alias_roundtrip: PASS\n\nAll @seria'
                                           b'lizable tests passed!\n',
 'tests/integration/test_serializable_decode_struct_literal_loop.sn': b'iteration 0: {"name":"te'
                                                                      b'st","items":["a","b"]}\ni'
                                                                      b'teration 1: {"name":"tes'
                                                                      b't","items":["a","b"]}\nit'
                                                                      b'eration 2: {"name":"test'
                                                                      b'","items":["a","b"]}\nPAS'
                                                                      b'S\n',
 'tests/integration/test_serializable_double_int_json.sn': b'test_zero_int_vector: PASS\ntest_nonz'
                                                           b'ero_int_vector: PASS\ntest_mixed_int_'
                                                           b'vector: PASS\ntest_double_vector: PAS'
                                                           b'S\narray interpolation: [10, 20, 30]\n'
                                                           b'test_array_interpolation: PASS\ntest_'
                                                           b'roundtrip_mixed_arrays: PASS\ntest_la'
                                                           b'rge_zero_vector: PASS\n\nAll double[] '
                                                           b'integer JSON tests passed!\n',
 'tests/integration/test_serializable_encoder_cleanup.sn': b'test_encode_cleanup: PASS\ntest_neste'
                                                           b'd_encode_cleanup: PASS\ntest_encodeAr'
                                                           b'ray_cleanup: PASS\ntest_roundtrip_cle'
                                                           b'anup: PASS\ntest_sequential_cleanup: '
                                                           b'PASS\ntest_two_step_encode: PASS\ntest'
                                                           b'_two_step_encodeArray: PASS\ntest_rep'
                                                           b'eated_result: PASS\n\nAll encoder-clea'
                                                           b'nup tests passed!\n',
 'tests/integration/test_serializable_long.sn': b'encoded: {"name":"click","timestamp":17116416000'
                                                b'00,"count":42}\ndecode: PASS\nAll @serializabl'
                                                b'e long tests passed!\n',
 'tests/integration/test_serializable_push_ownership.sn': b'test_push_lvalue: PASS\ntest_push_rva'
                                                          b'lue: PASS\ntest_re_encode: PASS\ntest_'
                                                          b'roundtrip: PASS\ntest_insert: PASS\n\nA'
                                                          b'll push-ownership tests passed!\n',
 'tests/integration/test_serializable_return_nested_array.sn': b'same-scope: buy\nsame-scope decod'
                                                               b'e: PASS\nfn-return: buy\nfn-return'
                                                               b' decode: PASS\nAll tests passed!\n',
 'tests/rust-native/native_serial_hygiene.sn': b'true\ntrue\ntrue\n',
 'tests/rust-native/native_serial_lifetimes.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n'
                                                 b'true\ntrue\ntrue\n',
 'tests/rust-native/native_serial_mixed_handles.sn': b'true\ntrue\n',
 'tests/rust-native/native_serial_objects.sn': b'{"name":"Ada","age":42,"score":3.5,"active":true'
                                               b',"missing":null}\ntrue\nAda\n42\ntrue\ntrue\nt'
                                               b'rue\n',
 'tests/rust-native/native_serial_threads.sn': b'true\ntrue\n',
 'tests/integration/test_thread_struct_return_types.sn': b'p1: value=10, ok=true, score=1.50000'
                                                         b'\np2: value=20, ok=true, score=3.0000'
                                                         b'0\nA: PASS\nc1: id=item_1, value=2.000'
                                                         b'00\nc2: id=item_2, value=4.00000\nB: P'
                                                         b'ASS\nt1: id=model_s1, acc=0.10000, cf'
                                                         b'g=adam\nt2: id=model_s2, acc=1.00000,'
                                                         b' cfg=sgd\nt3: id=model_s3, acc=0.5000'
                                                         b'0, cfg=rmsp\nC: PASS\nm1: count=10, to'
                                                         b'tal=15.00000\nm2: count=20, total=30.'
                                                         b'00000\nD: PASS\nAll thread struct retu'
                                                         b'rn type tests completed\n'}
SOURCE_SHA256 = {'tests/integration/test_serializable.sn': '2660ea8bcedcae2305d78dd38f1aec51497a6b1dfae4eb88544f86db87128c00',
 'tests/integration/test_serializable_decode_struct_literal_loop.sn': '42f15159311a783ab955c06af505fb60c2c9a8c138ad3c2c0cbaf0f195a6e5d2',
 'tests/integration/test_serializable_double_int_json.sn': 'bc996a180daf81a22ebeafe096e75e33c0fdaccada5b017db2b93036dfd9df30',
 'tests/integration/test_serializable_encoder_cleanup.sn': 'b3d4ac81274b5f70df6871a6dc7b0f77cf42208fdd7a261869291720a30b0ebe',
 'tests/integration/test_serializable_long.sn': '3d6305a4017b251a92adfed3d68646d3e7c5b0e459e2caa3577066d9b8c6e0f9',
 'tests/integration/test_serializable_push_ownership.sn': '96deaed5a5fefbe18af40a02d7d0ea7d494eb4b0aabcac5c8dba6b7e559c184f',
 'tests/integration/test_serializable_return_nested_array.sn': '6733ad1573b869c28650930c4d80ee1042f00ffb46f777dd55078001751a5be9',
 'tests/rust-native/native_serial_hygiene.sn': '3441935ae08bb2f08c9c1a9aaed9ffa91f4c620db6cdc246f1dd940191c2decc',
 'tests/rust-native/native_serial_lifetimes.sn': 'c7120a3474f8998c8ca9a4ff4014392c3bfd79126e0af72d6e68ed4fae2057c7',
 'tests/rust-native/native_serial_mixed_handles.sn': '79a2d9815453db0417a40578afa85ef54d0929cce2fdcc91ecd493243bd29178',
 'tests/rust-native/native_serial_objects.sn': 'ba02fcc0de04d82a8c598ccf80c334b19d244d1fa2b919dce09a2c12cf624f3f',
 'tests/rust-native/native_serial_threads.sn': '3b6698e20dc810968af4e18448862589d5326eb2b9987f4ca19b51ed86d20c8d',
 'tests/integration/test_thread_struct_return_types.sn': '4107ecec3bc70eba14b449f78df723b47809739b3e050c23eaa5ffb98e705df8'}
HELPER_SHA256 = {'tests/integration/test_serializable.sn.c': '9bd612be9fb5842e0912ba765257a2bee026035d7a429e4281a096e663b2edd0',
 'tests/integration/test_serializable_encoder_cleanup.sn.c': 'ee5655d5519b1bf8a2a16602f81ba3204b8f7ada7048dca827516e3308810d48',
 'tests/integration/test_serializable_long.sn.c': '9bd612be9fb5842e0912ba765257a2bee026035d7a429e4281a096e663b2edd0',
 'tests/integration/test_serializable_push_ownership.sn.c': '986c5a9d81bf6c7dd43e1da03d9c3be87278b87619a0a076bbf4e35ed4edd761',
 'tests/rust-native/native_handle_arrays.sn.c': 'd65ca7c65ed1f795f05abc50b3987c1beb63bf45aeb71f1fc0e0464e771d5241',
 'tests/rust-native/native_serial_lifetimes.sn.c': '2599754a6ef9bdfaeb7c45316c8bd8b14d3d691f5a09e68c5512e6479498c09e',
 'tests/integration/test_thread_struct_return_types.sn.c': 'b3915f4cff92578ff1beca1b6c8b1467d55b7ed2057ad15be01c79356237d65f'}

def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 117 successful serialization cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source] or case['source_sha256'] != SOURCE_SHA256[source]:
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
    report['oracle_scope'] = 'Generated Rust serialization through C Encoder/Decoder vtables: supported scalar/nested/array fields, aliases, hygienic field locals, ordinary returns, push ownership, finalization caching, context cleanup, nil handles, mixed native handle arrays and joined record threads. General foreign callback transport, native serialized-record/array transport and cross-thread serialization handles remain full-goal requirements.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 117 independent serialization C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
