#!/usr/bin/env python3
"""Check scalar value-parameter stores after calls borrowing that local value."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]
SCALARS = [('int', '1', '3', '4', '5'), ('long', '1', '3', '4', '5'),
           ('int32', '1', '3', '4', '5'), ('uint', '1', '3', '4', '5'),
           ('uint32', '1', '3', '4', '5'), ('byte', '1', '3', '4', '5'),
           ('float', '1.0f', '3.0f', '4.0f', '5.0f'),
           ('double', '1.0', '3.0', '4.0', '5.0'),
           ('bool', 'false', 'true', 'false', 'true'),
           ('char', "'a'", "'B'", "'C'", "'z'")]
CHARACTERS = '''fn advance(value: char as ref): char =>
  return value++

fn restore(value: char): bool =>
  var before: char = value
  var result: char = value = advance(value)
  return result == before && value == before

fn main(): void =>
  var values: char[] = { BYTE_VALUES }
  var passed: int = 0
  for var index: int = 0; index < values.length; index++ =>
    var original: char = values[index]
    if restore(original) && original == values[index] =>
      passed++
  println(passed)
'''.replace('BYTE_VALUES', ', '.join("'\\x%02x'" % value for value in range(256)))


def control_source():
    functions = []
    main = ['fn main(): void =>', '  var calls: int = 0']
    for kind, initial, first, second, result in SCALARS:
        functions.append(f'''fn pair_{kind}(left: {kind} as ref, right: {kind} as ref, calls: int as ref): {kind} =>
  left = {first}
  println(right == {first})
  right = {second}
  println(left == {second})
  calls += 1
  return {result}

fn relay_{kind}(left: {kind} as ref, right: {kind} as ref, calls: int as ref): {kind} =>
  return pair_{kind}(left, right, calls)

fn assign_{kind}(value: {kind}, calls: int as ref, viaRelay: bool): {kind} =>
  if viaRelay =>
    var result: {kind} = value = relay_{kind}(value, value, calls)
    println(result == {result})
  else =>
    var result: {kind} = value = pair_{kind}(value, value, calls)
    println(result == {result})
  println(value == {result})
  return value
''')
        main.append(f'  var original_{kind}: {kind} = {initial}')
        for via_relay in ('false', 'true'):
            main += [f'  println(assign_{kind}(original_{kind}, calls, {via_relay}) == {result})',
                     f'  println(original_{kind} == {initial})']
    functions.append('''fn widen(value: float as ref): double =>
  value = 3.0f
  return 4.5

fn narrow(value: double as ref): float =>
  value = 3.0
  return 4.5f

fn assignFloatConversion(value: float): float =>
  return value = widen(value)

fn assignDoubleConversion(value: double): double =>
  return value = narrow(value)

fn increment(value: int as ref): int =>
  value += 1
  return value

struct Observer =>
  stamp: int
  fn change(value: int): int =>
    return value = increment(value)

fn crossUpdate(local: int as ref, external: int as ref): int =>
  local = 7
  external = 9
  return 11

fn cross(value: int, external: int as ref): int =>
  return value = crossUpdate(value, external)
''')
    main += ['  println(assignFloatConversion(original_float) == 4.5f)',
             '  println(original_float == 1.0f)',
             '  println(assignDoubleConversion(original_double) == 4.5)',
             '  println(original_double == 1.0)',
             '  var observer: Observer = {stamp: 7}', '  var input: int = 1',
             '  println(observer.change(input) == 2)', '  println(input == 1)',
             '  println(observer.stamp == 7)', '  println(calls)',
             '  var external: int = 1', '  println(cross(external, external))',
             '  println(external)']
    return '\n'.join(functions) + '\n' + '\n'.join(main) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    original = ROOT / 'tests/rgen/errors/by_value_parameter_direct_assignment_ref_forward.sn'
    marker = json.loads(original.with_suffix('.parity.json').read_text())
    digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if (digest(original) != marker['source_sha256'] or
            digest(original.with_suffix('.expected')) != marker['legacy_oracle_sha256'] or
            marker['stdout_hex'] != b'2\n'.hex() or marker['exit'] != 0):
        raise ValueError('frozen parameter source or original diagnostic changed')
    if args.sanitize:
        os.environ.update(SN_CFLAGS=os.environ.get('SN_CFLAGS', '') +
                          ' -fsanitize=address,undefined -fno-omit-frame-pointer',
                          ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                          UBSAN_OPTIONS='halt_on_error=1')
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    control = control_source()
    with tempfile.TemporaryDirectory(prefix='sn-scalar-parameter-rhs-') as folder:
        temporary = Path(folder) / 'scalar-effects.sn'
        temporary.write_text(control)
        characters = Path(folder) / 'characters.sn'
        characters.write_text(CHARACTERS)
        fixtures = [(original, 'tests/rgen/errors/by_value_parameter_direct_assignment_ref_forward.sn', b'2\n'),
                    (temporary, 'control:ten-scalar-types', b'true\n' * 127 + b'20\n11\n9\n'),
                    (characters, 'control:character-byte-domain', b'256\n')]
        jobs = [(source, label, expected, mode, opt) for source, label, expected in fixtures
                for mode in ('default', 'checked', 'unchecked') for opt in ('-O0', '-O1', '-O2')]
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
              'control_source': control, 'character_control_source': CHARACTERS,
              'historical_sources_preserved': True,
              'sanitizers': args.sanitize, 'environment': {name: os.environ.get(name) for name in
                  ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')},
              'cases': cases, 'independent_oracle_cases': len(cases),
              'passed': len(cases) == 27 and all(case['passed'] for case in cases)}
    output = ROOT / '.sn' / ('rust-parity-scalar-parameter-rhs-sanitizers.json' if args.sanitize
                             else 'rust-parity-scalar-parameter-rhs.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if not report['passed']:raise SystemExit(1)
    print('PASS: 27 independent scalar parameter RHS cases')


if __name__ == '__main__':
    main()
