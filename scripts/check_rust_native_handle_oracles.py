#!/usr/bin/env python3
"""Frozen native-handle oracles, unchanged sources, and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

FIXTURES = {'tests/integration/test_native_ref_self_assign.sn': {'source_sha256': '5d002db9b99ea6693c4d37d5c4bb04cca010cca136c0a1d13c705e6a18db8f5e',
                                                      'oracle_sha256': '2203762be4be19ff0b7d6dd16984f14cd50b11b0e18a1253c5d71999d5dc37ab'},
 'tests/integration/test_native_resource_lifecycle.sn': {'source_sha256': '908c1586eb7a2e04b53e67e44176d0a21a7b61cc2c79a111917cf6df9b20494b',
                                                         'oracle_sha256': '66d46b4249ba86e92a994575b5bda02a94d2c1e9bfc6ab727153b4f0b18634d5'},
 'tests/integration/test_native_struct_ref_loop_cleanup.sn': {'source_sha256': '15783859baa80ddf712334ecb2205a95c5a47fcd41621fc1052c340b62fa53fb',
                                                              'oracle_sha256': 'cb002429b5234123042caaf208d72e9677dd6dd098928ef1dc57064bb9dceedf'},
 'tests/integration/test_refcount_arg_leak.sn': {'source_sha256': '6692d5e2d8b618eec36d758871f1511b6777e38cda3db86160cfb37f4f1ecb84',
                                                 'oracle_sha256': 'e7116f2b8858d208786e118e48e32ae983f788db9dc41373b0ced48232e7a74a'},
 'tests/integration/test_refcount_chain_in_struct_literal.sn': {'source_sha256': '9c67fcd6dae794fd2c63eac42f6a6a0e37a8545d5ebc720dfb4ad86b06b0a780',
                                                                'oracle_sha256': '97da1d0134d250feebe11c4c76b9490553a3120ca534157406d3eb9b849245da'},
 'tests/integration/test_str_return_as_arg_leak.sn': {'source_sha256': 'bd1cf85e17b6c4f598cdf81976cccd7b078ff2547d650fb39cf562835162a930',
                                                      'oracle_sha256': '768fe350dda92b6c5bc30d49ac59c0a81ec0a7c2a3473c29a2a59176cb1e9024'},
 'tests/integration/test_struct_return_array_leak.sn': {'source_sha256': 'd767e1c2ac752f7423c2a642f4c4d2c7beed2646ad9838ebf71390cb07a3b8df',
                                                        'oracle_sha256': '9aa7ad0b75b71d4287469d64e8c13a4001b8a54f38b2a88eec0b23d54388d67a'},
 'tests/integration/test_struct_rvalue_member_leak.sn': {'source_sha256': '61a9e58ebbfda20d6c2bba40ebf2c6471f94d5e8407103fe3f9c8fced42eead3',
                                                         'oracle_sha256': '1af5dd87203cc246376a46a0e4fb376839ed1147deef26d5eba92f69b869096e'},
 'tests/integration/test_struct_rvalue_member_leak_contexts.sn': {'source_sha256': '0e533a12b5de8d2f38ef5e4341c45e7fc66184713a25939f04b98087880b34a7',
                                                                  'oracle_sha256': '543b8611467098ee25f15fe286d2829e5bcdc4032cecbc5513895b0b439a726d'},
 'tests/integration/test_native_ref_return_evaluation.sn': {'source_sha256': '08a05bb78de46a7e7de80ba27e094b6f618adf44a505ab223916868d0fd94e9e',
                                                            'oracle_sha256': '89d93813de82f977e0b6967b962eb18ff2a788d76a195b61f70f25e7b1debc20'},
 'tests/rust-native/native_handle_record_borrows.sn': {'source_sha256': 'd916efaf5da7c787468e16782c1c4e67b88179711f9a8a57d247d46e0c8241e5',
                                                       'oracle_sha256': 'a3a239e81fbbe9000106b2ac69c3d465eddaccae0e4dc33c982835fe342201f0'},
 'tests/rust-native/native_handle_method_only.sn': {'source_sha256': '434a66feda00405df37c2b8ea59570bd24abbf4b88e56a5edeaa27acba55c66e',
                                                    'oracle_sha256': '68e84b3367e8e826d6bb8fb9c7e5a2b5705a0eb78dae12837479c4f704c40715'},
 'tests/rust-native/native_handle_default_methods.sn': {'source_sha256': 'aa97012770057a109018d8c7c860b133d5f223cfe34d0204e8dbf95aa2400433',
                                                        'oracle_sha256': '92d78baee7cddf051633193d163d1493b376234c3ff716caa840840b0eed78ed'}}
NATIVE_HELPERS = {'tests/integration/test_native_ref_return_evaluation_helper.c': 'bfabd1480485d73f7ac0952c5d51edadf35a9165e43e93cbb8c3fe69f2370ec2', 'tests/rust-native/native_handle_default_methods.sn.c': 'f696b5f5686dfcce09eabed9beecd06af6760323776c2e257f73adcc18414c0a'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in FIXTURES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 117 successful native handle cases')
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
        'Canonical C ownership, default parameter and method borrows, borrowed/retained/fresh/nil results, '
        'once-only temporary/nested/indexed argument evaluation, visible reference counts, owned aliases, '
        'native scalar field reads, method-only/default-name declarations, receiver-name hygiene, using disposal and scope-lived declaration temporaries, shared/nested record and array-element projections, and resource cleanup. '
        'Nine unchanged original programs and four ownership controls; remaining native arrays, global storage, '
        'managed native record parameters, callbacks, field stores and broader contracts stay separate work.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 117 independent native handle C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
