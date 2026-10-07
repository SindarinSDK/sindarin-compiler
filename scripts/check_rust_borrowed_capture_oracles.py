#!/usr/bin/env python3
"""Check frozen C/Rust lexical borrowed scalar capture contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_borrowed_capture_visible.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_nested.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_write_only.sn': b'true\n', 'tests/rust-native/scalar_borrowed_capture_bool.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_all_kinds.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_aliases.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_native.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_global.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_native_global.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_field.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_field_reassign.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_nested_field.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_native_field.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_nested_reassign.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_subrecord_reassign.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_thread.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_factory.sn': b'true\ntrue\n', 'tests/rust-native/scalar_borrowed_capture_factory_alias.sn': b'true\ntrue\n', 'tests/rgen/closure_values_borrowed_capture.sn': b'1\n', 'tests/rust-native/scalar_borrowed_capture_native_record_value.sn': b'true\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_borrowed_capture_visible.sn': 'ae0bbd40ff67103672a06ab5e6a567513659fb18ae818b53f098da894ec74049', 'tests/rust-native/scalar_borrowed_capture_nested.sn': '42789368dbadb3ddecb0680958a951ca85fd4239f2fbbb714373aea62dab7047', 'tests/rust-native/scalar_borrowed_capture_shadow.sn': 'bfcb2a1a11bdbecbfd4a60e7904591c5ff5c8b67b17682155c54af487e26e859', 'tests/rust-native/scalar_borrowed_capture_write_only.sn': '54a50a09df7808364c1a2244a4c8a1ed2db31570f8193c4dc8dc01319b6344bc', 'tests/rust-native/scalar_borrowed_capture_bool.sn': '04e4edd3af026843932eac4f6b40797e44a5cbc43a783c7a2b9b97e8fc852dc5', 'tests/rust-native/scalar_borrowed_capture_all_kinds.sn': '07ba64b5642243873dc281519157b645796b468920b045087aa8704e6371b316', 'tests/rust-native/scalar_borrowed_capture_aliases.sn': 'c986050a0e2663e3a39f5843ff5a7c87c29068a0cc09ee44f20d27663dcdfbda', 'tests/rust-native/scalar_borrowed_capture_native.sn': '1445c4ee6fb75db09a7c6148fa58ce4be6dfe640a275e2e91d1c00bdb584a6c8', 'tests/rust-native/scalar_borrowed_capture_global.sn': '7b215e2b89d2bd0b258816980cdf6b5d6d35b2e7245357af30df7137fad812ad', 'tests/rust-native/scalar_borrowed_capture_native_global.sn': 'bf5f4fcbc97ddbcafbb89748a1840ef5eaa7ba40dff34dce6bd9ded3dc1b73e5', 'tests/rust-native/scalar_borrowed_capture_field.sn': 'de416c35b456d147c1c70003ca3fdb28db0a6c83a7625ebc40ccbe5713bcf856', 'tests/rust-native/scalar_borrowed_capture_field_reassign.sn': 'cbd97488ceae9e32c71d91f25d6e980211381146d6c15f69982926eae6483a62', 'tests/rust-native/scalar_borrowed_capture_nested_field.sn': '738da05b5704bf35df2a1350ae0d5f163f17ff424569e42bbca44159004819bd', 'tests/rust-native/scalar_borrowed_capture_native_field.sn': 'a39614350fc2996ba165b93beb7f1660742227e6a1eb9285d2080ebbbeae282a', 'tests/rust-native/scalar_borrowed_capture_nested_reassign.sn': 'da168453c022f5a3d1ad8ac50ca48b57ae508e8285a986cd9ef42719065815a0', 'tests/rust-native/scalar_borrowed_capture_subrecord_reassign.sn': 'b3bb122500768924b0604c778a972e9d9db43ded5d2a02913e1cd93033b98470', 'tests/rust-native/scalar_borrowed_capture_thread.sn': 'ec4f2e645957557bfe8b69434c66287cb817b461ddb70d6287e7b7b5292342b7', 'tests/rust-native/scalar_borrowed_capture_factory.sn': 'c91f14fe064c8fe945488d631f7e9001d376bd4bcffe17c9c54011e1b17aaf51', 'tests/rust-native/scalar_borrowed_capture_factory_alias.sn': '54ef8c31f38e00dd5c1e381bd94c44ce8bab931d95998c764fd870acbbb692f6', 'tests/rgen/closure_values_borrowed_capture.sn': '1a61097e37327ab651d892ed835595e6d0d76f86f310a97cb3ddcc488dac821c', 'tests/rust-native/scalar_borrowed_capture_native_record_value.sn': '6e1fe25380425ab05d370293b88327765ecbd6727c7e5f55a4cf70e63a4c85d9'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 189 successful borrowed scalar capture cases')
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
    report['oracle_scope'] = 'Actual scalar caller owners, all scalar kinds, aliases, escaping/nested captures, fields and owner-preserving assignments, globals, native mutation, lambda factory copies and thread transport.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 189 independent borrowed scalar capture C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
