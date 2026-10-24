#!/usr/bin/env python3
"""Check C header identity, copies and effects in unchanged nested array search."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = 'tests/rgen/errors/array_search_nested_array_elements.sn'
SOURCE_SHA256 = '06a604c7d8e37134f885c229fc7cc5aa3754da0ccbf8be9bf7ac8bdebe334980'


def identity_control():
    lines = ['fn borrowed(values: int[][], needle: int[]): bool =>',
             '  return values.contains(needle)', 'fn mutate(values: int[][], needle: int[]): bool =>',
             '  values[0].push(2)', '  return values.contains(needle)',
             'fn make(): int[][] =>', '  return {{1}}',
             'fn next(calls: int as ref): int =>', '  calls++', '  return 0',
             'fn main(): void =>']
    expected = []
    for index, (kind, value) in enumerate((('int','1'),('long','1'),('int32','1'),
            ('uint','1u'),('uint32','1u32'),('byte','1'),('bool','true'),
            ('char',"'a'"),('float','1.0f'),('double','1.0'),('str','"same"'))):
        lines += [f'  var needle{index}: {kind}[] = {{{value}}}',
                  f'  var values{index}: {kind}[][] = {{needle{index}}}',
                  f'  var equal{index}: {kind}[] = {{{value}}}',
                  f'  println(values{index}.contains(needle{index}))',
                  f'  println(values{index}.contains(values{index}[0]))',
                  f'  println(values{index}.indexOf(needle{index}))',
                  f'  println(values{index}.indexOf(values{index}[0]))',
                  f'  println(values{index}.contains(equal{index}))',
                  f'  println(values{index}.indexOf(equal{index}))',
                  f'  values{index}[0].push({value})',
                  f'  println(values{index}[0].length)', f'  println(needle{index}.length)']
        expected += ['false','true','-1','0','false','-1','2','1']
    lines += ['  var missing: int[] = nil', '  var empty: int[] = {}',
              '  var values: int[][] = {missing,empty,missing}',
              '  println(values.contains(missing))', '  println(values.indexOf(missing))',
              '  println(values.contains(empty))', '  println(values.indexOf(empty))',
              '  println(values.contains(values[1]))', '  println(values.indexOf(values[1]))',
              '  var needle: int[] = {1}', '  var forwarded: int[][] = {needle}',
              '  println(borrowed(forwarded,needle))',
              '  println(borrowed(forwarded,forwarded[0]))',
              '  var calls: int = 0', '  println(borrowed(forwarded,forwarded[next(calls)]))',
              '  println(calls)', '  println(borrowed(make(),needle))',
              '  println(mutate(forwarded,forwarded[0]))', '  println(forwarded[0].length)']
    expected += ['true','0','false','-1','true','1','false','true','true','1','false','true','2']
    return '\n'.join(lines)+'\n', ('\n'.join(expected)+'\n').encode()


def rank_control():
    lines = ['fn main(): void =>', '  var level1: int[] = {7}']
    for rank in range(2,33):
        lines += [f'  var level{rank}: int'+('[]'*rank)+f' = {{level{rank-1}}}',
                  f'  println(level{rank}.contains(level{rank-1}))',
                  f'  println(level{rank}.contains(level{rank}[0]))',
                  f'  println(level{rank}.indexOf(level{rank-1}))',
                  f'  println(level{rank}.indexOf(level{rank}[0]))']
    return '\n'.join(lines)+'\n', b'false\ntrue\n-1\n0\n'*31


EFFECTS = """struct Box =>
  values: int[][]
  fn clear(): int[] =>
    self.values.clear()
    return {1}
fn clear(values: int[][]): int[] =>
  values.clear()
  return {1}
fn append(values: int[][]): int[] =>
  values.push({9})
  return {7}
fn receiver(calls: int as ref): int[][] =>
  calls++
  println("receiver")
  return {{1}}
