#!/usr/bin/env python3
"""Check byte-domain iterator postfix values, local storage and nested scopes."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = '''struct ByteIterator =>
  values: char[]
  position: int
  fn iter(): ByteIterator =>
    return self
  fn hasNext(): bool =>
    return self.position < self.values.length
  fn next(): char =>
    var result: char = self.values[self.position]
    self.position += 1
    return result

fn advance(value: char as ref): char =>
  return value++

fn main(): void =>
  var values: char[] = { BYTE_VALUES }
  var source: ByteIterator = {values: values, position: 0}
  var index: int = 0
  var original: int = 0
  var advanced: int = 0
  var postfix: int = 0
  var restored: int = 0
  var forwarded: int = 0
  for value in source =>
    var old: char = value++
    if old == values[index] =>
      original++
    if value == values[(index + 1) % values.length] =>
      advanced++
    var previous: char = value--
    if previous == values[(index + 1) % values.length] =>
      postfix++
    if value == values[index] =>
      restored++
    var refOld: char = advance(value)
    if refOld == values[index] && value == values[(index + 1) % values.length] =>
      forwarded++
    index++
  println($"{index} {original} {advanced} {postfix} {restored} {forwarded}")
  println(source.position)
'''.replace('BYTE_VALUES', ', '.join("'\\x%02x'" % value for value in range(256)))
SCOPES = '''struct Repeated =>
  count: int
  token: char
  fn iter(): Repeated =>
    return self
  fn hasNext(): bool =>
    return self.count > 0
  fn next(): char =>
    self.count -= 1
    return self.token

fn choose(source: Repeated): char =>
  for item in source =>
    item++
    return item
  return '!'

fn main(): void =>
  var value: char = 'z'
  var outer: Repeated = {count: 3, token: 'a'}
  var inner: Repeated = {count: 2, token: 'x'}
  var seen: int = 0
  var total: int = 0
  for value in outer =>
    var before: char = value++
    if before == 'a' && value == 'b' =>
      seen++
    for value in inner =>
      var old: char = value--
      if old == 'x' && value == 'w' =>
        total++
    if value == 'b' =>
      seen++
  println($"{value} {seen} {total}")
  println($"{choose(outer)} {outer.count} {inner.count}")
  var visited: int = 0
  for item in outer =>
    item++
    if visited == 0 =>
      visited++
      continue
    item--
    if item == 'a' =>
      visited++
    break
  println(visited)
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    original = ROOT / 'tests/rgen/errors/iterator_protocol_char_postfix.sn'
    marker = json.loads(original.with_suffix('.parity.json').read_text())
    digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if (digest(original) != marker['source_sha256'] or
            digest(original.with_suffix('.expected')) != marker['legacy_oracle_sha256'] or
            marker['stdout_hex'] != '' or marker['exit'] != 0):
        raise ValueError('frozen iterator source or original diagnostic changed')
    if args.sanitize:
        os.environ.update(SN_CFLAGS=os.environ.get('SN_CFLAGS', '') +
                          ' -fsanitize=address,undefined -fno-omit-frame-pointer',
                          ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                          UBSAN_OPTIONS='halt_on_error=1')
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    with tempfile.TemporaryDirectory(prefix='sn-character-iterator-') as folder:
        domain, scopes = Path(folder) / 'domain.sn', Path(folder) / 'scopes.sn'
        domain.write_text(DOMAIN)
        scopes.write_text(SCOPES)
        fixtures = [(original, 'tests/rgen/errors/iterator_protocol_char_postfix.sn', b''),
                    (ROOT / 'tests/rgen/character_parameter_byte_domain.sn',
                     'tests/rgen/character_parameter_byte_domain.sn', b'256 256 256 256\n'),
                    (domain, 'control:byte-domain', b'256 256 256 256 256 256\n0\n'),
                    (scopes, 'control:nested-scopes', b'z 6 6\nb 3 2\n2\n')]
        jobs = [(source, label, expected, mode, opt)
                for source, label, expected in fixtures
                for mode in ('default', 'checked', 'unchecked')
                for opt in ('-O0', '-O1', '-O2')]
        settings = argparse.Namespace(c_ldlibs=None, compile_timeout=120, run_timeout=30)

        def verify(job):
            index, (source, label, expected, mode, opt) = job
            case = compare_case(compiler, source, index, mode, opt, folder, settings)
            case['source'] = label
            if os.name == 'nt':expected = expected.replace(b'\n', b'\r\n')
            case['expected_stdout_hex'] = expected.hex()
            case['passed'] = case['passed'] and all(
                target['compile']['status'] == 0 and target['run']['status'] == 0 and
                target['run']['stdout_hex'] == expected.hex() and target['run']['stderr_hex'] == ''
                for target in case['targets'].values())
            print(('PASS ' if case['passed'] else 'FAIL ') + label + ' ' + mode + ' ' + opt, flush=True)
            return case

        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS', '2'))) as workers:
            cases = list(workers.map(verify, enumerate(jobs)))
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'control_sources': {'byte-domain': DOMAIN, 'nested-scopes': SCOPES},
              'historical_sources_preserved': True, 'sanitizers': args.sanitize,
              'environment': {name: os.environ.get(name) for name in
                              ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS',
                               'ASAN_OPTIONS', 'UBSAN_OPTIONS')},
              'cases': cases, 'independent_oracle_cases': len(cases),
              'passed': len(cases) == 36 and all(case['passed'] for case in cases)}
    output = ROOT / '.sn' / ('rust-parity-character-iterator-sanitizers.json' if args.sanitize
                             else 'rust-parity-character-iterator.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if not report['passed']:raise SystemExit(1)
    print('PASS: 36 independent byte-domain iterator cases')


if __name__ == '__main__':
    main()
