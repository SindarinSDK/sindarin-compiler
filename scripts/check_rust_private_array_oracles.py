#!/usr/bin/env python3
"""Check frozen C/Rust private closure array binding contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_private_array_basic.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_self.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_aliases.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_nil.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_forward.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_closure_forward.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_captures.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_mixed_values.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_partitions.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_native.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_private_array_native_aliases.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_private_array_asval_result.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_private_array_rhs_effects.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_private_array_sizeof.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_private_array_basic.sn': '865cab686d8169cb51e5dc1d3957c24e7b835280bb09aab848d7fa7fd5add49e', 'tests/rust-native/scalar_closure_private_array_self.sn': 'edba1754d602a5facf8f8b15e8f952984d99ac2a37f0a50ab89291887e424f0f', 'tests/rust-native/scalar_closure_private_array_aliases.sn': '64c04ab832094f6a13d401a608f8841885cc651897d4a3413982a3f8d4b65641', 'tests/rust-native/scalar_closure_private_array_nil.sn': '16d358c958b3644ad28fcf2290b2f1c7d0147361c5a29b39dc283641ba8fe024', 'tests/rust-native/scalar_closure_private_array_forward.sn': '40a6c43b653b695503c611e15c077fb65d10f3facd5640e1a8539fb905797b99', 'tests/rust-native/scalar_closure_private_array_closure_forward.sn': '073189b1261acc99135db411dd237b6633757cc96b78cce91b9b8da066740abb', 'tests/rust-native/scalar_closure_private_array_captures.sn': 'f32afc4e39ac61757d3b333dc92bb86016e25ab03cd69d8f7283f5bf24397347', 'tests/rust-native/scalar_closure_private_array_mixed_values.sn': 'e98c97e1678e6ec35532f6f19eda334f6487f41911dac1b3f7e39cb087cdd344', 'tests/rust-native/scalar_closure_private_array_nested.sn': '3b9cb6cde622eaf2332668991cf47d2bee0dc25c455dcc631ef6de57f6d57450', 'tests/rust-native/scalar_closure_private_array_partitions.sn': '9444a7ac00e732eb4ca3d43bc690fda8cf1a9ae8d49086c8f25790e98225deb8', 'tests/rust-native/scalar_closure_private_array_native.sn': '22f2bc85841d519c0d1d452211c58c6ce555102fb3ae4987bb9987ed18b64ce8', 'tests/rust-native/scalar_closure_private_array_native_aliases.sn': 'a2ef3bc963b9076a9f692fe601873598d56d2ee5f734c3d627eedb6b445adedd', 'tests/rust-native/scalar_closure_private_array_asval_result.sn': '0843ae2d059b7b1562e594c896017b9fa520becf2800671c9e44c61229a5f7d3', 'tests/rust-native/scalar_closure_private_array_rhs_effects.sn': '71635defbfb7cdc7046fea7b89d1d08fc2e1cc87e9360f9c98e066bf4e527834', 'tests/rust-native/scalar_closure_private_array_sizeof.sn': '8cf4d0ba3effcfd775283c9a68dcdf29be572222f2e766f885da45cc26c1f324'}

HELPER_SHA256 = {'tests/rust-native/scalar_closure_private_array_native.sn.c': 'd7aad44913b28488f5491c72fb873f28efff91c41c79f2846b4754917fe0fd6c', 'tests/rust-native/scalar_closure_private_array_native_aliases.sn.c': 'd7aad44913b28488f5491c72fb873f28efff91c41c79f2846b4754917fe0fd6c'}

def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 135 successful private array binding cases')
    for source, digest in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != digest:
            raise ValueError(f'native helper changed: {source}')
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
    report['oracle_scope'] = 'Default formals preserve caller aliases until private rebinding: self/nil/returns, forwarding/captures/mixed qualifiers/nested arrays/dynamic partitions, native handle buffer identities/refcounts/cleanup, once-only RHS effects and C sizeof wire width.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 135 independent private array binding C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
