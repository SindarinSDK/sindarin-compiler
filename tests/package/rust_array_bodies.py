#!/usr/bin/env python3
"""C and Rust apps consume independent C/RS body archives with typed nested arrays."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('SN_COMPILER', ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn'))).resolve()


class TypedBodyArrayTests(unittest.TestCase):
    def test_native_headers_survive_typed_rust_body_exports(self):
        cases = []
        with tempfile.TemporaryDirectory(prefix='sn-typed-body-array-') as folder:
            app = Path(folder)
            for runtime in ('C','RS'):
                package = app/'.sn/arrays'
                if package.exists():
                    shutil.rmtree(package)
                (package/'src').mkdir(parents=True)
                api = ('native fn numbers(values: int32[][]): int32[][]\n'
                    'native fn texts(values: str[][]): str[][]\n'
                    'native fn changed(values: int32[][]): int32[][]\n'
                    'native fn replaced(values: int32[][]): int32[][]\n')
                body = ('fn numbers(values: int32[][]): int32[][] =>\n  return copyOf(values)\n'
                    'fn texts(values: str[][]): str[][] =>\n  return copyOf(values)\n'
                    'fn changed(values: int32[][]): int32[][] =>\n  values[0][0]=55\n  return values\n'
                    'fn replaced(values: int32[][]): int32[][] =>\n  var fresh: int32[][] = {{9,10}}\n  values=fresh\n  return copyOf(values)\n')
                shapes = [('int', '42', 2), ('long', '42', 2), ('int32', '42', 2),
                    ('uint', '42', 2), ('uint32', '42', 2), ('byte', '128', 2),
                    ('bool', 'true', 2), ('float', '1.25', 2), ('double', '1.25', 2),
                    ('char', "'q'", 2), ('str', '"shape"', 3), ('int32', '7', 32)]
                shape_calls = ''
                for index, (leaf, value, rank) in enumerate(shapes):
                    array_type = leaf + '[]' * rank
                    name = f'roundtrip{index}'
                    api += f'native fn {name}(values: {array_type}): {array_type}\n'
                    body += f'fn {name}(values: {array_type}): {array_type} =>\n  return copyOf(values)\n'
                    shape_calls += f'  println({name}(nil)==nil)\n'
                    if rank <= 3:
                        literal = '{' * rank + value + '}' * rank
                        access = '[0]' * rank
                        shape_calls += (f'  var shape{index}: {array_type} = {literal}\n'
                            f'  var result{index}: {array_type} = {name}(shape{index})\n'
                            f'  println(result{index}{access}==shape{index}{access})\n')
                (package/'src/api.sn').write_text(api)
                (package/'src/body.sn').write_text(body)
                manifest = (f'name: arrays\nruntime: {runtime}\nnative:\n  abi: 1.7\n  declarations: [src/api.sn]\n'
                    '  builds:\n    - name: body\n      language: SN\n      entry: src/body.sn\n      sources: [src/body.sn]\n'
                    '  bindings:\n')
                for name in ('numbers','texts','changed','replaced') + tuple(f'roundtrip{i}' for i in range(len(shapes))):
                    manifest += (f'    - declaration: src/api.sn::{name}\n      function: {name}\n'
                        f'      symbol: export_{name}\n      build: body\n      convention: C\n      failure: status\n'
                        '      ownership: {parameters: {values: borrowed}, result: owned}\n')
                (package/'sn.yaml').write_text(manifest)
                (app/'main.sn').write_text('import "arrays/src/api"\n'
                    'fn localReplace(values: int32[][]): int32[][] =>\n'
                    '  var fresh: int32[][] = {{7,8}}\n  values=fresh\n  return copyOf(values)\n'
                    'fn exerciseNumbers(): int32[][] =>\n'
                    '  var input: int32[][] = {{1,2}}\n  var result: int32[][] = numbers(input)\n'
                    '  input[0][0]=99\n  return result\n'
                    'fn exerciseText(): str[][] =>\n  var missing: str = nil\n'
                    '  var input: str[][] = {{"first",missing}}\n  var result: str[][] = texts(input)\n'
                    '  input[0][0]="changed"\n  return result\n'
                    'fn main(): void =>\n  println(numbers(nil)==nil)\n  println(texts(nil)==nil)\n'
                    '  var integers: int32[][] = exerciseNumbers()\n  println(integers[0][0])\n'
                    '  println(integers[0][1])\n  var lines: str[][] = exerciseText()\n'
                    '  println(lines[0][0])\n  println(lines[0][1]==nil)\n'
                    '  var original: int32[][] = {{1,2}}\n  var mutation: int32[][] = changed(original)\n'
                    '  println(original[0][0])\n  println(mutation[0][0])\n'
                    '  var replacement: int32[][] = replaced(original)\n'
                    '  println(original[0][0])\n  println(replacement[0][0])\n'
                    '  var local: int32[][] = localReplace(original)\n'
                    '  println(original[0][0])\n  println(local[0][0])\n'
                    '  println(replaced(nil)[0][0])\n' + shape_calls)
                built = subprocess.run([str(COMPILER),'--build-native',str(package/'sn.yaml'),
                    '-o',str(package/'built')],cwd=app,capture_output=True,timeout=180)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly = Path(json.loads(built.stdout)['assembly'])
                for artifact in ('source','sealed'):
                    if artifact == 'sealed':
                        shutil.copytree(assembly.parent,package/'sealed')
                        descriptor = package/'sealed'/assembly.name
                        digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                        (package/'sn.yaml').write_text(manifest.replace('native:\n',
                            f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                        (package/'src/body.sn').unlink()
                    for target in ('c','rust'):
                        for opt in ('-O0','-O1','-O2'):
                            for mode in ('default','checked','unchecked'):
                                exe = app/'consumer.exe'
                                command = [str(COMPILER),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                                if mode != 'default':
                                    command.append('--'+mode)
                                built = subprocess.run(command,cwd=app,capture_output=True,timeout=180)
                                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                                run = subprocess.run([str(exe)],cwd=app,capture_output=True,timeout=15)
                                expected = b'true\ntrue\n1\n2\nfirst\ntrue\n55\n55\n55\n9\n55\n7\n9\n'
                                expected += b'true\n' * 23
                                if os.name == 'nt':
                                    expected = expected.replace(b'\n',b'\r\n')
                                if (run.returncode,run.stdout,run.stderr) != (0,expected,b''):
                                    (ROOT/'.sn/typed-body-array-failure.json').write_text(json.dumps({'runtime':runtime,'artifact':artifact,'target':target,'opt':opt,'mode':mode,'exit':run.returncode,'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex()},indent=2)+'\n')
                                self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))
                                cases.append({'runtime':runtime,'artifact':artifact,'target':target,'opt':opt,'mode':mode,'passed':True})
        (ROOT/'.sn/typed-body-array-results.json').write_text(json.dumps({'passed':True,'cases':cases},indent=2)+'\n')


if __name__ == '__main__':
    unittest.main(verbosity=2)