fn argument(calls: int as ref): int[] =>
  calls++
  println("argument")
  return {1}
fn index(calls: int as ref): int =>
  calls++
  return 0
fn plainArgument(calls: int as ref): int[] =>
  calls++
  return {1}
fn main(): void =>
  var values: int[][] = {{1}}
  println(values.contains(append(values)))
  println(values.length)
  println(values.indexOf(append(values)))
  println(values.length)
  println(values.contains(clear(values)))
  println(values.length)
  var box: Box = {values: {{1}}}
  println(box.values.contains(box.clear()))
  println(box.values.length)
  var calls: int = 0
  println(receiver(calls).contains(argument(calls)))
  println(calls)
  var indexes: int = 0
  var arguments: int = 0
  var outer: int[][][] = {{{1}}}
  println(outer[index(indexes)].contains(plainArgument(arguments)))
  println(indexes)
  println(arguments)
"""
EFFECTS_EXPECTED = b'false\n2\n-1\n3\nfalse\n0\nfalse\n0\nreceiver\nargument\nfalse\n2\nfalse\n1\n1\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    original = ROOT/ORIGINAL
    if hashlib.sha256(original.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise ValueError('frozen original nested-array search source changed')
    controls = [('original', original.read_text(), b''),
                ('identity', *identity_control()), ('ranks2-32', *rank_control()),
                ('effects', EFFECTS, EFFECTS_EXPECTED)]
    modes = [('-O0','default')] if args.quick else [(o,m) for o in ('-O0','-O1','-O2')
                                                 for m in ('default','checked','unchecked')]
    report = {'passed': False, 'cases': [], 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'original_source_sha256': SOURCE_SHA256,
              'controls': {name: source for name,source,_ in controls if name!='original'},
              'scope': 'Nested canonical C array header identity/copies/default loans/nil inner slots/ranks2-32 and receiver/argument effects. Nil outer dereference and unsupported unrelated contracts receive no parity credit.',
              'environment': {name: os.environ.get(name) for name in ('SN_CC','SN_CFLAGS','SN_RELEASE_CFLAGS',
                             'SN_RUSTFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS','') else ''
    output = ROOT/'.sn'/('rust-nested-array-search'+suffix+'.json')
    output.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sn-nested-array-search-') as folder:
        work = Path(folder)
        for name,source,expected in controls:
            source_path = original if name=='original' else work/'main.sn'
            if name!='original':source_path.write_text(source)
            if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
            for opt,mode in modes:
                case = {'name':name,'opt':opt,'mode':mode,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                        'expected_stdout_hex':expected.hex(),'targets':{}}
                for target in ('c','rust'):
                    exe=work/(target+'.exe')
                    command=[str(compiler),str(source_path),'--no-install','--target',target,opt,'-o',str(exe)]
                    if mode!='default':command.append('--'+mode)
                    build=subprocess.run(command,cwd=work,capture_output=True,timeout=180)
                    result={'compile_status':build.returncode,'compile_stderr_hex':build.stderr.hex()}
                    if build.returncode==0:
                        run=subprocess.run([str(exe)],cwd=work,capture_output=True,timeout=30)
                        result.update(run_status=run.returncode,stdout_hex=run.stdout.hex(),stderr_hex=run.stderr.hex(),
                                      passed=(run.returncode,run.stdout,run.stderr)==(0,expected,b''))
                    else:result['passed']=False
                    case['targets'][target]=result
                case['passed']=all(v['passed'] for v in case['targets'].values())
                report['cases'].append(case)
                output.write_text(json.dumps(report,indent=2)+'\n')
                print(('PASS' if case['passed'] else 'FAIL'),name,opt,mode,flush=True)
                if not case['passed']:raise SystemExit(1)
    assert len(report['cases'])==len(controls)*len(modes)
    report['passed']=True
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(report["cases"])} nested-array search C/Rust mode pairs')


if __name__=='__main__':
    main()
