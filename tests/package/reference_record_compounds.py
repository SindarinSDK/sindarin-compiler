#!/usr/bin/env python3
"""Verify reference-record compounds and aliases in independent RS/RS/C source and sealed artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from textwrap import dedent

ROOT = Path(__file__).resolve().parents[2]
CONSUMER = '''import "left/src/api"
import "right/src/api"
import "reference/src/api"
fn main(): void =>
  var wanted: int[] = {10,19,4,9,65,10}
  for index in 0..6 =>
    var left: int = chooseLeft(index+1)
    var right: int = chooseRight(index+1)
    var reference: int = chooseReference(index+1)
    println(left==wanted[index] && right==wanted[index] && reference==wanted[index])
'''
BACKING = ''
BODY = '''struct Cell =>
  value: int
  fn combine(other: Cell as ref): int =>
    self.value += other.value
    return other.value
  fn forward(other: Cell as ref): int =>
    return self.combine(other)
struct Pair =>
  left: Cell
  right: Cell
fn increment(cell: Cell as ref): int =>
  return cell.value += 1
fn read(left: Cell as ref, right: Cell as ref): int =>
  return left.value + right.value
fn reset(cell: Cell as ref): void =>
  cell = Cell {value:9}
fn change(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  var total: int = read(left,right)
  reset(right)
  return total + left.value
fn step(cell: Cell as ref, depth: int): int =>
  cell.value += 1
  if depth > 0 =>
    return step(cell,depth-1)
  return cell.value
fn changeMethod(left: Cell as ref, right: Cell as ref): int =>
  left.value += 1
  return left.forward(right)
fn chooseValue(selector: int): int =>
  var cell: Cell = {value:4}
  if selector==1 =>
    var result: int = increment(cell)
    return result + cell.value
  if selector==2 =>
    return change(cell,cell)
  if selector==3 =>
    cell.value = 0
    return step(cell,3)
  if selector==4 =>
    var pair: Pair = {left: Cell {value:2},right: Cell {value:3}}
    return change(pair.left,pair.right)
  if selector==6 =>
    return changeMethod(cell,cell)
  var before: Cell = cell
  var other: Cell = {value:20}
  increment(cell)
  return before.value*10 + cell.value + other.value
'''


def verify(quick=False):
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'source_sha256': hashlib.sha256(CONSUMER.encode()).hexdigest(),
              'backing_sha256': hashlib.sha256(BACKING.encode()).hexdigest(),
              'body_sha256': hashlib.sha256(BODY.encode()).hexdigest(),
              'scope':'Plain reference record fields and aliases inside independent providers. Public native exports are scalar; public record-valued ABI and other ownership contracts remain separate.',
              'builds': [], 'cases': [],
              'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    def run(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        if p.returncode: raise RuntimeError(str(command) + '\n' + p.stderr.decode(errors='replace'))
        return p
    with tempfile.TemporaryDirectory(prefix='sn-reference-record-packages-') as tmp:
        app, packages = Path(tmp), []
        for name, runtime in [('left', 'RS'), ('right', 'RS'), ('reference', 'C')]:
            package = app / '.sn' / name
            (package / 'src').mkdir(parents=True)
            fn = 'choose' + name.title()
            (package / 'src/api.sn').write_text(f'native fn {fn}(selector: int): int\n')
            (package / 'src/body.sn').write_text(BODY.replace('chooseValue', fn))
            manifest = package / 'sn.yaml'
            manifest.write_text(dedent(f'''\
                name: {name}
                runtime: {runtime}
                native:
                  abi: 1.0
                  declarations: [src/api.sn]
                  builds:
                    - name: body
                      language: SN
                      entry: src/body.sn
                      sources: [src/body.sn]
                  bindings:
                    - declaration: src/api.sn::{fn}
                      function: {fn}
                      symbol: {name}_choose
                      build: body
                      convention: C
                      failure: abort
                      ownership: {{parameters: {{selector: value}}, result: value}}
                '''))
            built = run([compiler, '--build-native', manifest, '-o', package / 'built'], app)
            assembly = Path(json.loads(built.stdout)['assembly'])
            metadata = json.loads(assembly.read_text())
            assert metadata['units'][0]['implementation_runtime'] == runtime
            report['builds'].append({'name': name, 'runtime': runtime, 'metadata': metadata})
            packages.append((package, manifest, assembly))
        (app / 'main.sn').write_text(CONSUMER)
        (app / 'main.sn.c').write_text(BACKING)
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                for package, manifest, assembly in packages:
                    shutil.copytree(assembly.parent, package / 'sealed')
                    descriptor = package / 'sealed' / assembly.name
                    digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                    manifest.write_text(manifest.read_text().replace('native:\n', f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                    (package / 'src/body.sn').unlink()
            for target in ('c', 'rust'):
                for opt, mode in modes:
                    exe = app / 'consumer.exe'
                    command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
                    if mode != 'default': command.append('--' + mode)
                    built, executed = run(command, app), None
                    executed = run([exe], app)
                    expected = (b'true\r\n' if os.name == 'nt' else b'true\n') * 6
                    assert executed.stdout == expected and not executed.stderr
                    report['cases'].append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode,
                        'compile_status': built.returncode, 'run_status': executed.returncode,
                        'stdout_hex': executed.stdout.hex(), 'stderr_hex': executed.stderr.hex(), 'expected_stdout_hex': expected.hex()})
                    print(artifact, target, opt, mode, 'passed', flush=True)
    report['passed'] = True
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('package-reference-record-compounds' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS:', len(report['cases']), 'reference record source/sealed package cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
