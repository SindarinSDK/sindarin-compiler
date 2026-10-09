#!/usr/bin/env python3
"""Check frozen C/Rust structural interface contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/integration/test_builtin_interfaces.sn': b'Test 1 passed: Comparabl'
                                                 b'e\nTest 2 passed: Hashabl'
                                                 b'e hash()\nTest 3 passed: '
                                                 b'Hashable equals()\nTest 4'
                                                 b' passed: Stringable\nTest'
                                                 b' 5 passed: Copyable\nAll '
                                                 b'builtin interface tests pass'
                                                 b'ed!\n',
 'tests/integration/test_interfaces.sn': b'Test 1 passed: Score.compare works c'
                                         b'orrectly\nTest 2 passed: Score sa'
                                         b'tisfies Comparable\nTest 3 passed'
                                         b': UserId.hash and UserId.equals work'
                                         b' correctly\nTest 4 passed: UserId'
                                         b' satisfies Hashable\nTest 5 passe'
                                         b'd: Color.toString works correctl'
                                         b'y\nTest 6 passed: Color satisfies'
                                         b' Stringable\nTest 7 passed: Playe'
                                         b'r satisfies Comparable and Stringabl'
                                         b'e\nAll interface tests passed!\n',
 'tests/rust-native/native_interface_array_copies.sn': b'true\ntrue\ntrue\nt'
                                                       b'rue\ntrue\ntrue\ntr'
                                                       b'ue\ntrue\ntrue\ntru'
                                                       b'e\ntrue\ntrue\ntrue'
                                                       b'\ntrue\ntrue\ntrue\n'
                                                       b'true\ntrue\ntrue\nt'
                                                       b'rue\n',
 'tests/rust-native/native_interface_array_storage.sn': b'true\ntrue\ntrue\nt'
                                                        b'rue\ntrue\ntrue\ntr'
                                                        b'ue\ntrue\ntrue\ntru'
                                                        b'e\ntrue\ntrue\ntrue'
                                                        b'\ntrue\ntrue\ntrue\n'
                                                        b'true\ntrue\ntrue\nt'
                                                        b'rue\ntrue\ntrue\ntr'
                                                        b'ue\n',
 'tests/rust-native/native_interface_captured_array_alias.sn': b'false\nfalse\n'
                                                               b'false\ntrue\n',
 'tests/rust-native/native_interface_captured_field_arrays.sn': b'true\ntru'
                                                                b'e\ntrue\nt'
                                                                b'rue\ntrue'
                                                                b'\ntrue\ntr'
                                                                b'ue\ntrue\n'
                                                                b'true\ntru'
                                                                b'e\ntrue\nt'
                                                                b'rue\ntrue'
                                                                b'\ntrue\ntr'
                                                                b'ue\ntrue\n'
                                                                b'true\ntru'
                                                                b'e\ntrue\nt'
                                                                b'rue\ntrue'
                                                                b'\ntrue\ntr'
                                                                b'ue\n',
 'tests/rust-native/native_interface_default_values.sn': b'true\ntrue\ntrue\nt'
                                                         b'rue\ntrue\ntrue\ntr'
                                                         b'ue\ntrue\ntrue\ntru'
                                                         b'e\ntrue\ntrue\ntrue'
                                                         b'\ntrue\ntrue\ntrue\n'
                                                         b'true\ntrue\ntrue\nt'
                                                         b'rue\ntrue\ntrue\ntr'
                                                         b'ue\ntrue\n',
 'tests/rust-native/native_interface_empty_array.sn': b'true\ntrue\ntrue\nt'
                                                      b'rue\ntrue\ntrue\ntr'
                                                      b'ue\ntrue\ntrue\ntru'
                                                      b'e\ntrue\ntrue\ntrue'
                                                      b'\ntrue\ntrue\ntrue\n'
                                                      b'true\ntrue\n',
 'tests/rust-native/native_interface_field_arrays.sn': b'true\ntrue\ntrue\nt'
                                                       b'rue\ntrue\ntrue\ntr'
                                                       b'ue\ntrue\ntrue\ntru'
                                                       b'e\ntrue\ntrue\ntrue'
                                                       b'\ntrue\ntrue\ntrue\n'
                                                       b'true\ntrue\ntrue\nt'
                                                       b'rue\ntrue\ntrue\n',
 'tests/rust-native/native_interface_metadata_capture.sn': b'true\n',
 'tests/rust-native/native_interface_metadata_growth.sn': b'true\n',
 'tests/rust-native/native_interface_metadata_scope.sn': b'true\n',
 'tests/rust-native/native_interface_parent_field_parameter.sn': b'true\ntru'
                                                                 b'e\ntrue\nt'
                                                                 b'rue\ntrue'
                                                                 b'\ntrue\ntr'
                                                                 b'ue\ntrue\n'
                                                                 b'true\ntru'
                                                                 b'e\ntrue\nt'
                                                                 b'rue\ntrue'
                                                                 b'\ntrue\ntr'
                                                                 b'ue\ntrue\n'
                                                                 b'true\ntru'
                                                                 b'e\n',
 'tests/rust-native/native_interface_plain_record_copies.sn': b'true\ntrue\ntr'
                                                              b'ue\ntrue\n',
 'tests/rust-native/native_interface_returned_array_scope.sn': b'true\n',
 'tests/rust-native/native_interface_serial_runtime.sn': b'{"name":"Ada","age":'
                                                         b'42,"score":3.5,"acti'
                                                         b've":true,"missing":n'
                                                         b'ull}\ntrue\nAda\n42'
                                                         b'\ntrue\ntrue\ntrue\n'
                                                         b'true\n',
 'tests/rust-native/native_interface_storage_mutations.sn': b'true\ntrue\ntr'
                                                            b'ue\ntrue\ntrue'
                                                            b'\ntrue\ntrue\nt'
                                                            b'rue\ntrue\ntru'
                                                            b'e\ntrue\ntrue\n'
                                                            b'true\ntrue\ntr'
                                                            b'ue\ntrue\ntrue'
                                                            b'\ntrue\ntrue\nt'
                                                            b'rue\ntrue\n',
 'tests/rust-native/native_interface_temporary_fields.sn': b'true\ntrue\ntr'
                                                           b'ue\ntrue\ntrue'
                                                           b'\ntrue\ntrue\nt'
                                                           b'rue\ntrue\ntru'
                                                           b'e\ntrue\ntrue\n'
                                                           b'true\ntrue\ntr'
                                                           b'ue\ntrue\n',
 'tests/rust-native/native_structural_interfaces.sn': b'true\ntrue\ntrue\nt'
                                                      b'rue\ntrue\ntrue\ntr'
                                                      b'ue\ntrue\ntrue\ntru'
                                                      b'e\ntrue\ntrue\ntrue'
                                                      b'\ntrue\n'}
SOURCE_SHA256 = {'tests/integration/test_builtin_interfaces.sn': '6c26c2c105a8a5d2d05a022c72519ab79db428dbc0e6c1a4c8880a358fb43e1b',
 'tests/integration/test_interfaces.sn': '5f33120783a91040d094dbb50fef90aeb282ddf0e8beacdd4a58e96b8f817c11',
 'tests/rust-native/native_interface_array_copies.sn': '09b3554b89662de3f263beb534d7776965da68625d7f8b5c146a8fdf70dc01e2',
 'tests/rust-native/native_interface_array_storage.sn': 'be6153729cee47f25e20949b19f619fbe930abf0fcad38ecc87de63982ef962f',
 'tests/rust-native/native_interface_captured_array_alias.sn': '72716bed97054303e6f888ba4bd75eb7daf008b2b9f755aa1a7c8a5d02ead69d',
 'tests/rust-native/native_interface_captured_field_arrays.sn': '3d92a5a1cb19430f256126cc9bffb8e42bf97fcd05ca81d48a0e50acfd0b32ac',
 'tests/rust-native/native_interface_default_values.sn': '5eb20a2892f93bbad26d81d1ce5b6dd9a5756c906e35dd2eae3df2acdc999571',
 'tests/rust-native/native_interface_empty_array.sn': '3188ae39a92550c0e5b69b785575b28bab187e2f9dc540ae1108f656717a9cd8',
 'tests/rust-native/native_interface_field_arrays.sn': '9aa17705a0441c08e9920e8ebfa0b66fe3248281d974fe15139127629433b82c',
 'tests/rust-native/native_interface_metadata_capture.sn': '6fbd36de62f11ff7457d435691c1b7e0e21ab8acd7dafd01d9d1fc1c46ac46a3',
 'tests/rust-native/native_interface_metadata_growth.sn': 'de430338b15dbf6ea16ad4f3d41675179152d1cc047aea985750676c63fe2ad1',
 'tests/rust-native/native_interface_metadata_scope.sn': '8067457d87ae2d2051fab63ad3170ad520109287c9a15edd1ec493be2bb729e7',
 'tests/rust-native/native_interface_parent_field_parameter.sn': '3f14157ca22a34b14e7dfdb75cab042b793ab041342a0dfb3821e36415c69fe9',
 'tests/rust-native/native_interface_plain_record_copies.sn': 'b916b9f8ea04731755750b9f75cc03f6c66db72882108635b726ff504aab40ab',
 'tests/rust-native/native_interface_returned_array_scope.sn': '2ede86af57de34bac2316c76034c1660a6291c9f7abdbc7526489829d486ba63',
 'tests/rust-native/native_interface_serial_runtime.sn': '7af2223b552d1d4e4f9d7aec9c42c4c4501dfad1a71470f31a9151d562972852',
 'tests/rust-native/native_interface_storage_mutations.sn': '955cb66bad66f4738be5c974a10031189fd6a27bff0738bdf7476f4ee21d9031',
 'tests/rust-native/native_interface_temporary_fields.sn': '7958e87efe59d9cb3f69914762fa767bab134cbdb4c5472284a1df8bcdfb3ab2',
 'tests/rust-native/native_structural_interfaces.sn': '814a2e90f1dcff939d249b2c27dbeca03f0944c4f61bce8a33d313def293629e'}
HELPER_SHA256 = {'tests/integration/test_serializable.sn.c': '9bd612be9fb5842e0912ba765257a2bee026035d7a429e4281a096e663b2edd0'}

# These four fixtures observe addresses of two distinct zero-sized C locals.
# Their historical GCC oracle remains frozen. C compilers may allocate the
# locals together instead, changing only this explicit source comparison.
EMPTY_LOCAL_COMPARISON = {
    'tests/rust-native/native_interface_array_copies.sn': 16,
    'tests/rust-native/native_interface_captured_field_arrays.sn': 16,
    'tests/rust-native/native_interface_empty_array.sn': 16,
    'tests/rust-native/native_interface_field_arrays.sn': 16,
}
PROBE_ORACLES = {'tests/rust-interfaces/scope_managed.sn': b'true\ntrue\n', 'tests/rust-interfaces/named_callable_assignment.sn': b'true\nfalse\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\nowned capture\nowned capture\ntrue\n', 'tests/rust-interfaces/nullable_callable_transport.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n4\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n'}
PROBE_SHA256 = {'tests/rust-interfaces/empty_capture_identity.sn': 'c64e7829d878eb51e1b6f21f32d29af676c7cab8486d9804d4f14fda91e813dd', 'tests/rust-interfaces/scope_identity.sn': 'ca86dc55d4c37ba68dc0ec4eebb2e3ec80110e47c2d607b95b8253c5b1872dd2', 'tests/rust-interfaces/scope_managed.sn': 'f7ccdf4d298a2aacf4eaaa9abe7cc619664be3a755ced392d0b9ded5c04870a3'}
PROBE_SHA256.update({'tests/rust-interfaces/named_callable_assignment.sn': '001981a3bab917147596b0badd817517b5d77a18818eda0a3fb146809990c210', 'tests/rust-interfaces/nullable_callable_transport.sn': 'fefd243946c76e78433f18a820d4cda490cc8b77da92f4c4df6297c99d7d6e62'})
PROBE_ORACLES['tests/rust-interfaces/effectful_callable_assignment.sn'] = b'final\n'
PROBE_SHA256['tests/rust-interfaces/effectful_callable_assignment.sn'] = '28c1afe45e62f6ab26ec2934b71b168248deb1215a9a6f818c52e625e8a0f5e9'


def verify_sources():
    for source, expected in (SOURCE_SHA256 | PROBE_SHA256 | HELPER_SHA256).items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected:
            raise ValueError(f'source changed: {source}')
    for source, expected in (ORACLES | PROBE_ORACLES).items():
        if Path(source).with_suffix('.expected').read_bytes() != expected:
            raise ValueError(f'stale fixture oracle: {source}')


def verify(path):
    verify_sources()
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in (*ORACLES, *PROBE_SHA256)
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError(f'expected {len(required)} successful structural interface cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        expected_hash = (SOURCE_SHA256 | PROBE_SHA256)[source]
        if (hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash
                or case['source_sha256'] != expected_hash):
            raise ValueError(f'source changed: {source}')
        expected = (ORACLES | PROBE_ORACLES).get(source)
        if expected is not None and Path(source).with_suffix('.expected').read_bytes() != expected:
            raise ValueError(f'stale fixture oracle: {source}')
        alternatives = {expected} if expected is not None else set()
        if source in EMPTY_LOCAL_COMPARISON:
            lines = expected.splitlines(keepends=True)
            assert lines[EMPTY_LOCAL_COMPARISON[source]] == b'true\n'
            lines[EMPTY_LOCAL_COMPARISON[source]] = b'false\n'
            alternatives.add(b''.join(lines))
        if os.name == 'nt' and expected is not None:
            expected = expected.replace(b'\n', b'\r\n')
            alternatives = {value.replace(b'\n', b'\r\n') for value in alternatives}
        if set(case['targets']) != {'c', 'rust'}:
            raise ValueError(f'missing target: {identity}')
        for target in ('c', 'rust'):
            result = case['targets'][target]
            if result['compile']['status'] != 0 or result['run']['status'] != 0:
                raise ValueError(f'unsuccessful execution: {identity} {target}')
            if result['run']['stderr_hex'] or (alternatives and
                    bytes.fromhex(result['run']['stdout_hex']) not in alternatives):
                raise ValueError(f'independent output mismatch: {identity} {target}')
        if (case['targets']['c']['run']['stdout_hex'] != case['targets']['rust']['run']['stdout_hex']):
            raise ValueError(f'C/Rust storage identity mismatch: {identity}')
    if seen != required:
        raise ValueError('incomplete optimization/arithmetic coverage')
    report['independent_oracle_cases'] = 9 * (len(ORACLES) + len(PROBE_ORACLES))
    report['storage_probe_cases'] = 9 * (len(PROBE_SHA256) - len(PROBE_ORACLES))
    for source, expected_hash in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'helper changed: {source}')
    report['helper_source_sha256'] = HELPER_SHA256
    report['oracle_scope'] = 'Structural interface arguments, source slot identity, aliases and copies, inline C offsets, native serialization handles, readonly aggregate transport, empty and record arrays, mutation, temporary/captured values, source closure snapshots, callable replacement and nullable callable transport, and metadata lifetime ownership. Foreign interface ABI remains a separate full-goal requirement.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(seen)} structural interface C/Rust cases, '
          f"{report['independent_oracle_cases']} independent output oracles")


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
