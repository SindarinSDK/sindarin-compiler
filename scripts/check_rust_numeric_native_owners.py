#!/usr/bin/env python3
"""Preserve numeric closure evaluation with canonical native array storage.

This checks observable calls, alongside the existing numeric-place gate.
It does not provide sanitizer ownership acceptance for legacy numeric arrays.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '''native struct Field as ref =>
  data: byte[]
native fn field(): Field =>
  println("field")
  return Field {data:{7b}}
fn fresh(): int[] =>
  println("owner")
  return {7}
fn main(): void =>
  var callback: fn(): int = fn(): int =>
    var values: byte[] = field().data
    return fresh()[-1]++
  println("created")
  println(callback())
  println(callback())
'''
EXPECTED = b'owner\ncreated\nfield\nowner\nowner\n7\nfield\nowner\nowner\n7\n'


def main():
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    output = ROOT / '.sn' / 'rust-parity-numeric-native-owners.json'
    output.parent.mkdir(exist_ok=True)
    report = {'passed': False, 'source': SOURCE, 'cases': [],
              'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'scope': 'Evaluation order; no legacy C temporary-array sanitizer ownership credit',
              'environment': {k: os.environ.get(k) for k in ('SN_CC', 'SN_CFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    expected = EXPECTED.replace(b'\n', b'\r\n') if os.name == 'nt' else EXPECTED
    settings = argparse.Namespace(c_ldlibs=None, compile_timeout=180, run_timeout=30)
    with tempfile.TemporaryDirectory(prefix='sn-numeric-native-owners-') as tmp:
        source = Path(tmp) / 'main.sn'
        source.write_text(SOURCE)
        for opt in ('-O0', '-O1', '-O2'):
            for mode in ('default', 'checked', 'unchecked'):
                case = compare_case(compiler, source, len(report['cases']), mode, opt, tmp, settings)
                case['source'] = 'control:numeric-native-closure-owners'
                case['expected_stdout_hex'] = expected.hex()
                case['passed'] = case['passed'] and all(
                    (r.get('run', {}).get('stdout_hex'), r.get('run', {}).get('stderr_hex')) == (expected.hex(), '')
                    for r in case['targets'].values())
                report['cases'].append(case)
                output.write_text(json.dumps(report, indent=2) + '\n')
                print('PASS' if case['passed'] else 'FAIL', opt, mode, flush=True)
    report['passed'] = all(c['passed'] for c in report['cases'])
    output.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
