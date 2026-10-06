#!/usr/bin/env python3
"""Check frozen C/Rust closure array method contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_array_required_methods.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_int.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_string.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_capture.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_required_methods_index.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_argument.sn': b'true\ntrue\n', 'tests/rust-native/scalar_required_methods_floating_variable.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_array_required_methods.sn': '87103a321d8e757b3eda1c03f113fc54983dd83fc486680594d6f675d75ee0e7', 'tests/rust-native/scalar_required_methods_int.sn': '6e0cf0fbbd4765bc532aaf601f72fa8ae8f4660d3db63dd6928148491b82a4da', 'tests/rust-native/scalar_required_methods_string.sn': '5fbdb211a0808ac703649a440a2655b0e5a5907b2ede70823b46f86001d42cf5', 'tests/rust-native/scalar_required_methods_nested.sn': '873d3ab20586261437a9240e9a0534e559c6677ca52fad24bcec0b60fe5752a6', 'tests/rust-native/scalar_required_methods_capture.sn': '2a352601b97aad93524ad44d62f04ce9ef215d55f37392ab8f9fda8768c6cbb7', 'tests/rust-native/scalar_required_methods_index.sn': 'eb64aee2d90c05cc3ddda3285843e65a590ce2ae738d64bb4174e46c4341c235', 'tests/rust-native/scalar_required_methods_argument.sn': '1954664583bf3e21f07b633af3fc2ebd16b74b2db48ba7fc248b64b27d9205d4', 'tests/rust-native/scalar_required_methods_floating_variable.sn': '3f98bbfb2450eddf0c7ae7c3290e795f8ca62549446fd8b1bab0c25611876bc2'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 72 successful closure array method cases')
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
    report['oracle_scope'] = ('Reverse, clear, insert and remove for shared closure array parameters and private captures, alias visibility, strings, nested rows, callback-bearing arguments and indices, and typed float insertion.')
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 72 independent closure array method C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
