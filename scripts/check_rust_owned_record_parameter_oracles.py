#!/usr/bin/env python3
"""Check frozen C/Rust owned record closure parameter contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_owned_record_parameter_basic.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_array.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_method.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_two_copies.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_return.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_capture.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_effects.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_places.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_rebind.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_field_string.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_array_methods.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_nested_method.sn': b'true\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_method_arguments.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_receiver_alias.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_array_alias.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_owned_record_parameter_dynamic_alias.sn': b'true\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_owned_record_parameter_basic.sn': '4e76d4ac4223736e3719fe6088893af4ffccfe4b283e87f2c8257ecd671099f1', 'tests/rust-native/scalar_owned_record_parameter_array.sn': '08ca8fcd9628be0ffd2e964908a7da3ef73a52b9d0584e52ea178ab0a4c57f7a', 'tests/rust-native/scalar_owned_record_parameter_method.sn': '21145b6496a07e3069647775cb4b50d6fb972a7b2bc21bc9b327286577a161f9', 'tests/rust-native/scalar_owned_record_parameter_two_copies.sn': '414f6f441418dab571d7f98bb86a22b322aee0fc5a62bad7acb98aa79f31e826', 'tests/rust-native/scalar_owned_record_parameter_return.sn': '8753f5748def0d98e1e1fbbb2e874c2178b917ec0c5f2df8ffb27b1923a4c6ed', 'tests/rust-native/scalar_owned_record_parameter_capture.sn': 'b4d37341768363df0defeca435121f2928c391c35001f1742c06385efd4e9f14', 'tests/rust-native/scalar_owned_record_parameter_nested.sn': '4cf70ce40f0f40f688cbf844737334b3eac42ca2040bf6b32526a65e3fb8791d', 'tests/rust-native/scalar_owned_record_parameter_effects.sn': '70e59be655432153a7457fe669a8fee427796a52abd3872eb2c21600dd9f6142', 'tests/rust-native/scalar_owned_record_parameter_places.sn': '548542f324b029f12e9051f362a9999848031dd511f5c18b6487942348501580', 'tests/rust-native/scalar_owned_record_parameter_rebind.sn': 'b8e447e1d5aadac2aabf4e8fab72ba4605589d26bfebcad96c6137a5015d15de', 'tests/rust-native/scalar_owned_record_parameter_field_string.sn': '4cee758e1e194f778d3c474b134f4e660750e9b3ebcaf6db67d0aef603c3628f', 'tests/rust-native/scalar_owned_record_parameter_array_methods.sn': 'a9c39eb0eb850e34b56dfe3766d3181a9a249dd0536e98e5796a6527705a29fe', 'tests/rust-native/scalar_owned_record_parameter_nested_method.sn': 'bf4a2c92ac314cbd85d96919b05af1453a6fef6090a283a60f68382a56c400df', 'tests/rust-native/scalar_owned_record_parameter_method_arguments.sn': '81fa97dabe62585d5593f4b850d47ac66cb30bd56855b7ba0a01efbfbf289847', 'tests/rust-native/scalar_owned_record_parameter_receiver_alias.sn': '7524d5de9901f06e02e57c3ac6160189d1c91ce237483dc93d9b1ee281a7dbd7', 'tests/rust-native/scalar_owned_record_parameter_array_alias.sn': '038f8c074f5c63a252249d87ffa4d781637dd0d42dd862076ad3234f5ef4422b', 'tests/rust-native/scalar_owned_record_parameter_dynamic_alias.sn': '3cdadb44c9295d486e4cb95278b5154f3d53437b6681ff132d20d6b52f30fdb6'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 153 successful owned record parameter cases')
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
    report['oracle_scope'] = 'Owned as-val record copies, independent fields/arrays/returns/captures, nested mutation receivers, once-only argument effects, stable actual places, and receiver/static/dynamic array aliases through nested methods.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 153 independent owned record parameter C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
