#!/usr/bin/env python3
"""Frozen global/thread ownership oracles and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

FIXTURES = {'tests/integration/test_module_array_swap_leak.sn': {'source_sha256': 'e9485d7a3fc801ae45b043c2c678e801e6c25cc7bd118d347dc393fc28bfdb91', 'oracle_sha256': '40bedfbd7b0a142c7a837d01b1754cd0283de62ef21e9ff617efb7c2cecccde3'}, 'tests/rust-native/native_handle_globals.sn': {'source_sha256': 'fa81b384ebdf06fe12a951cf6d479de96001ef2f3b19ed54f86e32d82d33e2c2', 'oracle_sha256': '282198c28ea912b090ceb27a2be8d3c67d8caf823c63d21085c7856c0a5ead28'}, 'tests/rust-native/native_handle_joined_results.sn': {'source_sha256': '2dd6c1c561a3e92d65a930a8d4c6eb4a7e65c4a7fda23f406ac3d3a91f8bfcba', 'oracle_sha256': '0fd13ada42874bdc56b4a34d432648a328af639aa34c572ca26691d5c9210a6b'}, 'tests/rust-native/global_array_thread_aliases.sn': {'source_sha256': '2088420b50a689261c37b3a90b9d3f68e289476006f41bb66395db3ff854fd61', 'oracle_sha256': 'd7797ad492745636c43eaa2c2f431a964cf835ffe732ebcf0015fbdf22dd4203'}}
NATIVE_HELPERS = {'tests/integration/test_module_array_swap_leak_helper.c': 'f40d5ae40e6821d0d8b646c34be591cc6525bf0cc31b85e9252073ea407fc52e', 'tests/rust-native/native_handle_globals.sn.c': '4d7c12753ff172e7203c511b4f2b7e52833a2494d8fdcf8fe780d74e89527acb', 'tests/rust-native/native_handle_joined_results.sn.c': '6046b0589f73200fc8bacf9712c9d308ab02f91ca4cb143401662ef1fba2f378'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in FIXTURES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 36 successful global/thread cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        specification = FIXTURES[source]
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != specification['source_sha256'] or case['source_sha256'] != specification['source_sha256']:
            raise ValueError(f'source changed: {source}')
        expected = Path(source).with_suffix('.expected').read_bytes()
        if hashlib.sha256(expected).hexdigest() != specification['oracle_sha256']:
            raise ValueError(f'oracle changed: {source}')
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
    for source, expected_hash in NATIVE_HELPERS.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'native helper changed: {source}')
    if seen != required:
        raise ValueError('incomplete optimization/arithmetic coverage')
    report['independent_oracle_cases'] = len(seen)
    report['native_source_sha256'] = NATIVE_HELPERS
    report['oracle_scope'] = (
        'Process-wide native globals and nested owner cleanup; duplicate borrowed C-array identity, '
        'C empty literal metadata, moved native owners with atomic C credits and Send only, '
        'default global-array transport with zero copies, alias mutation/reallocation visibility, '
        'explicit array copy callback counts and zero surviving resources; joined native reference '
        'result defaults, repeated/grouped joins and overwrite/scope cleanup; ordinary global arrays, '
        'lexical shadowing, replacement and threaded scalar reference aliases. Other C ownership '
        'failures, callbacks/SDK/qualifiers/native value families and broader thread lifetimes remain required.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 36 independent global/thread C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
