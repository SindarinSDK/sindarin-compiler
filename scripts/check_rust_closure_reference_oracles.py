#!/usr/bin/env python3
"""Check frozen closure reference storage, aliasing and evaluation contracts."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_reference_basic.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_alias.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_forward.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_fields.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_reference_field.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_operators.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_effects.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_hygiene.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_types.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_named_forward.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_named_alias.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_owned_effects.sn': b'kept\ntrue\ntrue\nkept\n', 'tests/rust-native/scalar_closure_reference_method_alias.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_closure_reference_mixed_array.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_reference_integer_order.sn': b'4\n4\n', 'tests/rgen/closure_values_qualified_signature.sn': b'1\n', 'tests/rgen/shared_frontend_as_ref_method_field_arguments.sn': b'4\n13\n', 'tests/rgen/shared_frontend_checked_ref_parameter_byte_mutations.sn': b'17 0 62 0 5 4 2 2 2 2 255 0 4 4 13\n', 'tests/rgen/shared_frontend_checked_ref_parameter_int32_mutations.sn': b'17\n0\n62\n0\n5\n4\n2\n2\n2\n2\n2147483647\n-2147483648\n4\n4\n13\n', 'tests/rgen/shared_frontend_checked_ref_parameter_uint32_mutations.sn': b'17 0 62 0 5 4 2 2 2 2 4294967295 0 4000000000 4000000000 4 4 13\n', 'tests/rgen/shared_frontend_checked_ref_parameter_uint_mutations.sn': b'17 0 62 0 5 4 2 2 2 2 true true 0 true true 4 4 13\n', 'tests/rgen/shared_frontend_floating_as_ref_parameter.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rgen/shared_frontend_floating_ref_parameter_mutations.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rgen/checked_ref_parameter_mutations.sn': b'17\n0\n62\n0\n0\n-1\n0\n1\n0\n0\n2\n2\n', 'tests/rgen/float_mutation_precision.sn': b'true\n1\ntrue\ntrue\ntrue\ntrue\n2\ntrue\ntrue\n3\ntrue\nfalse\ntrue\n', 'tests/rust-native/scalar_closure_reference_native.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_reference_basic.sn': '54df2067bd1d0cf5918ed4e94bdab14e63b72bce0eb9e4ad286f1958d33b5d33', 'tests/rust-native/scalar_closure_reference_alias.sn': 'a44919bc6ab6caf1e91bf10f9e5d2cc86eab03cceca17e080d464d44dc66ef51', 'tests/rust-native/scalar_closure_reference_forward.sn': '40ef33ec63694ab325148cea5b9a46c3d0f67d643a356ff1eeaeac5abc5d1850', 'tests/rust-native/scalar_closure_reference_fields.sn': '273f8ea3831e783bfcfaae1cd4f6d099e5bfeb67b1c5ff2672552eb294f900c8', 'tests/rust-native/scalar_closure_reference_reference_field.sn': '354b9b4d9e57add82eed86cd2a22b8b5c7dbf760ba257138476eaffb52c501d0', 'tests/rust-native/scalar_closure_reference_operators.sn': '2d4ddc42a1135971f91e23491656452d401b6ffcfdda23f39d861779d7dd14b6', 'tests/rust-native/scalar_closure_reference_effects.sn': '5ae2eb85fc336668dbafaef0646916b30a44a49f887b15e81df04f97c6692a68', 'tests/rust-native/scalar_closure_reference_hygiene.sn': '3e7c389e40fe37339408b8b587c7fe17aa0e815d26581b191141deb68734b052', 'tests/rust-native/scalar_closure_reference_types.sn': '43f4835c07bbcecb606ef36015785f0f25ef6effe7e2e99e929e228561ec385a', 'tests/rust-native/scalar_closure_reference_named_forward.sn': 'a1d549d77558dbe06d4841ca69353ec0b3232c5d30adb91adcf4f0ccbe2bf15e', 'tests/rust-native/scalar_closure_reference_named_alias.sn': 'e4ca4ef55429e7073ce8b8b38825ee314215fc09234bcbba6bd060ac6bbeda8c', 'tests/rust-native/scalar_closure_reference_owned_effects.sn': '05a4f4bb91c4d8075ae53d4cef1833b4b92a0d3efd5d5796a4235e2246b845a5', 'tests/rust-native/scalar_closure_reference_method_alias.sn': '1bcf430eef9e46071d7f9a1d504cfaf0c5558e121c5f1e85d743d846cad7b9ee', 'tests/rust-native/scalar_closure_reference_mixed_array.sn': '0add7955659aa85daf5f3a94d4e683fd087a717f28be0f293813f1dc91d65dfe', 'tests/rust-native/scalar_closure_reference_integer_order.sn': '6b0e706282b391d6bbb0d83b642aed8678cb79d96e42d19a0d7483365bdd79e5', 'tests/rgen/closure_values_qualified_signature.sn': '9598edb17b860cc1cd8d01f0c5cdd987289fcef339efb607de9c9508143f12fc', 'tests/rgen/shared_frontend_as_ref_method_field_arguments.sn': 'b428105dc21be6d664937642a9ad03db145fe0189f4fde052cc3a254b11a1c2a', 'tests/rgen/shared_frontend_checked_ref_parameter_byte_mutations.sn': 'b7125ff43deabf45c5da6174deabc18c6b3d191cc8f54f9c5cf175463d6e45c1', 'tests/rgen/shared_frontend_checked_ref_parameter_int32_mutations.sn': '4febf9994763a6343a45ad196a4b220eff2caa6e09c0f537177b64fa7ca88d2b', 'tests/rgen/shared_frontend_checked_ref_parameter_uint32_mutations.sn': '660f270e24fa89b547806d882a4afde2d9fe79da11feae08cabfffd40ca3146c', 'tests/rgen/shared_frontend_checked_ref_parameter_uint_mutations.sn': 'fa0766e9f284c015d109edef2f18dad18748f0e5b9065b71a49b0c7354e68818', 'tests/rgen/shared_frontend_floating_as_ref_parameter.sn': '35c58e5627dc8a1f37d5df2ff16d3577771f9c83e35a68c7b19a0b6090e564b0', 'tests/rgen/shared_frontend_floating_ref_parameter_mutations.sn': 'dd97923b1ab880870ef3e78d6a0e7b06f247bfd7905d3ed2e560e235a3996516', 'tests/rgen/checked_ref_parameter_mutations.sn': '993f9efa6c053f250bd4ffd4d4e6565389e7e06ed598abd2620d78150f7975fa', 'tests/rgen/float_mutation_precision.sn': '52bf1d423d1e86dd02c114e8474c6603fc9c4ab05bf95d7492e975db8c329e93', 'tests/rust-native/scalar_closure_reference_native.sn': '55e45743c5a0562c80800fe7d67a5dee26212922b34905e7d5177dcd1ce7b6fb'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 234 successful closure reference cases')
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
    report['oracle_scope'] = 'Scalar closure references, shared record fields, dynamic aliases through named/method callees, mixed arrays, once-only effects and owned strings, primitive types, and prior-value reads before effectful reference compound RHS.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 234 independent closure reference C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
