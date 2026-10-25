#!/usr/bin/env python3
"""Explicit scalar values preserve local copies and independent package calls."""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import unittest

import callable_methods as fixtures

sys.path.insert(0, str(fixtures.ROOT/'scripts'))
from check_rust_parity import compare_case
from check_rust_scalar_parameter_rhs import control_source, CHARACTERS

SCALARS = (
    ('int', 'long long', 'i64', 'int64', '1', '12', '11'),
    ('long', 'long long', 'i64', 'int64', '1', '12', '11'),
    ('int32', 'int32_t', 'i32', 'int32', '1', '12', '11'),
    ('uint', 'uint64_t', 'u64', 'uint64', '1', '12', '11'),
    ('uint32', 'uint32_t', 'u32', 'uint32', '1', '12', '11'),
    ('byte', 'unsigned char', 'u8', 'uint8', '1', '0x0C', '0x0B'),
    ('float', 'float', 'f32', 'float32', '1.0f', '12.00000', '11.00000'),
    ('double', 'double', 'f64', 'float64', '1.0', '12.00000', '11.00000'),
    ('bool', 'bool', 'bool', 'bool', 'false', 'false', 'true'),
    ('char', 'char', 'u8', 'uint8', "'a'", 'c', 'b'),
)


def integral_control():
    functions, main = [], ['fn main(): void =>']
    kinds = ('int','long','int32','uint','uint32','byte')
    operations = (('+=',15),('-=',12),('*=',36),('/=',12),('%=',0),
                  ('|=',3),('&=',3),('^=',0),('=',12),('<<=',96),('>>=',12))
    for qualifier in ('default','as_val'):
        for left in kinds:
            for right in kinds:
                name = qualifier+'_'+left+'_'+right
                qual = ' as val' if qualifier=='as_val' else ''
                body = [f'fn {name}(value: {left}{qual}, amount: {right}): bool =>']
                for operation, expected in operations:
                    body += [f'  value {operation} '+('12' if operation=='=' else 'amount'),
                             f'  if value != {expected} =>', '    return false']
                functions += body+['  return true']
                main += [f'  var original_{name}: {left} = 12',f'  var rhs_{name}: {right} = 3',
                         f'  println({name}(original_{name},rhs_{name}))',
                         f'  println(original_{name} == 12)']
    functions += ['fn byteDivide(value: byte as val, amount: int): byte =>',
                  '  value /= amount', '  return value',
                  'fn wideDivide(value: uint32, amount: long): uint32 =>',
                  '  value /= amount', '  return value',
                  'fn narrowSigned(value: int32 as val, amount: int): int32 =>',
                  '  value += amount', '  return value']
    main += ['  println(byteDivide(200,-1))', '  println(wideDivide(12,4294967298))',
             '  println(narrowSigned(2147483647,1))']
    return '\n'.join(functions+main)+'\n'


def native_callback_control():
    functions, main = [], ['fn main(): void =>']
    for kind,_,_,_,initial,_,_ in SCALARS:
        update = ('value = !value' if kind=='bool' else "value = 'b'" if kind=='char'
                  else 'value += 1.0f' if kind=='float' else 'value += 1.0' if kind=='double'
                  else 'value += 1')
        expected = ('true' if kind=='bool' else "'b'" if kind=='char'
                    else '2.0f' if kind=='float' else '2.0' if kind=='double' else '2')
        callback = f'fn({kind}): {kind}'
        functions += [f'native fn invoke_{kind}(action: {callback}, value: {kind} as val): {kind} =>',
                      '  return action(value)',
                      f'native fn relay_{kind}(action: {callback}): {callback} =>',
                      '  return action', f'fn change_{kind}(value: {kind} as val): {kind} =>',
                      '  '+update, '  return value']
        main += [f'  var original_{kind}: {kind} = {initial}',
                 f'  var named_{kind} = change_{kind}',
                 f'  println(invoke_{kind}(named_{kind}, original_{kind}) == {expected})',
                 f'  println(original_{kind} == {initial})',
                 f'  var closure_{kind} = fn(value: {kind} as val): {kind} =>',
                 '    '+update, '    return value',
                 f'  println(invoke_{kind}(closure_{kind}, original_{kind}) == {expected})',
                 f'  var returned_{kind} = relay_{kind}(closure_{kind})',
                 f'  println(returned_{kind} == closure_{kind})',
                 f'  println(returned_{kind}(original_{kind}) == {expected})',
                 f'  println(original_{kind} == {initial})']
    return '\n'.join(functions+main)+'\n'


