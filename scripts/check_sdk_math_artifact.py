#!/usr/bin/env python3
"""Verify every unchanged Math export through source and sealed C SDK artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

from build_sdk_package import prepare, checked

ROOT = Path(__file__).resolve().parents[1]
NATIVE = {
    'sin': ((0.5235987755982988,), 0.5), 'cos': ((0,), 1),
    'tan': ((0.7853981633974483,), 1), 'asin': ((0.5,), 0.5235987755982988),
    'acos': ((0.5,), 1.0471975511965976), 'atan': ((1,), 0.7853981633974483),
    'atan2': ((1, 1), 0.7853981633974483),
    'sinh': ((1,), 1.1752011936438014), 'cosh': ((1,), 1.5430806348152437),
    'tanh': ((1,), 0.7615941559557649), 'asinh': ((1,), 0.881373587019543),
    'acosh': ((2,), 1.3169578969248166), 'atanh': ((0.5,), 0.5493061443340548),
    'exp': ((1,), 2.718281828459045), 'exp2': ((3,), 8),
    'log': ((2.718281828459045,), 1), 'log2': ((8,), 3), 'log10': ((100,), 2),
    'log1p': ((1,), 0.6931471805599453), 'expm1': ((1,), 1.718281828459045),
    'pow': ((2, 3), 8), 'sqrt': ((4,), 2), 'cbrt': ((8,), 2), 'hypot': ((3, 4), 5),
    'ceil': ((1.2,), 2), 'floor': ((1.8,), 1), 'trunc': ((-1.8,), -1),
    'round': ((1.6,), 2), 'fmod': ((5, 2), 1), 'remainder': ((5, 2), 1),
    'fabs': ((-2,), 2), 'copysign': ((2, -1), -2), 'fdim': ((5, 2), 3),
    'fmax': ((2, 3), 3), 'fmin': ((2, 3), 2),
}
CONSTANTS = {'pi': 3.141592653589793, 'e': 2.718281828459045,
             'tau': 6.283185307179586, 'phi': 1.618033988749895,
             'sqrt2': 1.4142135623730951, 'sqrt3': 1.7320508075688772,
             'ln2': 0.6931471805599453, 'ln10': 2.302585092994046}
HELPERS = {
    'absDouble': ((-3,), 3), 'minDouble': ((2, 3), 2), 'maxDouble': ((2, 3), 3),
    'clampDouble': ((5, 0, 3), 3), 'signDouble': ((-3,), -1),
    'degToRad': ((180,), 3.141592653589793), 'radToDeg': ((1.5707963267948966,), 90),
    'safeSqrt': ((-1,), 0), 'safeLog': ((-1,), 0), 'safeLog10': ((-1,), 0),
    'safeLog2': ((-1,), 0), 'safeAsin': ((2,), 1.5707963267948966),
    'safeAcos': ((-2,), 3.141592653589793), 'safeDiv': ((2, 0), 0),
    'safeAcosh': ((0,), 0), 'safeAtanh': ((2,), 0),
    'lerp': ((0, 10, 0.5), 5), 'invLerp': ((0, 10, 5), 0.5),
    'remap': ((0.5, 0, 1, 0, 10), 5), 'approxEq': ((1, 1.000001, 0.01), True),
    'wrapAngle': ((0,), 0), 'wrapAngleSigned': ((0,), 0),
    'distance2D': ((0, 0, 3, 4), 5), 'distance3D': ((0, 0, 0, 2, 3, 6), 7),
    'smoothstep': ((0, 1, 0.5), 0.5), 'isNan': ((0,), False), 'isFinite': ((1,), True),
}
INTEGERS = {'absInt': ((-3,), 3), 'minInt': ((2, 3), 2), 'maxInt': ((2, 3), 3),
            'clampInt': ((5, 0, 3), 3), 'signInt': ((-3,), -1)}
FLOAT_HELPERS = {'absDouble': 'absF', 'minDouble': 'minF', 'maxDouble': 'maxF',
                 'clampDouble': 'clampF', 'signDouble': 'signF'}


def literal(value, kind):
    if kind in ('int', 'long', 'int32', 'uint', 'uint32', 'byte'): return str(int(value))
    text = format(value, '.15f').rstrip('0').rstrip('.')
    if '.' not in text:text += '.0'
    return text + ('f' if kind == 'float' else '')


def all_export_control(model):
    probes = dict(NATIVE)
    probes.update({name: ((), value) for name, value in CONSTANTS.items()})
    probes.update(HELPERS)
    for name, probe in list(probes.items()):
        probes[FLOAT_HELPERS.get(name, name+'F')] = probe
    probes.update(INTEGERS)
    functions = {function['name']: function for function in model['functions']}
    if len(functions) != 145 or set(probes) != set(functions):
        raise ValueError('Math API probe set differs from all original declarations')
    lines = ['import "sindarin-pkg-sdk/src/core/math" as math', 'fn main(): void =>']
    for index, (name, (arguments, expected)) in enumerate(probes.items()):
        function = functions[name]
        if len(arguments) != len(function['params']):raise ValueError('Math arity changed: '+name)
        values = ', '.join(literal(value, parameter['type']['kind'])
                           for value, parameter in zip(arguments, function['params']))
        result = function['return_type']['kind']
        lines.append(f'  var result{index}: {result} = math.{name}({values})')
        if result == 'bool':
            condition = f'result{index} == '+('true' if expected else 'false')
        elif result == 'int':
            condition = f'result{index} == {int(expected)}'
        else:
            epsilon = 0.0001 if result == 'float' else 0.000000001
            low, high = literal(expected-epsilon, result), literal(expected+epsilon, result)
            condition = f'result{index} >= {low} && result{index} <= {high}'
        lines.append(f'  println({condition})')
    return '\n'.join(lines)+'\n', list(probes)


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases, sources = [], {}
    public = {p.relative_to(sdk).as_posix(): p.read_bytes() for p in (sdk/'src').rglob('*.sn')}
    with tempfile.TemporaryDirectory(prefix='sn-sdk-math-') as folder:
        app = Path(folder)
        package = app/'.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('core/math',))
        model = json.loads((package/'native/core_math.model.json').read_text())
        control, covered = all_export_control(model)
        built = checked([compiler, '--build-native', manifest, '-o', package/'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        if len(metadata['units']) != 2 or len(metadata['provider_signatures']) != 145:
            raise ValueError('Math package omits a declared function')
        native = sum(bool(f['is_native']) for f in model['functions'])
        if native != 70:raise ValueError('Math original native function inventory changed')
        modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2')
                                                   for m in ('default', 'checked', 'unchecked')]
        original = sdk/'tests/core/test_math.sn'
        oracle = original.with_suffix('.expected')
        for source in (original, oracle):sources[source.relative_to(sdk).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, package/'sealed')
                descriptor = package/'sealed'/assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                manifest.write_text(manifest.read_text().replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                for directory in (package/'src', package/'native'):
                    for path in directory.rglob('*.c'):path.unlink()
                if any(path for directory in (package/'src', package/'native') for path in directory.rglob('*.c')):
                    raise ValueError('sealed Math package retains backing C')
            for name, source, expected in [('original', original.read_bytes(), oracle.read_bytes().replace(b'\r\n', b'\n')),
                                           ('all-exports', control.encode(), b'true\n'*145)]:
                (app/'main.sn').write_bytes(source)
                if os.name == 'nt':expected = expected.replace(b'\n', b'\r\n')
                for target in ('c', 'rust'):
                    for optimization, mode in modes:
                        executable = app/'client.exe'
                        command = [compiler, 'main.sn', '--no-install', '--target', target,
                                   optimization, '-o', executable]
                        if mode != 'default':command.append('--'+mode)
                        compiled = checked(command, app)
                        result = checked([executable], app)
                        if result.stdout != expected or result.stderr:
                            raise ValueError(f'{name}/{artifact}/{target}/{optimization}/{mode}: {result.stdout!r} {result.stderr!r}')
                        cases.append({'case': name, 'artifact': artifact, 'target': target,
                            'opt': optimization, 'mode': mode, 'compile_status': compiled.returncode,
                            'run_status': result.returncode, 'source_sha256': hashlib.sha256(source).hexdigest(),
                            'stdout_hex': result.stdout.hex(), 'stderr_hex': result.stderr.hex(),
                            'expected_stdout_hex': expected.hex(), 'passed': True})
            if any((package/path).read_bytes() != data for path, data in public.items()):
                raise ValueError('staged public Math SDK source changed')
    report = {'passed': True, 'cases': cases, 'units': 2, 'exports': 145,
        'original_native_functions': native, 'original_Sindarin_functions': 145-native,
        'all_export_probes': covered, 'control_source': control,
        'original_source_sha256': sources, 'public_source_sha256': {
            name: hashlib.sha256(data).hexdigest() for name, data in public.items()},
        'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
        'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'environment': {name: os.environ.get(name) for name in
                        ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS',
                         'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT/'.sn'/f'sdk-math-artifact{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2)+'\n')
    print(f'PASS: {len(cases)} Math artifact cases, all 145 public exports covered')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT/'.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
