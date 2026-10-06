#!/usr/bin/env python3
"""Check frozen C/Rust owned closure array rebinding contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_array_rebinding_basic.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_captures.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_self.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_rhs.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_strings.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_records.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_two_copies.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_native.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_index_effects.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_array_rebinding_hygiene.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_array_rebinding_basic.sn': '62989d50507ce8a4714b9fbf9967ee0b9807793ef23594b88f503f9be21cd425', 'tests/rust-native/scalar_closure_array_rebinding_captures.sn': '1c0879327cddfbdce36ccc7bd21fee4083f0da61ba44be250ba51ff1dd02284c', 'tests/rust-native/scalar_closure_array_rebinding_self.sn': '839931a7b9d7c822ba66c705ab70e9be781d928965e837455feaadc542e036a7', 'tests/rust-native/scalar_closure_array_rebinding_rhs.sn': '7402325c5850939746e423047df8321fe9b7948ac3033f4a2e96eda8a52cda4b', 'tests/rust-native/scalar_closure_array_rebinding_nested.sn': 'b31313063a4b26e6327f2e9f95e637445edac567b2770a3dbf8c2693e0c96c87', 'tests/rust-native/scalar_closure_array_rebinding_strings.sn': '0526bda82a0d1e987ffc3a4a2e9d6530497d0b7af21bacfb050858ddaf22e0ba', 'tests/rust-native/scalar_closure_array_rebinding_records.sn': 'be8248ab1609067293522edafd6e2eb03fda9d9551529127a0f561c374da455b', 'tests/rust-native/scalar_closure_array_rebinding_two_copies.sn': 'efc79e130f67cbddc4e3d93336cd7a7cd94e08dc27d4b6bc1685e469e57e81cd', 'tests/rust-native/scalar_closure_array_rebinding_native.sn': '1b3f8b59748ae20a60ecddfa437f122eec06cdcf1f08475b2758df8a3cb02396', 'tests/rust-native/scalar_closure_array_rebinding_index_effects.sn': 'c62b40d19370686568b938f08c8de7e470f4ab525dcab3674ec75212f288f874', 'tests/rust-native/scalar_closure_array_rebinding_hygiene.sn': '625e65d80acd37cb2ffaee9685eb9ff24e2c0248db3643c95c0ebe1b8e3f8504'}
HELPER_SHA256 = {'tests/rust-native/scalar_closure_array_rebinding_native.sn.c': 'd7aad44913b28488f5491c72fb873f28efff91c41c79f2846b4754917fe0fd6c'}


def verify(path):
    report = json.loads(path.read_text())
    for source, digest in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != digest:
            raise ValueError(f'native helper changed: {source}')
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 99 successful owned array rebinding cases')
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
    report['oracle_scope'] = 'Per-call as-val array rebinding, snapshot captures, self/RHS evaluation, nested/record/string ownership and original record-element storage, native borrowed arguments/reference credits and helper hygiene.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 99 independent owned array rebinding C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
