#!/usr/bin/env python3
"""Retain C integer boundary support in independent Rust bodies and sealed artifacts."""
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
@source "main.sn.c"
native fn foreignMid(): uint
native fn foreignExpected(): int
fn main(): void =>
  var value: uint = foreignMid()
  println(chooseLeft(value)==foreignExpected() && chooseRight(value)==foreignExpected())
  println(chooseLeft(value)==chooseReference(value) && chooseRight(value)==chooseReference(value))
'''
BACKING = '''#include "sn_types.h"
uint64_t foreignMid(void) { return UINT64_C(9223372036854775808); }
long long foreignExpected(void) {
    return !(sizeof(-9223372036854775808LL)>8 && -9223372036854775808LL < 0);
}
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
    with tempfile.TemporaryDirectory(prefix='sn-integer-match-packages-') as tmp:
        app, packages = Path(tmp), []
        for name, runtime in [('left', 'RS'), ('right', 'RS'), ('reference', 'C')]:
            package = app / '.sn' / name
            (package / 'src').mkdir(parents=True)
            fn = 'choose' + name.title()
            (package / 'src/api.sn').write_text(f'native fn {fn}(value: uint): int\n')
            (package / 'src/body.sn').write_text(f'fn {fn}(value: uint): int =>\n  return match value =>\n    9223372036854775808u => 1\n    else => 0\n')
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
                      ownership: {{parameters: {{value: value}}, result: value}}
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
                    expected = b'true\r\ntrue\r\n' if os.name == 'nt' else b'true\ntrue\n'
                    assert executed.stdout == expected and not executed.stderr
                    report['cases'].append({'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode,
                        'compile_status': built.returncode, 'run_status': executed.returncode,
                        'stdout_hex': executed.stdout.hex(), 'stderr_hex': executed.stderr.hex(), 'expected_stdout_hex': expected.hex()})
                    print(artifact, target, opt, mode, 'passed', flush=True)
    report['passed'] = True
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / '.sn' / ('package-integer-match-patterns' + suffix + '.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS:', len(report['cases']), 'integer match source/sealed package cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quick', action='store_true')
    verify(parser.parse_args().quick)
