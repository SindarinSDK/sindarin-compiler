#!/usr/bin/env python3
"""Verify comparison results and stored values independently of either backend."""
import hashlib
import json
import os
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
RAW = 'tests/rust-native/scalar_floating_compound_comparisons.sn'
STRICT = 'tests/rust-native/scalar_floating_compound_strict_comparisons.sn'


def verify(report, windows=None):
    windows = os.name == 'nt' if windows is None else windows
    if not report['passed']:
        raise ValueError('compound comparison differential gate failed')
    seen = set()
    for case in report['cases']:
        # Path's string representation in Windows reports uses backslashes.
        # Normalize fixture identity only; output and source bytes stay exact.
        source = case['source'].replace('\\', '/')
        if source not in (RAW, STRICT):
            continue
        identity = (source, case['arithmetic_mode'], case['optimization'])
        if identity in seen:
            raise ValueError(f'duplicate compound comparison case: {identity}')
        seen.add(identity)
        if case['source_sha256'] != hashlib.sha256((ROOT / source).read_bytes()).hexdigest():
            raise ValueError(f'compound comparison source changed: {source}')
        if source == RAW:
            wanted = (ROOT / source).with_suffix('.expected').read_text()
        else:
            # Default O2 selects unchecked arithmetic. Explicit --checked keeps
            # C's strict-comparison helper boundary at every optimization level.
            raw = case['arithmetic_mode'] == 'unchecked' or (
                case['arithmetic_mode'] == 'default' and case['optimization'] == '-O2')
            wanted = ('true\n1\ntrue\n1\ntrue\n1.00000\n' if raw else
                      'true\n3\nfalse\n3\ntrue\n3.50000\n')
        if windows:
            wanted = wanted.replace('\n', '\r\n')
        for target in ('c', 'rust'):
            result = case['targets'][target]
            run = result['run']
            if (result['compile']['status'] != 0 or run['status'] != 0 or
                    bytes.fromhex(run['stdout_hex']) != wanted.encode() or run['stderr_hex']):
                raise ValueError(f'compound comparison oracle failed: {identity}, {target}')
    required = {(source, mode, optimization) for source in (RAW, STRICT)
                for mode in ('default', 'checked', 'unchecked')
                for optimization in ('-O0', '-O1', '-O2')}
    if seen != required:
        raise ValueError('incomplete compound comparison mode/optimization coverage')
    print('18 independent compound comparison oracles passed')


if __name__ == '__main__':
    verify(json.loads(Path(sys.argv[1]).read_text()))
