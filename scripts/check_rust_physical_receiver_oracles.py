#!/usr/bin/env python3
"""Check frozen C/Rust lexical physical receiver contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_physical_receiver_retired_slot.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_self_alias.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_operations.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_cloned_storage.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_construction.sn': b'true\ntrue\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_concat.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_sized.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_nested_method.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_sized_effects.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_recursive_method.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_argument_removal.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_more_operations.sn': b'true\ntrue\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_named_function.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_direct_call.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_named_index_effects.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_named_qualified.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_nil_concat_capacity.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_mixed_fields.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_temporary_owner.sn': b'true\ntrue\n', 'tests/rust-native/scalar_physical_receiver_foreach_mutation.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_foreach_rebind.sn': b'true\ntrue\ntrue\n', 'tests/rust-native/scalar_physical_receiver_foreach_growth.sn': b'true\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_physical_receiver_retired_slot.sn': 'dab060980659ad3ce002bb75fe75840ef18a374425c44ea27230b28165bf647c', 'tests/rust-native/scalar_physical_receiver_self_alias.sn': '1302bc4b7da87a7ef4e1762396b0eaa2ba461be4471571e4b97d20b118461021', 'tests/rust-native/scalar_physical_receiver_operations.sn': '34f06ecdb2133fea196c55e2dd2ab55451b570c4cc255586dedc738ae68e3e41', 'tests/rust-native/scalar_physical_receiver_cloned_storage.sn': '14db712e4e0445667ffc184f2a960c69e7db7b8afe990f8a19b772bad9bca615', 'tests/rust-native/scalar_physical_receiver_construction.sn': 'f38603ac02295034ffe3d8b3e5ecde1a45e6df893f12c9112decf9e01718ae57', 'tests/rust-native/scalar_physical_receiver_concat.sn': '6b84a4ee3eb9509e807d42201490c355d633086ea1823f946285d16eabdfbb50', 'tests/rust-native/scalar_physical_receiver_sized.sn': '12ccabf1cc5e9779e94fc1d7f2fd9025a9591a4df8a4b524a8e4415400856f34', 'tests/rust-native/scalar_physical_receiver_nested_method.sn': '3dd80a2a1a3a59d51b26921147b097c53d956ea54e21ec39a35e0b062898fc84', 'tests/rust-native/scalar_physical_receiver_sized_effects.sn': '31f985374de7b3f57db06537cc7d40280fd6d68e02ef00efdc68368711b12796', 'tests/rust-native/scalar_physical_receiver_recursive_method.sn': '6a6f4805bdb17a836a72996cbc5150f91443239d0a24880c06623adeb768415a', 'tests/rust-native/scalar_physical_receiver_argument_removal.sn': 'af71797a267db1f2831123b47063f55a8421bc1728261905136eb55687557ea8', 'tests/rust-native/scalar_physical_receiver_more_operations.sn': 'b9e3279e076eb7f6667bd68e1b640926d6fb50566a50dfe764d816aab527b58f', 'tests/rust-native/scalar_physical_receiver_named_function.sn': '85eab205d23890dc9165a826a6266756df2b2f4119fa972ee5ed8412ea0a9673', 'tests/rust-native/scalar_physical_receiver_direct_call.sn': '8ee23ae3e663f29c149a6748e57bcc15f2a22875eff608695ebfc97d92008082', 'tests/rust-native/scalar_physical_receiver_named_index_effects.sn': '92570b30cedc4a01456cbed78378dbc7e914844448fdb09ceac6452ce2cf046d', 'tests/rust-native/scalar_physical_receiver_named_qualified.sn': 'e73ca0a63812ff275dca2c7120aeba9b4c5aa9971994a5dd57d74cbb94d82f7f', 'tests/rust-native/scalar_physical_receiver_nil_concat_capacity.sn': '6d00d0e4bba5661076c74ff1ff9b2fca3c2ad20112ac06fe93f8b1e96d961634', 'tests/rust-native/scalar_physical_receiver_mixed_fields.sn': 'dcf3e56b48004e7733db04cd5ce5a74dd662ef7a7e6e5820b5197c903515198d', 'tests/rust-native/scalar_physical_receiver_temporary_owner.sn': 'ae4f76cd45b7fe7402205aa0575826dcef3670f1a181d9d64b22ac6983bbf88c', 'tests/rust-native/scalar_physical_receiver_foreach_mutation.sn': '0d673d948eac6d344139f335569eec83c8e938405ef048e26005f5101f6d28f3', 'tests/rust-native/scalar_physical_receiver_foreach_rebind.sn': 'ad18bfb779429e41ce560cbffc4ab00e5e87ceecf8b59fef824f780f55765ed7', 'tests/rust-native/scalar_physical_receiver_foreach_growth.sn': '1fba9517a4f47099c4609c527e4160faaba1a18aa4bf70d3a6fba088ef23441f'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 198 successful physical receiver cases')
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
    report['oracle_scope'] = 'Physical value-record slots, retained array capacity and initialized removed slots, argument removal, nil/concat/slices, sized defaults and nested/recursive methods.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 198 independent physical receiver C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
