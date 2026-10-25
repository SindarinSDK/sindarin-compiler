#!/usr/bin/env python3
"""Verify qualified plain-record methods in independent RS/RS/C source and sealed artifacts."""
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
  var wanted: int[] = {3,3,3,1,2,5,9}
  for index in 0..7 =>
    var left: int = chooseLeft(index+1)
    var right: int = chooseRight(index+1)
    var reference: int = chooseReference(index+1)
    println(left==wanted[index] && right==wanted[index] && reference==wanted[index])
'''
BACKING = ''
BODY = '''struct Point =>
  x: int
  fn increment(other: Point as ref): void =>
    other.x += self.x
  fn add(other: Point as val): int =>
    self.x += other.x
    other.x += 10
    return self.x
  fn replace(other: Point as ref): void =>
    other = Point {x:self.x}
  fn replaceReturn(other: Point as ref): Point =>
    return other = Point {x:self.x}
  fn ownedReturn(other: Point as val): Point =>
    self.x += 1
    return other = Point {x:5}
fn chooseValue(selector: int): int =>
  var point: Point = {x:1}
  var other: Point = {x:2}
  if selector==1 =>
    point.increment(other)
    return other.x
  if selector==2 =>
    return point.add(other)
  if selector==3 =>
    var values: Point[] = {other}
    point.increment(values[0])
    return values[0].x
  if selector==4 =>
    point.replace(other)
    return other.x
  if selector==5 =>
    return point.add(point)
  if selector==6 =>
    var result: Point = point.replaceReturn(other)
    other.x = 4
    return result.x+other.x
  var result: Point = point.ownedReturn(other)
  return result.x+point.x+other.x
'''


def verify(quick=False):
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'source_sha256': hashlib.sha256(CONSUMER.encode()).hexdigest(),
              'backing_sha256': hashlib.sha256(BACKING.encode()).hexdigest(),
              'builds': [], 'cases': [],
              'environment': {n: os.environ.get(n) for n in ('SN_CC', 'SN_CFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    def run(command, cwd):
        p = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        if p.returncode: raise RuntimeError(str(command) + '\n' + p.stderr.decode(errors='replace'))
        return p
    with tempfile.TemporaryDirectory(prefix='sn-method-struct-packages-') as tmp:
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
                    expected = (b'true\r\n' if os.name == 'nt' else b'true\n') * 7
                    assert executed.stdout == expected and not executed.stderr
                    report['cases'].append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode,
                        'compile_status': built.returncode, 'run_status': executed.returncode,
                        'stdout_hex': executed.stdout.hex(), 'stderr_hex': executed.stderr.hex(), 'expected_stdout_hex': expected.hex()})
                    print(artifact, target, opt, mode, 'passed', flush=True)
    report['passed'] = True
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('package-method-struct-parameters' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS:', len(report['cases']), 'method struct source/sealed package cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
