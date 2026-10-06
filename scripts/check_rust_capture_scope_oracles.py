#!/usr/bin/env python3
"""Check frozen C/Rust lexical capture scope contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/rust-native/scalar_capture_scope_block_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_heap_initializer_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_heap_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_initializer_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_late_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_loop_iterator.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_nested_initializer_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_nested_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_recursive.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_reference_shadow.sn': b'true\ntrue\n', 'tests/rust-native/scalar_capture_scope_while_shadow.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/rust-native/scalar_capture_scope_block_shadow.sn': '56c2978e84dcfdc19290f84ebb8984259f22d744a9fab8b1f7077234b293c917', 'tests/rust-native/scalar_capture_scope_heap_initializer_shadow.sn': '6ad2651a8c62c028579108b4c89ae7f8d7290d7a1c91c2d75e68b4dc478b4812', 'tests/rust-native/scalar_capture_scope_heap_shadow.sn': 'be0916ffb1afffeab9ca97f0154cb83348a3960002a9a741b4d12e14268af994', 'tests/rust-native/scalar_capture_scope_initializer_shadow.sn': '119fa36fc592b0d38d773855e213f1b9bf6a11fdaefbe536809ea3a59f1e88c3', 'tests/rust-native/scalar_capture_scope_late_shadow.sn': '2bfd5b1e4641c3bd3dbfa43ff692a28f18ac18be262adb109da1ed7f75b56dc1', 'tests/rust-native/scalar_capture_scope_loop_iterator.sn': '2f564496dc2c55788b76a9df310c286e5b16b0b8bc5f7eb7fa009ca1bac4c6fd', 'tests/rust-native/scalar_capture_scope_nested_initializer_shadow.sn': '8dfef859b4d9d7b697d95f9b7f903dd5ba11b710ad48aa13f6be45e74788aeec', 'tests/rust-native/scalar_capture_scope_nested_shadow.sn': 'a374ee657a6209680a433d4becc77ff890932769fb2dbf81379051b041485e6a', 'tests/rust-native/scalar_capture_scope_recursive.sn': 'bb8ae6a7668ff56078bff434ca1ea29e3fa988f5c5020b8d46ff7c31fe1a4608', 'tests/rust-native/scalar_capture_scope_reference_shadow.sn': '551220f12c625289f43cdeb759c25df3c226f8821641a07691ce056dae00cb58', 'tests/rust-native/scalar_capture_scope_while_shadow.sn': 'a5ede254a3f5f480ec4eb428cc6a84974c738967e96e3135907a073392ab2560'}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 99 successful capture scope cases')
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
    report['oracle_scope'] = 'Source-order capture discovery, block/branch/loop shadowing, late body declarations, heap cleanup, nested captures and recursive initializers.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 99 independent capture scope C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
