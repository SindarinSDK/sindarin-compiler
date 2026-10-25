#!/usr/bin/env python3
"""Check synchronous borrowed scalar storage in independent C package adapters."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SCALARS = {'int': 'long long', 'long': 'long long', 'int32': 'int32_t',
           'uint': 'uint64_t', 'uint32': 'uint32_t', 'byte': 'unsigned char',
           'char': 'char', 'bool': 'bool', 'float': 'float', 'double': 'double'}


def inputs():
    declarations, backing, consumer, bindings = [], ['#include <stdint.h>', '#include <stdbool.h>'], [], []
    direct = []
    for kind, c_type in SCALARS.items():
        initial = 'false' if kind == 'bool' else "'x'" if kind == 'char' else '0.0f' if kind == 'float' else '0.0' if kind == 'double' else '0'
        first = 'true' if kind == 'bool' else "'a'" if kind == 'char' else '6'
        second = '*b = !*b' if kind == 'bool' else "*b = 'z'" if kind == 'char' else '*b += 2'
        expected = 'false' if kind == 'bool' else "'z'" if kind == 'char' else '8'
        consumer += [f'  var {kind}Value: {kind} = {initial}']
        for form in ('ref', 'pointer'):
            name = form + kind.title()
            typ = kind + ' as ref' if form == 'ref' else '*' + kind
            declarations.append(f'native fn {name}(a: {typ}, b: {typ}): void')
            backing.append(f'void {name}({c_type} *a, {c_type} *b) {{ *a = {first}; {second}; }}')
            arguments = f'{kind}Value, {kind}Value' if form == 'ref' else f'addressOf({kind}Value), addressOf({kind}Value)'
            consumer += [f'  {name}({arguments})', f'  println({kind}Value == {expected})']
            other = kind + form.title() + 'Other'
            consumer += [f'  var {other}: {kind} = {initial}']
            distinct = f'{kind}Value, {other}' if form == 'ref' else f'addressOf({kind}Value), addressOf({other})'
            second_expected = 'true' if kind == 'bool' else "'z'" if kind == 'char' else '2'
            consumer += [f'  {name}({distinct})', f'  println({kind}Value == {first} && {other} == {second_expected})']
            if form == 'ref':
                direct += [f'  var direct{kind}: {kind} = {initial}', f'  var other{kind}: {kind} = {initial}',
                           f'  {name}(direct{kind}, direct{kind})', f'  println(direct{kind} == {expected})',
                           f'  {name}(direct{kind}, other{kind})', f'  println(direct{kind} == {first} && other{kind} == {second_expected})']
            bindings += [f'    - declaration: src/api.sn::{name}', f'      function: {name}', f'      symbol: memory_{name}',
                         '      build: body', '      convention: C', '      failure: abort',
                         '      ownership: {parameters: {a: borrowed, b: borrowed}, result: value}']
    declarations += ['native fn bytes(data: *byte, count: int): int']
    backing += ['long long bytes(unsigned char *data, long long count) { if (!data) return count == 0; data[1] = 42; return data[0] == 0 && data[2] == 255; }']
    bindings += ['    - declaration: src/api.sn::bytes', '      function: bytes', '      symbol: memory_bytes',
                 '      build: body', '      convention: C', '      failure: abort',
                 '      ownership: {parameters: {data: borrowed, count: value}, result: value}']
    consumer += ['  var data: byte[] = {0b,128b,255b}', '  println(bytes(addressOf(data), 3) == 1 && data[1] == 42)',
                 '  println(bytes(nil, 0) == 1)']
    source = 'import "memory/src/api"\nnative fn exercise(): void =>\n' + '\n'.join(consumer) + '\nfn direct(): void =>\n' + '\n'.join(direct) + '\nfn main(): void =>\n  direct()\n  exercise()\n'
    manifest = 'name: memory\nruntime: C\nnative:\n  abi: 1.9\n  declarations: [src/api.sn]\n  builds:\n    - name: body\n      language: C\n      sources: [src/body.c]\n  bindings:\n' + '\n'.join(bindings) + '\n'
    return '\n'.join(declarations)+'\n', '\n'.join(backing)+'\n', source, manifest


def verify(quick=False):
    compiler = ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn')
    declarations, backing, source, manifest_text = inputs()
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(), 'cases': [],
              'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'scalar_families': list(SCALARS), 'declarations': declarations, 'backing': backing, 'consumer': source, 'negative_contracts': [],
              'environment': {k: os.environ.get(k) for k in ('SN_CC','SN_CFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')}}
    def run(command, cwd):
        r = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, timeout=180)
        if r.returncode: raise ValueError(str(command)+'\n'+r.stderr.decode(errors='replace'))
        return r
    for provider in ('C', 'SN'):
        with tempfile.TemporaryDirectory(prefix='sn-borrowed-scalars-') as temp:
            app=Path(temp);package=app/'.sn/memory';(package/'src').mkdir(parents=True)
            (package/'src/api.sn').write_text(declarations);(package/'src/body.c').write_text(backing)
            active_manifest = manifest_text
            if provider == 'SN':
                body = []
                for scalar in SCALARS:
                    first = 'true' if scalar == 'bool' else "'a'" if scalar == 'char' else '6.0f' if scalar == 'float' else '6.0' if scalar == 'double' else '6'
                    second = 'b = !b' if scalar == 'bool' else "b = 'z'" if scalar == 'char' else 'b += 2'
                    name = 'ref'+scalar.title()
                    body += [f'fn {name}(a: {scalar} as ref, b: {scalar} as ref): void =>', f'  a = {first}', f'  {second}']
                    active_manifest = active_manifest.replace(f'      symbol: memory_{name}\n      build: body', f'      symbol: memory_{name}\n      build: facade')
                (package/'src/body.sn').write_text('\n'.join(body)+'\n')
                active_manifest = active_manifest.replace('  bindings:\n', '    - name: facade\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n  bindings:\n')
                (package/'src/body.c').write_text('\n'.join(line for line in backing.splitlines() if not line.startswith('void ref'))+'\n')
            manifest=package/'sn.yaml';manifest.write_text(active_manifest);(app/'main.sn').write_text(source)
            for label, text, wanted in [
                    ('old-ABI', active_manifest.replace('abi: 1.9', 'abi: 1.8'), b'require native ABI 1.9'),
                    ('wrong-ownership', active_manifest.replace('a: borrowed', 'a: value', 1), b'input ownership'),
                    ('Rust-provider', active_manifest.replace('language: C', 'language: RS').replace('sources: [src/body.c]', 'entry: src/body.rs\n      sources: [src/body.rs]'), b'currently requires a C provider')]:
                manifest.write_text(text)
                rejected=subprocess.run([str(compiler),'--build-native',str(manifest),'-o',str(package/('rejected-'+label))],cwd=app,capture_output=True,timeout=180)
                if rejected.returncode == 0 or wanted not in rejected.stderr:raise ValueError(f'{label}: {rejected.returncode} {rejected.stderr!r}')
                report['negative_contracts'].append({'provider':provider,'case':label,'status':rejected.returncode,'stderr':rejected.stderr.decode(errors='replace'),'passed':True})
            manifest.write_text(active_manifest)
            built=run([compiler,'--build-native',manifest,'-o',package/'built'],app)
            assembly=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(assembly.read_text())
            assert len(metadata['provider_signatures']) == 21
            modes=[('-O0','default')] if quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]
            for artifact in ('source','sealed'):
                if artifact == 'sealed':
                    shutil.copytree(assembly.parent,package/'sealed')
                    manifest.write_text(active_manifest.replace('native:\n','native:\n  assembly: {path: sealed/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                    (package/'src/body.c').unlink()
                    if (package/'src/body.sn').exists():(package/'src/body.sn').unlink()
                for target in ('c','rust'):
                    for opt,mode in modes:
                        executable=app/'client.exe';command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',executable]
                        if mode != 'default':command.append('--'+mode)
                        run(command,app);result=run([executable],app)
                        expected=(b'true\r\n' if os.name=='nt' else b'true\n')*62
                        if result.stdout != expected or result.stderr:raise ValueError(f'{artifact}/{target}/{opt}/{mode}: {result.stdout!r} {result.stderr!r}')
                        report['cases'].append({'provider':provider,'artifact':artifact,'target':target,'optimization':opt,'mode':mode,'stdout_hex':result.stdout.hex(),'passed':True})
    report['passed']=True
    output=ROOT/'.sn'/('package-borrowed-scalar-memory'+('-sanitized' if os.environ.get('SN_CFLAGS') else '')+'.json')
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(report["cases"])} borrowed scalar memory cases, 62 independent outputs each')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true')
    verify(parser.parse_args().quick)
