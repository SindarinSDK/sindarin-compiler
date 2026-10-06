#!/usr/bin/env python3
"""Check frozen C/Rust string closure parameter rebinding contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_closure_string_rebinding_default.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_asval.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_result.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_self.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_captures.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_expression.sn': b'true\ntrue\n', 'tests/rust-native/scalar_closure_string_rebinding_compound.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_closure_string_rebinding_default.sn': '41cc3eece8c83d764b04c9a8f94f675e228430336255936d5d81104983e0909e', 'tests/rust-native/scalar_closure_string_rebinding_asval.sn': '8149d99010574bb354cadd7da7d7e66ed8fc1b86f3fadcb26befb3497081bf90', 'tests/rust-native/scalar_closure_string_rebinding_result.sn': 'f92cbc61ae1553430b048768cb484ac427abd3d09d5b5a9fd861d7946a8964de', 'tests/rust-native/scalar_closure_string_rebinding_self.sn': '83c5035c9d795b44eb1ddde036d97c9898f98d1fbf45fd65fd5cf582d8cef68f', 'tests/rust-native/scalar_closure_string_rebinding_captures.sn': '449e87509b6ac4754ca93b672b8236b12898311d1a52430a15bbdf6ce4c83536', 'tests/rust-native/scalar_closure_string_rebinding_shadow.sn': '64da24a58d39cb0f021fd1437d8edb55de137d0d8ddb54ae3da2ddfb6154941d', 'tests/rust-native/scalar_closure_string_rebinding_expression.sn': 'e8505048ff0690e434da16867746bde3b35095d57da2b5db7ab41188b3eebb23', 'tests/rust-native/scalar_closure_string_rebinding_compound.sn': '7b3c0ac6a087ededf0bb47d6c3659bee71db9cc00fa2558a9ccece1f60d55a7a'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 72 successful string rebinding cases')
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
    report['oracle_scope'] = 'Default/as-val string formals with caller preservation, independent assignment results, self-assignment, snapshot captures before/after replacement, lexical shadowing, expression bodies and compound appends.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 72 independent string rebinding C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
