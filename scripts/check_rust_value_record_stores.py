#!/usr/bin/env python3
"""Check value record parameter field/slot stores, copies and expression values."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]

SCALARS = '''struct Cell_int =>
  field: int

fn field_int(value: Cell_int): int =>
  value.field = 7
  return value.field

fn whole_int(value: Cell_int): Cell_int =>
  return value = Cell_int {field: 7}

struct Cell_long =>
  field: long

fn field_long(value: Cell_long): long =>
  value.field = 7
  return value.field

fn whole_long(value: Cell_long): Cell_long =>
  return value = Cell_long {field: 7}

struct Cell_int32 =>
  field: int32

fn field_int32(value: Cell_int32): int32 =>
  value.field = 7
  return value.field

fn whole_int32(value: Cell_int32): Cell_int32 =>
  return value = Cell_int32 {field: 7}

struct Cell_uint =>
  field: uint

fn field_uint(value: Cell_uint): uint =>
  value.field = 7
  return value.field

fn whole_uint(value: Cell_uint): Cell_uint =>
  return value = Cell_uint {field: 7}

struct Cell_uint32 =>
  field: uint32

fn field_uint32(value: Cell_uint32): uint32 =>
  value.field = 7
  return value.field

fn whole_uint32(value: Cell_uint32): Cell_uint32 =>
  return value = Cell_uint32 {field: 7}

struct Cell_byte =>
  field: byte

fn field_byte(value: Cell_byte): byte =>
  value.field = 7
  return value.field

fn whole_byte(value: Cell_byte): Cell_byte =>
  return value = Cell_byte {field: 7}

struct Cell_float =>
  field: float

fn field_float(value: Cell_float): float =>
  value.field = 7.0f
  return value.field

fn whole_float(value: Cell_float): Cell_float =>
  return value = Cell_float {field: 7.0f}

struct Cell_double =>
  field: double

fn field_double(value: Cell_double): double =>
  value.field = 7.0
  return value.field

fn whole_double(value: Cell_double): Cell_double =>
  return value = Cell_double {field: 7.0}

struct Cell_bool =>
  field: bool

fn field_bool(value: Cell_bool): bool =>
  value.field = true
  return value.field

fn whole_bool(value: Cell_bool): Cell_bool =>
  return value = Cell_bool {field: true}

struct Cell_char =>
  field: char

fn field_char(value: Cell_char): char =>
  value.field = 'z'
  return value.field

fn whole_char(value: Cell_char): Cell_char =>
  return value = Cell_char {field: 'z'}

fn main(): void =>
  var input_int: Cell_int = {field: 1}
  println(field_int(input_int) == 7)
  println(input_int.field == 1)
  println(whole_int(input_int).field == 7)
  println(input_int.field == 1)
  var input_long: Cell_long = {field: 1}
  println(field_long(input_long) == 7)
  println(input_long.field == 1)
  println(whole_long(input_long).field == 7)
  println(input_long.field == 1)
  var input_int32: Cell_int32 = {field: 1}
  println(field_int32(input_int32) == 7)
  println(input_int32.field == 1)
  println(whole_int32(input_int32).field == 7)
  println(input_int32.field == 1)
  var input_uint: Cell_uint = {field: 1}
  println(field_uint(input_uint) == 7)
  println(input_uint.field == 1)
  println(whole_uint(input_uint).field == 7)
  println(input_uint.field == 1)
  var input_uint32: Cell_uint32 = {field: 1}
  println(field_uint32(input_uint32) == 7)
  println(input_uint32.field == 1)
  println(whole_uint32(input_uint32).field == 7)
  println(input_uint32.field == 1)
  var input_byte: Cell_byte = {field: 1}
  println(field_byte(input_byte) == 7)
  println(input_byte.field == 1)
  println(whole_byte(input_byte).field == 7)
  println(input_byte.field == 1)
  var input_float: Cell_float = {field: 1.0f}
  println(field_float(input_float) == 7.0f)
  println(input_float.field == 1.0f)
  println(whole_float(input_float).field == 7.0f)
  println(input_float.field == 1.0f)
  var input_double: Cell_double = {field: 1.0}
  println(field_double(input_double) == 7.0)
  println(input_double.field == 1.0)
  println(whole_double(input_double).field == 7.0)
  println(input_double.field == 1.0)
  var input_bool: Cell_bool = {field: false}
  println(field_bool(input_bool) == true)
  println(input_bool.field == false)
  println(whole_bool(input_bool).field == true)
  println(input_bool.field == false)
  var input_char: Cell_char = {field: 'a'}
  println(field_char(input_char) == 'z')
  println(input_char.field == 'a')
  println(whole_char(input_char).field == 'z')
  println(input_char.field == 'a')
'''
COMPOSITION = '''struct Point =>
  x: int
  y: int
struct Box =>
  point: Point
  stamp: int
fn write(value: Point): int =>
  var before: Point = value
  value.x = 2
  value.y = 3
  println(before.x)
  var assigned: int = value.x = 4
  println(assigned)
  return value.x + value.y
fn nested(value: Box): Box =>
  value.point.x = 9
  var result: Point = value.point = Point {x: 5, y: 6}
  println(result.x)
  println(value.point.y)
  return value
fn reference(value: Point as ref): int =>
  value.x = 12
  return 3
fn forward(value: Point): int =>
  value.y = reference(value)
  return value.x + value.y
fn replace(value: Point): int =>
  var result: Point = value = Point {x: 7, y: 8}
  println(result.y)
  println(value.x)
  value = result
  return value.y
fn recurse(value: Point, depth: int): int =>
  if depth > 0 =>
    value.x = depth
    return recurse(value, depth - 1)
  value.x = 20
  return value.x
struct Operations =>
  amount: int
  static fn overwrite(value: Point): Point =>
    return value = Point {x: 2, y: 3}
  fn change(value: Point): int =>
    value.x = self.amount
    return value.x
fn scoped(value: Point): int =>
  if true =>
    var value: Point = {x: 30, y: 40}
    value.x = 31
    println(value.x)
  return value.x
fn localStore(): int =>
  var point: Point = {x: 1, y: 2}
  return point.x = 42
fn hygiene(value: Point, __sn_place_value_0: int): int =>
  var __sn_place_value_1: int = __sn_place_value_0
  return value.x = __sn_place_value_1
fn next(calls: int as ref): int =>
  calls++
  return 0
fn replacement(value: Point as ref): Point =>
  value.x = 4
  return Point {x: 9, y: value.x}
fn afterReference(value: Point): Point =>
  return value = replacement(value)
fn explicit(value: Point as val): int =>
  value.x = 88
  return value.x
fn ignored(value: Point): void =>
  value.x = 90
fn main(): void =>
  var original: Point = {x: 1, y: 2}
  println(write(original))
  println(original.x)
  var box: Box = {point: original, stamp: 10}
  var result: Box = nested(box)
  println(result.point.x)
  println(result.stamp)
  println(box.point.x)
  println(forward(original))
  println(original.x)
  println(replace(original))
  println(original.x)
  println(recurse(original, 3))
  println(original.x)
  println(Operations.overwrite(original).y)
  var ops: Operations = {amount: 10}
  println(ops.change(original))
  println(ops.amount)
  println(original.x)
  println(scoped(original))
  println(localStore())
  println(hygiene(original, 66))
  var items: Point[] = {Point {x: 1, y: 2}}
  var calls: int = 0
  println(write(items[next(calls)]))
  println(calls)
  println(items[0].x)
  var callback = write
  println(callback(original))
  println(original.x)
  println(afterReference(original).y)
  println(original.x)
  println(explicit(original))
  println(original.x)
  ignored(original)
  println(original.x)
'''

COMPOSITION_EXPECTED = b'1\n4\n7\n1\n5\n6\n5\n10\n1\n15\n1\n8\n7\n8\n1\n20\n1\n3\n10\n10\n1\n31\n1\n42\n66\n1\n4\n7\n1\n1\n1\n4\n7\n1\n' + b'4\n1\n88\n1\n1\n'
WORKER = 'fn worker(value: Point): int =>\n  value.x = 77\n  return value.x\n'
JOIN = '  var pending: int = & worker(original)\n  println(pending!)\n  println(original.x)\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    fixtures = []
    digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    for name in ('by_value_parameter_direct_assignment_field', 'by_value_parameter_direct_assignment_struct'):
        source = ROOT/'tests/rgen/errors'/(name+'.sn')
        marker = json.loads(source.with_suffix('.parity.json').read_text())
        if (digest(source) != marker['source_sha256'] or
                digest(source.with_suffix('.expected')) != marker['legacy_oracle_sha256'] or
                marker['stdout_hex'] != b'2\n'.hex() or marker['exit'] != 0):
            raise ValueError('frozen record parameter source or original diagnostic changed')
        fixtures.append((source, source.relative_to(ROOT).as_posix(), b'2\n'))
    if args.sanitize:
        os.environ.update(SN_CFLAGS=os.environ.get('SN_CFLAGS', '') +
                          ' -fsanitize=address,undefined -fno-omit-frame-pointer',
                          ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                          UBSAN_OPTIONS='halt_on_error=1')
    compiler = ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn')
    with tempfile.TemporaryDirectory(prefix='sn-value-record-stores-') as folder:
        for label, code, expected in (
                ('ten-scalar-field-types', SCALARS, b'true\n'*40),
                ('nested-copies-calls-methods-and-hygiene', COMPOSITION, COMPOSITION_EXPECTED),
                ('thread-storage-copy-composition', WORKER+COMPOSITION+JOIN, COMPOSITION_EXPECTED+b'77\n1\n')):
            source = Path(folder)/(label+'.sn')
            source.write_text(code)
            fixtures.append((source, 'control:'+label, expected))
        jobs = [(source,label,expected,mode,opt) for source,label,expected in fixtures
                for mode in ('default','checked','unchecked') for opt in ('-O0','-O1','-O2')]
        settings = argparse.Namespace(c_ldlibs=None, compile_timeout=120, run_timeout=30)

        def verify(job):
            index,(source,label,expected,mode,opt) = job
            case = compare_case(compiler,source,index,mode,opt,folder,settings)
            case['source'] = label
            if os.name == 'nt':
                expected = expected.replace(b'\n',b'\r\n')
            case['expected_stdout_hex'] = expected.hex()
            case['passed'] = case['passed'] and all(
                target['compile']['status'] == 0 and target['run']['status'] == 0 and
                target['run']['stdout_hex'] == expected.hex() and target['run']['stderr_hex'] == ''
                for target in case['targets'].values())
            print(('PASS ' if case['passed'] else 'FAIL ')+label+' '+mode+' '+opt,flush=True)
            return case

        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS','2'))) as workers:
            cases = list(workers.map(verify,enumerate(jobs)))
    report = {'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'oracle_script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'control_sources':{'scalars':SCALARS,'composition':COMPOSITION,'worker':WORKER,'join':JOIN},
              'historical_sources_preserved':True,'sanitizers':args.sanitize,
              'environment':{name:os.environ.get(name) for name in
                  ('SN_CC','SN_CFLAGS','SN_RELEASE_CFLAGS','SN_RUSTFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')},
              'cases':cases,'independent_oracle_cases':len(cases),
              'passed':len(cases)==45 and all(case['passed'] for case in cases)}
    output = ROOT/'.sn'/('rust-parity-value-record-stores-sanitizers.json' if args.sanitize
                          else 'rust-parity-value-record-stores.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['passed']:
        raise SystemExit(1)
    print('PASS: 45 independent value record store cases')


if __name__ == '__main__':
    main()
