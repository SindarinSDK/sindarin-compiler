#!/usr/bin/env python3
"""Preserve C floating comparison promotions and numeric match arm conversions."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]
ORIGINALS = {
    'statement_match_double_float_pattern': b'mixed\n',
    'statement_match_float_double_pattern': b'mixed\n',
    'value_match_double_float_pattern': b'10\n',
    'value_match_float_double_pattern': b'10\n',
    'value_match_exact_float_result_type': b'1.00000\n',
    'value_match_exact_integer_result_type': b'10\n',
    'value_match_final_shape_before_prefix': b'10\n',
}
FLOATING = '''fn subject(trace: int as ref): float =>
  trace = trace*10+1
  return 0.1f
fn pattern(trace: int as ref, value: double): double =>
  trace = trace*10+2
  return value
fn main(): void =>
  var trace: int = 0
  match subject(trace) =>
    pattern(trace,0.1) => println(false)
    pattern(trace,0.10000000149011612) => println(true)
    pattern(trace,0.1) => println(false)
    else => println(false)
  println(trace == 122)
  var large: float = 16777216.0f
  var exact: int = match large =>
    16777217.0 => 9
    16777216.0 => 4
    else => 0
  println(exact == 4)
  var decimal: double = 0.1
  match decimal =>
    0.1f => println(true)
    else => println(false)
  var negative: double = -0.1
  var negativeResult: bool = match negative =>
    -0.1f => true
    else => false
  println(negativeResult)
  var widened: double = match 2 =>
    1 => 0.0
    else => 0.1f
  println(widened == 0.1)
  var narrowed: float = match 2 =>
    1 => 0.0f
    else => 16777217.0
  println(narrowed == 16777216.0f)
  var zero: float = -0.0f
  match zero =>
    0.0 => println(true)
    else => println(false)
  var computed: float = 0.1f
  var answer: bool = match decimal =>
    computed => false
    else => true
  println(answer)
  match computed =>
    0.1f => println(false)
    else => println(true)
  var storedResult: double = match 2 =>
    1 => 0.0
    else => computed
  println(storedResult == 0.10000000149011612)
  var wideArithmetic: float = match 2 =>
    1 => 0.0f
    else => (large+1.0f)-large
  println(wideArithmetic)
'''


def integer_control():
    lines = ['fn main(): void =>']
    for left in ('int','long','int32','uint','uint32','byte'):
        for right in ('int','long','int32','uint','uint32','byte'):
            if left != right and 'int' not in (left,right):
                continue
            name = left+'_'+right
            lines += [f'  var destination_{name}: {left} = 0',
                      f'  var source_{name}: {right} = 21',
                      f'  var chosen_{name}: {left} = match 2 =>',
                      f'    1 => destination_{name}', f'    else => source_{name}',
                      f'  println(chosen_{name} == 21)']
    lines += ['  var narrowed: byte = match 2 =>', '    1 => 0b', '    else => -1',
              '  println(narrowed == 255b)',
              '  var wrapped: uint32 = match 2 =>', '    1 => 0u32',
              '    else => 4294967297', '  println(wrapped == 1u32)']
    return '\n'.join(lines)+'\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize',action='store_true')
    args = parser.parse_args()
    if args.sanitize:
        os.environ.update(SN_CFLAGS=os.environ.get('SN_CFLAGS','')+' -fsanitize=address,undefined -fno-omit-frame-pointer',
                          ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    compiler = ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn')
    digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest()
    inputs = []
    for name,expected in ORIGINALS.items():
        path = ROOT/'tests/rgen/errors'/(name+'.sn')
        marker = json.loads(path.with_suffix('.parity.json').read_text())
        assert (digest(path),digest(path.with_suffix('.expected')),expected.hex(),0) == (
            marker['source_sha256'],marker['legacy_oracle_sha256'],marker['stdout_hex'],marker['exit'])
        inputs.append((path,path.relative_to(ROOT).as_posix(),expected))
    controls = {'floating': FLOATING, 'integers': integer_control()}
    with tempfile.TemporaryDirectory(prefix='sn-numeric-match-') as temporary:
        folder = Path(temporary)
        for name,expected in [('floating',b'true\n'*11),('integers',b'true\n'*18)]:
            path=folder/(name+'.sn');path.write_text(controls[name]);inputs.append((path,'control:'+name,expected))
        jobs=[(source,label,expected,mode,opt) for source,label,expected in inputs
              for mode in ('default','checked','unchecked') for opt in ('-O0','-O1','-O2')]
        settings=argparse.Namespace(c_ldlibs=None,compile_timeout=120,run_timeout=30)
        def verify(job):
            index,(source,label,expected,mode,opt)=job
            case=compare_case(compiler,source,index,mode,opt,str(folder),settings);case['source']=label
            if label == 'control:floating':
                # Default arithmetic is checked at O0/O1 and unchecked at O2.
                # C's typed checked helpers round the addition to float;
                # unchecked C expressions retain the unsuffixed literal's double width.
                checked = mode == 'checked' or (mode == 'default' and opt != '-O2')
                expected += b'0.00000\n' if checked else b'1.00000\n'
            if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
            for target in case['targets'].values():
                if target['compile']['status']!=0 or (target['run']['status'],target['run']['stdout_hex'],target['run']['stderr_hex'])!=(0,expected.hex(),''):
                    raise ValueError(json.dumps(case))
            assert case['passed'];case['expected_stdout_hex']=expected.hex();print('PASS',label,opt,mode,flush=True);return case
        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS','2'))) as workers:
            cases=list(workers.map(verify,enumerate(jobs)))
    assert len(cases)==81
    report={'passed':True,'cases':cases,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
            'script_sha256':digest(Path(__file__)),'control_sources':controls,
            'environment':{k:os.environ.get(k) for k in ('SN_CC','SN_CFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')},
            'independent_oracle_cases':81,'historical_sources_and_diagnostics_preserved':True}
    out=ROOT/'.sn'/('rust-numeric-match-sanitized.json' if args.sanitize else 'rust-numeric-match.json')
    out.write_text(json.dumps(report,indent=2)+'\n');print('PASS: 81 numeric match pairs',flush=True)


if __name__=='__main__':main()
