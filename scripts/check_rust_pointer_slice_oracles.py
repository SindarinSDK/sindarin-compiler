#!/usr/bin/env python3
"""Verify frozen byte-pointer slice outputs, sources, helpers and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {'tests/integration/test_buffer_unwrap.sn': b'Testing buffer unwrap code generation...\nArray creat'
                                            b'ed successfully with length parameter: 5\nArray lengt'
                                            b'h property: 5\nPattern 1 (ptr[0..len] as val): OK'
                                            b'\nPattern 3 (fn()[0..fn2()] as val): OK\nBuffer un'
                                            b'wrap code generation test complete!\n',
 'tests/integration/test_nil_pointer_slice.sn': b'Testing nil pointer slicing...\nArray length:'
                                                b' 5\nLength check: PASS\nNil pointer slice test'
                                                b' complete!\n',
 'tests/exploratory/test_pointer_slice_bounds.sn': b'Testing pointer slice bounds behavior...\nNot'
                                                   b'e: Out-of-bounds slicing is undefined behavi'
                                                   b'or!\n\nCorrect bounds: array length = 5\nBo'
                                                   b'unds test: PASS\n\nPointer slice bounds test c'
                                                   b'omplete!\n',
 'tests/rust-native/native_pointer_slices.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n'
                                               b'true\ntrue\n',
 'tests/rust-native/native_pointer_slice_contexts.sn': b'true\ntrue\ntrue\ntrue\ntrue\ntrue\n'}
SOURCE_SHA256 = {'tests/integration/test_buffer_unwrap.sn': '0fb1fa07a23902908a0059fad7833be238ef80f49fc57dd2c18b23fda7d7b4ae',
 'tests/integration/test_nil_pointer_slice.sn': '7e0d8eab059b09774768bc14cb3d2d2894fbccc576968aa1a10b1bab77799cfc',
 'tests/exploratory/test_pointer_slice_bounds.sn': '138ea31a5de3bfa764d7eefb102f26d73ad4c502c1ca5d1a33748f9c92ff88a8',
 'tests/rust-native/native_pointer_slices.sn': '9e41250619b2526e78a7dd25e56586142c2ef7f70d9cb6880a804a3b11580687',
 'tests/rust-native/native_pointer_slice_contexts.sn': '34ec507632437d006bd8fa72802fce549c2cc30c1d13a1cb9125c42ea427eb1f'}
HELPER_SHA256 = {'tests/rust-native/native_pointer_slices.sn.c': 'ad759453261c748080fcb3a5f252b12f18da3ad6a6915e2fc8cd8c41a3359c80'}

def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 45 successful pointer slice cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source] or case['source_sha256'] != SOURCE_SHA256[source]:
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        oracle_path = Path(source).with_suffix('.expected')
        if oracle_path.read_bytes() != expected:
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
    for source, expected_hash in HELPER_SHA256.items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'helper source changed: {source}')
    report['helper_source_sha256'] = HELPER_SHA256
    report['oracle_scope'] = 'Owned byte-pointer slices and no-op valueOf: nil zero-fill, empty/reversed ranges, omitted start, negative offsets within a valid source allocation, unsigned bytes/NUL, source mutation and disposal, numeric bounds with actual C char promotion, globals, record fields, ordinary returns and joined threads, private helper names and once-only evaluation. C leaves independent argument order unspecified; no portable C-order claim is made. Other scalar/managed pointer-array storage remains full-goal work.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 45 independent byte-pointer slice C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
