#!/usr/bin/env python3
"""Check frozen C/Rust closure array contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_array_alias_control.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_array_forward_control.sn': b'true\n', 'tests/rust-native/scalar_closure_array_index_alias_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_string_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_distinct_index_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_nested_forward_control.sn': b'true\n', 'tests/rust-native/scalar_closure_array_temporary_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_field_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_named_alias_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_captured_forward_control.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_array_nested_owned_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_return_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_expression_return_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_named_distinct_control.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_array_value_record_method_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_reference_record_method_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_record_method_argument_control.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_effects_control.sn': b'201\n1\n2\n', 'tests/rust-native/scalar_closure_array_capture_control.sn': b'1\n2\n', 'tests/exploratory/test_array_mutation_lambda.sn': b'Test array mutation inside lambda:\n\nTest 1 - Lambda reading captured array:\n  Sum of [10, 20, 30] via lambda: 60\n\nTest 2 - Lambda reading at specific index:\n  arr2[0] = 1\n  arr2[2] = 3\n  arr2[4] = 5\n\nTest 3 - Lambda modifying array element:\n  Before: arr3[1] = 200\n  After modify_arr(1, 999): arr3[1] = 200\n  modify_arr returned: 999\n\nTest 4 - Lambda with array parameter:\n  sum_arr([1,2,3,4,5]) = 15\n\nTest 5 - Lambda modifying parameter array:\n  Before: arr5 = [5, 10, 15]\n  After double_arr: arr5 = [10, 20, 30]\n  double_arr returned: 10\n  Array was modified (pass by reference)\n\nTest 6 - Nested lambda with array:\n  Nested lambda sum of [7,8,9]: 24\n\nAll array mutation lambda tests PASSED!\n', 'tests/rust-native/scalar_closure_array_hygiene_control.sn': b'true\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_array_alias_control.sn': 'e869e851707df0e5c463ad1cae7e712c3246ea344fe10d363fff8fd3efc5e419', 'tests/rust-native/scalar_closure_array_forward_control.sn': '1b9f8e7c9b2c78f0f178a7a9848950d7f04403a06677c76d587663e96a7cde2e', 'tests/rust-native/scalar_closure_array_index_alias_control.sn': '501798064e561aaffff3329819a1b0627d092467b538c081aa95bdc022c953ab', 'tests/rust-native/scalar_closure_array_string_control.sn': '6d83b9f788e83e5d3cc6d9b79b5363d33c6259d2d24f7b066428e3ed9917d820', 'tests/rust-native/scalar_closure_array_distinct_index_control.sn': '7c67a1e20b62289802bc1f34582bd274005b0adf7c11a79b54eff3e125fecef5', 'tests/rust-native/scalar_closure_array_nested_forward_control.sn': 'bd19267faf44b594c8aca1d510cddee94aa53f3fb3bee625765b0d561013d0f2', 'tests/rust-native/scalar_closure_array_temporary_control.sn': '767815e8dd6ea09e3e58bf959fd5632acd870b78424d54ca2b351034dc210995', 'tests/rust-native/scalar_closure_array_field_control.sn': '6b5b92e32fcb66fa3a5c88f39b2dd9b056b0653314f4781744d348aef75fa9d5', 'tests/rust-native/scalar_closure_array_named_alias_control.sn': '3c4c02bf1f4737d901777652f42c43f49b1d17dc3c98f541ce78be83ba415df3', 'tests/rust-native/scalar_closure_array_captured_forward_control.sn': '2832ac126bb5fced12be320a092c3e28790e67dbfd522a43c064da8887700635', 'tests/rust-native/scalar_closure_array_nested_owned_control.sn': '4927ab9ac96b1f147f0dcbca534017f4f9f13e0a580d3894aaf193b7c7b1ac9c', 'tests/rust-native/scalar_closure_array_return_control.sn': 'e374085d1fa3e31100d0264ca11eb3f3126790a6943e924f809612303100d4d0', 'tests/rust-native/scalar_closure_array_expression_return_control.sn': '0fab108a43e880d7d448ce293dddf8bf9eb33e85d98acc65946fe0e21fdc5d31', 'tests/rust-native/scalar_closure_array_named_distinct_control.sn': '8ff214ea9f238f37e10a764543204a5d3aa01e8333a5837153a633783e9d728e', 'tests/rust-native/scalar_closure_array_value_record_method_control.sn': 'b8e4d3ed6506837f34365ff1c6d0b9d5a2c262ac80b86a0ff2d100742f738e8d', 'tests/rust-native/scalar_closure_array_reference_record_method_control.sn': '45dbcdce511f1258554ea5ddbdcbdf4beec232c86f94d48c947402f9d0a0e1c5', 'tests/rust-native/scalar_closure_array_record_method_argument_control.sn': 'd24c5e8400721418db23891133de467907257638f72040c7cd1da4bccf09a1f9', 'tests/rust-native/scalar_closure_array_effects_control.sn': '6a8158202e12a14fef6f85b00e26b34b2a8d12a56638f72c4d6e64d70944882d', 'tests/rust-native/scalar_closure_array_capture_control.sn': '1f78a7ce7f13c7405e63d2e15451965dd0d196cd6dc0a91f1a6cabe2ffcaca7c', 'tests/exploratory/test_array_mutation_lambda.sn': '627fc6ff1b9d3d5b2de753df9c2d8f6e6358307dae0a7258a3994b517709c3a8', 'tests/rust-native/scalar_closure_array_hygiene_control.sn': '2fc5a1fb96ca91f06ee5e1c41b8e86431db7e41c02919eab93472c27451a0640'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 189 successful closure array cases')
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
        if (Path(source).with_suffix('.expected').exists()
                and Path(source).with_suffix('.expected').read_bytes() != expected):
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
    report['oracle_scope'] = ('Shared default-array closure parameters: mutation, aliases, snapshots, returning independent arrays, fields, nested/temporary arrays, strings and records, callable forwarding, record method argument callbacks and generated-name hygiene. Unchanged original exploratory program included.')
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 189 independent closure array C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
