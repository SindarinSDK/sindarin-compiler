#!/usr/bin/env python3
"""Check integer field compounds through explicit reference record parameters."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from check_rust_parity import compare_case

ROOT=Path(__file__).resolve().parents[1]

METHOD_BOUNDARY='''struct Cell =>
  value: int
  fn combine(other: Cell as ref): int =>
    self.value += other.value
    return other.value
  fn forward(other: Cell as ref): int =>
    return self.combine(other)
  fn recurse(other: Cell as ref, depth: int): int =>
    self.value++
    if depth > 0 =>
      return self.recurse(other,depth-1)
    return other.value
  static fn add(left: Cell as ref, right: Cell as ref): int =>
    return left.value += right.value
fn change(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  return left.forward(right)
fn main(): void =>
  var cell: Cell = {value:4}
  println(change(cell,cell)==10 && cell.value==10)
  println(Cell.add(cell,cell)==20 && cell.value==20)
  cell.value = 4
  var other: Cell = {value:3}
  println(change(cell,other)==3 && cell.value==8 && other.value==3)
'''

METHOD_RECURSIVE='''struct Cell =>
  value: int
  fn step(other: Cell as ref, depth: int): int =>
    self.value++
    if depth > 0 =>
      return self.step(other,depth-1)
    return other.value
fn change(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  return left.step(right,2)
fn main(): void =>
  var cell: Cell = {value:4}
  println(change(cell,cell)==8 && cell.value==8)
'''

COMPOSITION='''struct Cell =>
  value: int
  text: str
struct Pair =>
  left: Cell
  right: Cell
fn add(left: Cell as ref, right: Cell as ref): int =>
  return left.value += right.value
fn forward(value: Cell as ref): int =>
  return add(value, value)
fn main(): void =>
  var cell: Cell = {value:3,text:"preserved"}
  println(add(cell,cell)==6 && cell.value==6 && cell.text=="preserved")
  println(forward(cell)==12 && cell.value==12)
  var pair: Pair = {left: Cell {value:2,text:"left"},right: Cell {value:3,text:"right"}}
  println(add(pair.left,pair.right)==5 && pair.left.value==5 && pair.right.value==3)
  println(pair.left.text=="left" && pair.right.text=="right")
'''

PLAIN_ALIASES='''struct Cell =>
  value: int
struct Pair =>
  left: Cell
  right: Cell
fn add(left: Cell as ref, right: Cell as ref): int =>
  return left.value += right.value
fn forward(value: Cell as ref): int =>
  return add(value, value)
fn main(): void =>
  var cell: Cell = {value:3}
  println(add(cell,cell)==6 && cell.value==6)
  println(forward(cell)==12 && cell.value==12)
  var pair: Pair = {left: Cell {value:2},right: Cell {value:3}}
  println(add(pair.left,pair.right)==5 && pair.left.value==5 && pair.right.value==3)
'''

RECURSIVE='''struct Cell =>
  value: int
fn step(cell: Cell as ref, depth: int): int =>
  cell.value += 1
  if depth > 0 =>
    return step(cell,depth-1)
  return cell.value
fn main(): void =>
  var cell: Cell = {value:0}
  println(step(cell,3)==4 && cell.value==4)
'''

SHADOW='''struct Cell =>
  value: int
fn change(value: Cell as ref): int =>
  value.value += 1
  if true =>
    var value: Cell = {value:40}
    value.value += 2
    println(value.value==42)
  return value.value
fn main(): void =>
  var value: Cell = {value:4}
  println(change(value)==5 && value.value==5)
'''

TRANSITIVE='''struct Cell =>
  value: int
fn read(left: Cell as ref, right: Cell as ref): int =>
  return left.value + right.value
fn reset(value: Cell as ref): void =>
  value = Cell {value:9}
fn change(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  var total: int = read(left,right)
  reset(right)
  return total + left.value
fn main(): void =>
  var cell: Cell = {value:4}
  println(change(cell,cell)==19 && cell.value==9)
'''

MANAGED_PARENT='''struct Cell =>
  value: int
struct Holder =>
  marker: str
  cell: Cell
fn add(left: Cell as ref, right: Cell as ref): int =>
  return left.value += right.value
fn main(): void =>
  var holder: Holder = {marker:"preserved",cell:Cell {value:4}}
  println(add(holder.cell,holder.cell)==8)
  println(holder.cell.value==8 && holder.marker=="preserved")
'''

LOCAL_COMPUTED='''struct Cell =>
  value: int
fn change(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  var numbers: int[] = {4}
  numbers[0] += 2
  return left.value + numbers[0] + right.value
fn main(): void =>
  var cell: Cell = {value:4}
  println(change(cell,cell)==16 && cell.value==5)
'''

WRAPPING='''struct Wide =>
  value: uint
struct Word =>
  value: uint32
struct Octet =>
  value: byte
fn wide(cell: Wide as ref): uint =>
  var one: uint = 1
  return cell.value += one
fn word(cell: Word as ref): uint32 =>
  var one: uint32 = 1
  return cell.value += one
fn octet(cell: Octet as ref): byte =>
  var one: byte = 1
  return cell.value += one
fn main(): void =>
  var a: Wide = {value:18446744073709551615u}
  var b: Word = {value:4294967295}
  var c: Octet = {value:255b}
  println(wide(a)==0 && a.value==0)
  println(word(b)==0 && b.value==0)
  println(octet(c)==0 && c.value==0)
'''



def controls():
    result=[]
    for kind in ('int','long','int32','uint','uint32','byte'):
        source=f'''struct Cell =>
  value: {kind}
fn operate(cell: Cell as ref): {kind} =>
  var five: {kind} = 5
  var two: {kind} = 2
  var three: {kind} = 3
  var seven: {kind} = 7
  cell.value += five
  cell.value -= two
  cell.value *= three
  cell.value /= two
  cell.value %= five
  return cell.value += seven
fn forward(cell: Cell as ref): {kind} =>
  return operate(cell)
fn main(): void =>
  var cell: Cell = {{value:4}}
  println(operate(cell)==7 && cell.value==7)
  cell.value = 4
  println(forward(cell)==7 && cell.value==7)
'''
        result.append((kind,source,b'true\n'*2))
    result.append(('aliases-heap-fields',COMPOSITION,b'true\n'*4))
    result.append(('plain-aliases',PLAIN_ALIASES,b'true\n'*3))
    result.append(('unsigned-wrap',WRAPPING,b'true\n'*3))
    result.extend([('recursive',RECURSIVE,b'true\n'),('transitive',TRANSITIVE,b'true\n'),
                   ('managed-parent',MANAGED_PARENT,b'true\n'*2),('local-computed',LOCAL_COMPUTED,b'true\n'),
                   ('method-boundary',METHOD_BOUNDARY,b'true\n'*3),('method-recursive',METHOD_RECURSIVE,b'true\n')])
    return result


def function_body(source,name):
    start=source.index('{',source.index('fn '+name+'('));depth=1;end=start+1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}');end+=1
    return source[start+1:end-1]


def inspect_generated_rust(compiler,fixtures,directory):
    checks=[]
    for name,path,_ in fixtures:
        if name not in ('plain-aliases','recursive','transitive','method-boundary','method-recursive'):continue
        output=Path(directory)/(name+'.rs')
        run=subprocess.run([str(compiler),str(path),'--no-install','--emit-rust','-O0','-o',str(output)],capture_output=True,timeout=180)
        assert run.returncode==0,run.stderr
        source=output.read_text()
        headers=dict(re.findall(r'fn (__sn_reference_record(?:_\w+)?)\(([^\n]*)\)',source))
        assert headers and all('*mut Cell' in header for header in headers.values()),headers
        bodies={name:function_body(source,name) for name in headers}
        assert all('&mut ' not in body for body in bodies.values()),bodies
        main=function_body(source,'main')
        assert any(helper+'(std::ptr::addr_of_mut!' in main for helper in headers),main
        checks.append({'control':name,'raw_helper_headers':headers,'raw_helper_bodies':bodies,
            'generated_Rust':source,'passed':True})
    assert len(checks)==5
    return checks


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--quick',action='store_true')
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--output',type=Path)
    args=parser.parse_args();args.c_ldlibs=None;args.compile_timeout=180;args.run_timeout=30
    if args.sanitize:
        os.environ.update(SN_CC='gcc',SN_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer',ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    compiler=args.compiler.resolve()
    original=ROOT/'tests/rgen/errors/nested_as_ref_parameter_compound_mutation.sn'
    marker=json.loads(original.with_suffix('.parity.json').read_text())
    digest=lambda p:hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest()
    assert digest(original)==marker['source_sha256'] and digest(original.with_suffix('.expected'))==marker['legacy_oracle_sha256']
    with tempfile.TemporaryDirectory(prefix='sn-record-ref-compounds-') as directory:
        fixtures=[('original',original,bytes.fromhex(marker['stdout_hex']))]
        for name,source,expected in controls():
            path=Path(directory)/(name+'.sn');path.write_text(source);fixtures.append((name,path,expected))
        jobs=[(i,name,path,expected,opt,mode) for i,(name,path,expected) in enumerate(fixtures)
              for opt,mode in ([('-O0','default')] if args.quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')])]
        def check(job):
            i,name,path,expected,opt,mode=job
            case=compare_case(compiler,path,i,mode,opt,directory,args);case['control']=name;case['expected_stdout_hex']=expected.hex()
            wanted=expected.replace(b'\n',b'\r\n') if os.name=='nt' else expected
            for result in case['targets'].values():
                run=result.get('run',{});case['passed'] &= run.get('status')==0 and run.get('stdout_hex')==wanted.hex() and run.get('stderr_hex')==''
            return case
        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS','4'))) as pool:cases=list(pool.map(check,jobs))
        generated=inspect_generated_rust(compiler,fixtures,directory) if all(c['passed'] for c in cases) else []
    report={'passed':all(c['passed'] for c in cases),'cases':cases,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),'script_sha256':digest(Path(__file__)),
        'generated_Rust_checks':generated,
        'source_preserved':digest(original)==marker['source_sha256'],'legacy_oracle_preserved':digest(original.with_suffix('.expected'))==marker['legacy_oracle_sha256'],
        'environment':{n:os.environ.get(n) for n in ('SN_CC','SN_CFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')},
        'scope':'Direct integer fields of explicit reference value records; computed places, by-value compounds and floating fields retain separate contracts.'}
    output=args.output or ROOT/'.sn'/('reference-record-compounds'+('-sanitized' if args.sanitize else '')+'.json');output.write_text(json.dumps(report,indent=2)+'\n')
    failed=[(c['control'],c['optimization'],c['arithmetic_mode']) for c in cases if not c['passed']]
    print(('PASS' if report['passed'] else 'FAIL')+f': {len(cases)} C/Rust reference record compound pairs; failed={failed}')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