class ScalarValueTests(unittest.TestCase):
    setUp = fixtures.CallableMethodTests.setUp
    write = fixtures.CallableMethodTests.write
    binding = staticmethod(fixtures.CallableMethodTests.binding)

    def report(self, name, cases, **details):
        suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
        output = fixtures.ROOT/'.sn'/('package-scalar-value-'+name+suffix+'.json')
        output.parent.mkdir(exist_ok=True)
        result = {'passed': True, 'cases': cases,
            'compiler_sha256': hashlib.sha256(fixtures.COMPILER.read_bytes()).hexdigest(),
            'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'environment': {name: os.environ.get(name) for name in
                ('SN_CC', 'SN_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')}, **details}
        output.write_text(json.dumps(result, indent=2)+'\n')

    def test_language_mutations_calls_and_character_domain(self):
        original = fixtures.ROOT/'tests/rgen/errors/floating_as_val_parameter_compound_mutation.sn'
        marker = json.loads(original.with_suffix('.parity.json').read_text())
        digest = lambda p: hashlib.sha256(p.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
        self.assertEqual(digest(original), marker['source_sha256'])
        self.assertEqual(digest(original.with_suffix('.expected')), marker['legacy_oracle_sha256'])
        self.assertEqual((marker['exit'], marker['stdout_hex']), (0, ''))
        pattern = r'\bvalue: (int32|uint32|double|float|long|uint|byte|bool|char|int)(?=[,)])'
        controls = {'scalar-effects': re.sub(pattern, r'value: \1 as val', control_source()),
                    'character-domain': re.sub(pattern, r'value: \1 as val', CHARACTERS),
                    'integral-promotions': integral_control(),
                    'native-callbacks': native_callback_control()}
        inputs = [(original, original.relative_to(fixtures.ROOT).as_posix(), b'')]
        for label, code, expected in (
                ('scalar-effects', controls['scalar-effects'], b'true\n'*127+b'20\n11\n9\n'),
                ('character-domain', controls['character-domain'], b'256\n'),
                ('integral-promotions', controls['integral-promotions'], b'true\n'*144+b'0x38\n0\n-2147483648\n'),
                ('native-callbacks', controls['native-callbacks'], b'true\n'*60)):
            source = self.root/(label+'.sn')
            source.write_text(code)
            inputs.append((source, 'control:'+label, expected))
        jobs = [(source,label,expected,mode,opt) for source,label,expected in inputs
                for mode in ('default','checked','unchecked') for opt in ('-O0','-O1','-O2')]
        settings = argparse.Namespace(c_ldlibs=None, compile_timeout=120, run_timeout=30)

        def verify(job):
            index,(source,label,expected,mode,opt) = job
            case = compare_case(fixtures.COMPILER,source,index,mode,opt,str(self.root),settings)
            case['source'] = label
            if os.name == 'nt':
                expected = expected.replace(b'\n',b'\r\n')
            for result in case['targets'].values():
                self.assertEqual(result['compile']['status'], 0, result['compile'])
                self.assertEqual((result['run']['status'], result['run']['stdout_hex'], result['run']['stderr_hex']),
                                 (0, expected.hex(), ''))
            self.assertTrue(case['passed'], case)
            case['expected_stdout_hex'] = expected.hex()
            return case

        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS','2'))) as workers:
            cases = list(workers.map(verify,enumerate(jobs)))
        self.assertEqual(len(cases), 45)
        self.report('language', cases, control_sources=controls, original_source_sha256=digest(original))

    def configure(self, language, runtime):
        api = self.package/'src/api.sn'
        text = api.read_text().replace('static fn read(value: int)', 'static fn read(value: int as val)')
        text = text.replace('static fn make(value: int)', 'static fn make(value: int as val)')
        source = {'C': '#include <stdint.h>\n#include <stdbool.h>\n', 'RS': '', 'GO': 'package backing\n'}[language]
        manifest = self.manifest.replace('runtime: C', 'runtime: '+runtime)
        extension = {'C':'c','RS':'rs','GO':'go'}[language]
        provider = 'foreign-go/provider.go' if language=='GO' else 'src/provider.'+extension
        options = '      entry: src/provider.rs\n' if language=='RS' else ''
        if language=='GO':
            options = '      module: foreign-go\n'
            self.write('.sn/methods/foreign-go/go.mod', 'module sindarin.test/scalar-values\n\ngo 1.26.0\n')
        manifest = manifest.replace('  bindings:\n',
            '    - name: foreign\n      language: '+language+'\n      sources: ['+provider+']\n'+options+'  bindings:\n')
        main = (self.root/'main.sn').read_text()
        expected = b'12\n9\n19\ntrue\n1\n'
        for kind,c_type,rust_type,go_type,initial,result,reference_result in SCALARS:
            # The first alias intentionally names a different public callable.
            alias = 'foreign_int32' if kind == 'int' else 'original_'+kind
            text += f'@alias "{alias}"\nnative fn foreign_{kind}(value: {kind} as val): {kind}\n'
            update = ('value = !value' if kind == 'bool' else 'value++' if kind == 'char'
                      else 'value += '+('1.0f' if kind == 'float' else '1.0' if kind == 'double' else '1'))
            body = (f'  var pending: int = & foreign_int(value)\n  return pending!\n' if kind == 'int'
                    else f'  return foreign_{kind}(value)\n')
            text += f'fn value_{kind}(value: {kind} as val): {kind} =>\n  {update}\n'+body
            expression = '!value' if kind == 'bool' else 'value+1' if kind == 'char' else 'value+10'
            provider_function = ('Provider_' if language == 'GO' else 'provider_')+kind
            if language == 'C':
                source += f'{c_type} provider_{kind}({c_type} value) {{ return {expression}; }}\n'
            elif language == 'RS':
                rust_expression = 'value+10.0' if kind in ('float','double') else expression
                source += f'pub fn provider_{kind}(value: {rust_type}) -> {rust_type} {{ {rust_expression} }}\n'
            else:
                source += f'func {provider_function}(value {go_type}) {go_type} {{ return {expression} }}\n'
            for name, function, unit in ((f'foreign_{kind}',provider_function,'foreign'),
                                          (f'value_{kind}',f'value_{kind}','facade')):
                manifest += self.binding(name,function,unit,'value','value: value')
            main += (f'  var original_{kind}: {kind} = {initial}\n'
                     f'  println(value_{kind}(original_{kind}))\n'
                     f'  println(original_{kind} == {initial})\n'
                     f'  var callback_{kind} = foreign_{kind}\n'
                     f'  println(callback_{kind}(original_{kind}))\n'
                     f'  println(original_{kind} == {initial})\n')
            expected += (result+'\ntrue\n'+reference_result+'\ntrue\n').encode()
        api.write_text(text)
        self.write('.sn/methods/'+provider, source)
        self.write('.sn/methods/sn.yaml', manifest)
        self.write('main.sn', main)
        return manifest, expected

    def test_package_bodies_providers_references_methods_and_sealed_values(self):
        baseline = {path: path.read_bytes() for path in (self.package/'src').iterdir()}
        main = (self.root/'main.sn').read_bytes()
        cases = []
        for language in ('C','RS','GO'):
            for runtime in ('C','RS'):
                for path in (self.package/'src').iterdir():
                    if path.name.startswith('provider.'):
                        path.unlink()
                for path,data in baseline.items():
                    path.write_bytes(data)
                (self.root/'main.sn').write_bytes(main)
                manifest, expected = self.configure(language,runtime)
                paths = list((self.package/'src').iterdir())
                if (self.package/'foreign-go').exists():
                    paths += list((self.package/'foreign-go').iterdir())
                frozen = {path:path.read_bytes() for path in paths}
                built = subprocess.run([str(fixtures.COMPILER),'--build-native',str(self.package/'sn.yaml'),
                    '-o',str(self.package/('built-'+language+'-'+runtime))],cwd=self.root,capture_output=True,timeout=240)
                self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                assembly = Path(json.loads(built.stdout)['assembly'])
                metadata = json.loads(assembly.read_text())
                selected = [s for s in metadata['provider_signatures'] if
                            s['binding']['declaration'].split('::')[1].startswith(('foreign_','value_'))]
                self.assertEqual(len(selected),20)
                self.assertTrue(all(s['params'][0]['mem_qual']=='as_val' for s in selected))
                for sealed in (False,True):
                    if sealed:
                        destination = self.package/('sealed-'+language+'-'+runtime)
                        shutil.copytree(assembly.parent,destination)
                        descriptor = destination/assembly.name
                        digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                        self.write('.sn/methods/sn.yaml',manifest.replace('native:\n',
                            f'native:\n  assembly: {{path: {destination.name}/{assembly.name}, sha256: {digest}}}\n'))
                        for path in frozen:
                            if path.suffix in ('.c','.rs','.go'):
                                path.unlink()
                    for target in ('c','rust'):
                        for optimization in ('-O0','-O1','-O2'):
                            for mode in ('default','checked','unchecked'):
                                with self.subTest(language=language,runtime=runtime,sealed=sealed,target=target,
                                                  optimization=optimization,mode=mode):
                                    exe = self.root/'consumer.exe'
                                    built = subprocess.run([str(fixtures.COMPILER),'main.sn','--no-install',
                                        '--target',target,optimization,*([] if mode=='default' else ['--'+mode]),
                                        '-o',str(exe)],cwd=self.root,capture_output=True,timeout=180)
                                    self.assertEqual(built.returncode,0,built.stderr.decode(errors='replace'))
                                    run = subprocess.run([str(exe)],cwd=self.root,capture_output=True,timeout=30)
                                    output = expected.replace(b'\n',b'\r\n') if os.name=='nt' else expected
                                    self.assertEqual((run.returncode,run.stdout,run.stderr),(0,output,b''))
                                    cases.append({'language':language,'runtime':runtime,'sealed':sealed,'target':target,
                                        'optimization':optimization,'mode':mode,'compile_status':built.returncode,
                                        'run_status':run.returncode,'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex()})
                    if sealed:
                        for path,data in frozen.items():
                            path.write_bytes(data)
                for path,data in frozen.items():
                    self.assertEqual(path.read_bytes(),data)
        self.assertEqual(len(cases),216)
        self.report('packages',cases)


if __name__ == '__main__':
    unittest.main(verbosity=2)
