#!/usr/bin/env python3
"""Check frozen C/Rust scalar closure value parameter contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_value_parameter_default.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_asval.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_types.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_postfix.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_forward.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_closure_forward.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_shadow.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_rhs.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_capture_mutation.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_expression.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_boundaries.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_floating.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_value_parameter_hygiene.sn': b'true\ntrue\n', 'tests/rgen/closure_values_parameter_mutation.sn': b'7\n', 'tests/rgen/floating_by_value_parameter_lambda_compound_rhs_precedence.sn': b'3.00000\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_value_parameter_default.sn': 'c899912b2d118c8cef8bb5d27f321238006a7853fc2216a441200c2d40f78251', 'tests/rust-native/scalar_closure_value_parameter_asval.sn': '29a4884b145b1a39f0c537fd908a031422eb38b511fffe32e6ff702cd17b576e', 'tests/rust-native/scalar_closure_value_parameter_types.sn': '5161a2f63cb5e354ea6ec46cdda0d2acc6a18a5d1f3169a2b8a30c8a94f3944e', 'tests/rust-native/scalar_closure_value_parameter_postfix.sn': '80ca9f16cf93b434b9b99ae18fdb66fdd33eb5a021149980269a18c4b0b0f342', 'tests/rust-native/scalar_closure_value_parameter_nested.sn': 'a983c85543a495232d69905860335e8848bef5d53094d2967fa7372d64958d2c', 'tests/rust-native/scalar_closure_value_parameter_forward.sn': '53114ce430a4441a439285f81ee988fd152fb2ed1de857d9afd1b6434efbabc0', 'tests/rust-native/scalar_closure_value_parameter_closure_forward.sn': '4adfb62394132836d7338be5dffed17411f0eba17b4894a6b50ec1e925fb3bda', 'tests/rust-native/scalar_closure_value_parameter_shadow.sn': 'ce30901f3c0586bcc3cf16e6ab72d51aed85f15791051d01ad8ab5d5dcf2a77e', 'tests/rust-native/scalar_closure_value_parameter_rhs.sn': '9e497b5ea7ed2ea561b39dd75c19e9b02e161d51190e99e2f45c54325acd1d07', 'tests/rust-native/scalar_closure_value_parameter_capture_mutation.sn': '5434eba2035cf5e26e9719fdd6bec2f05716fe9a16c5741ffca33367500d3c62', 'tests/rust-native/scalar_closure_value_parameter_expression.sn': '02310f7b987f6122651919be5b0029ee008f39cee71221a01012af21c3591b48', 'tests/rust-native/scalar_closure_value_parameter_boundaries.sn': '5259e57208ca570fe46a2845cf53443d19470f23974e7d0eda090a4a8bf82257', 'tests/rust-native/scalar_closure_value_parameter_floating.sn': '58c84fc61939f96468faf2a80d76a226b5cfd87819c45318471d2c45cb1da401', 'tests/rust-native/scalar_closure_value_parameter_hygiene.sn': '84d9f76626f718072a8423ae8eb7be21fe7a7bc7ef4684293f1dbebc34204fe1', 'tests/rgen/closure_values_parameter_mutation.sn': 'c0ff41adeb85bb576f5da9729a65a4a15b19a265446d7a727ff570ed4b5da19c', 'tests/rgen/floating_by_value_parameter_lambda_compound_rhs_precedence.sn': '34fc9e4f2f893019532e5233b27d66ef001f47a182fbc68016b7700de4e6bebd'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 144 successful scalar value parameter cases')
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
    report['oracle_scope'] = 'Per-invocation default/as-val scalar parameters, primitive widths and float precision, postfix and assignment results, reference forwarding, lexical shadowing, nested snapshots, wrapping limits and once-only RHS effects.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 144 independent scalar value parameter C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
