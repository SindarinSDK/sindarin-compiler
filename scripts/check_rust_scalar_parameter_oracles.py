#!/usr/bin/env python3
"""Check complete mode coverage and independent scalar-parameter output oracles."""
import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_byte_arr_insert.sn': b'1 2 3\n',
    'tests/integration/test_as_ref_params.sn': b"=== Parameter-level 'as ref' Tests ===\n\nTest 1: Basic as ref parameter\n  PASS\nTest 2: Multiple as ref parameters\n  PASS\nTest 3: Different primitive types\n  PASS\nTest 4: Threaded as ref modification\n  PASS\nTest 5: Modification after sync\n  PASS\nTest 6: Mixed as ref and regular params\n  PASS\nTest 7: Byte and char as ref\n  PASS\nTest 8: Nested as ref calls\n  PASS\n\n=== All 'as ref' parameter tests passed! ===\n",
    'tests/rgen/char_as_ref_parameter.sn': b'',
    'tests/rgen/by_value_parameter_direct_assignment_char.sn': b'z\n',
    'tests/rgen/by_value_parameter_char_postfix_mutation.sn': b'a\n',
    'tests/rgen/character_parameter_byte_domain.sn': b'256 256 256 256\n',
    'tests/rgen/character_reference_places.sn': b'Z A B A B B C 1\n',
    'tests/rgen/character_thread_references.sn': b'A B\nZ Z\n',
    'tests/rgen/character_reference_aliases.sn': b'X\nY\nY\nB\nX\nX C\n',
    'tests/rgen/byte_reference_assignment_values.sn': b'0 1 1\n',
    'tests/rgen/scalar_reference_aliases.sn': b'true\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\n',
    'tests/rgen/integer_reference_storage_boundaries.sn': b'0 255 255 255 255\n',
    'tests/rgen/array_search_methods.sn': b'true\nfalse\n0\n-1\ntrue\n0\n-1\nbeta\ntrue\n0\ntrue\n2\ntrue\nfalse\n0\n-1\nfalse\n-1\n',
    'tests/rgen/capture_array_index_widths.sn': b'0.50000\n3\ntrue\nz\n5\n1.00000\n5\nfalse\nz\n5\n0.00000\n1\nfalse\na\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError(f'expected {len(required)} successful scalar-parameter cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != case['source_sha256']:
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        sidecar = Path(source).with_suffix('.expected')
        if sidecar.read_bytes() != expected:
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
    source = Path('tests/rgen/character_parameter_byte_domain.sn')
    compiler = Path(report['compiler'])
    if not compiler.is_file():
        raise ValueError('compiler required for formatter round-trip evidence')
    with tempfile.TemporaryDirectory(prefix='sn-character-formatter-') as directory:
        copy = Path(directory) / source.name
        original = source.read_bytes()
        copy.write_bytes(original)
        for arguments in (['--format', '--no-install'], ['--format', '--check', '--no-install']):
            result = subprocess.run([str(compiler.resolve()), *arguments], cwd=directory,
                                    capture_output=True, timeout=30)
            if result.returncode or copy.read_bytes() != original:
                raise ValueError('formatter changed the 256 byte-domain character literals')
    report['formatter_round_trip_source_sha256'] = hashlib.sha256(original).hexdigest()
    report['independent_oracle_cases'] = len(seen)
    report['oracle_scope'] = (
        'Character reference/value parameter assignment and postfix operations, all 256 byte '
        'identities, thread transport and mutation, scalar aliases through forwarding calls '
        'for ten scalar types, byte assignment narrowing and expression results, large '
        'integer literals, source-preserved array insertion, and two runtime/snapshot controls. '
        'C frontend-rejected direct scalar field/index reference arguments receive no credit.'
    )
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(seen)} independent scalar-parameter C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
