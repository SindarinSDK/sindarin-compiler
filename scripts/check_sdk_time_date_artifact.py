#!/usr/bin/env python3
"""Consume unchanged Time/Date APIs and owned opaque results from canonical SDK artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

from build_sdk_package import checked, prepare

ROOT = Path(__file__).resolve().parents[1]


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases = []
    public = {p.relative_to(sdk).as_posix(): p.read_bytes() for p in (sdk / 'src').rglob('*.sn')}
    sources = {}
    with tempfile.TemporaryDirectory(prefix='sn-sdk-time-date-artifacts-') as folder:
        app = Path(folder)
        package = app / '.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('time/date',))
        built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        assert len(metadata['units']) == 4 and {u['language'] for u in metadata['units']} == {'C', 'SN'}
        signatures = metadata['provider_signatures']
        assert {s['binding']['method'] for s in signatures if s['binding']['declaration'].endswith('::Date.isLeapYear')} == {'static', 'instance'}
        opaque = [s for s in signatures if s['binding'].get('result_type')]
        assert {s['binding']['declaration'] for s in opaque} == {'src/time/date.sn::Date.toTime', 'src/time/date.sn::sn_date_to_time'}
        (app / 'consume.c').write_text(
            '#undef NDEBUG\n#include <assert.h>\n#include ".sn/sindarin-pkg-sdk/src/time/time.native.h"\n'
            'long long consume_sdk_time(void *p) { SnSdkTimeRecord *t=p;assert(t && sn_sdk_time_refs(t)==1);'
            'long long value=t->milliseconds;sn_sdk_time_release(t);return value; }\n', encoding='utf-8')
        control = (
            'import "sindarin-pkg-sdk/src/time/date"\n@source "consume.c"\n'
            '@alias "consume_sdk_time"\nnative fn consumeTime(p: *void): int\n'
            'native fn makePointer(): *void =>\n  var date: Date = Date.fromEpochDays(2)\n'
            '  var alias: Date = date\n  alias._days=3\n  return date.toTime()\n'
            'native fn exercise(): void =>\n  var pointer: *void = makePointer()\n'
            '  println(consumeTime(pointer))\n  println(sn_date_to_time(nil)==nil)\n'
            'fn main(): void =>\n  exercise()\n')
        modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, package / 'sealed')
                descriptor = package / 'sealed' / assembly.name
                digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                manifest.write_text(manifest.read_text().replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                for directory in (package / 'src', package / 'native'):
                    for path in directory.rglob('*.c'):
                        path.unlink()
                assert not any(path for directory in (package / 'src', package / 'native') for path in directory.rglob('*.c'))
            for name in ('time', 'date', 'opaque-result'):
                if name == 'opaque-result':
                    source, expected = control.encode(), b'259200000\ntrue\n'
                else:
                    path = sdk / 'tests/time' / ('test_' + name + '.sn')
                    oracle = path.with_suffix('.expected')
                    source, expected = path.read_bytes(), oracle.read_bytes().replace(b'\r\n', b'\n')
                    sources[path.relative_to(sdk).as_posix()] = hashlib.sha256(source).hexdigest()
                    sources[oracle.relative_to(sdk).as_posix()] = hashlib.sha256(oracle.read_bytes()).hexdigest()
                (app / 'main.sn').write_bytes(source)
                if os.name == 'nt':
                    expected = expected.replace(b'\n', b'\r\n')
                for target in ('c', 'rust'):
                    for opt, mode in modes:
                        exe = app / 'consumer.exe'
                        command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
                        if mode != 'default':
                            command.append('--' + mode)
                        checked(command, app)
                        run = checked([exe], app)
                        if run.stdout != expected or run.stderr:
                            raise ValueError(f'{name}/{artifact}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append({'case': name, 'artifact': artifact, 'target': target, 'opt': opt, 'mode': mode, 'passed': True})
            assert all((package / path).read_bytes() == data for path, data in public.items())
    report = {'passed': True, 'cases': cases, 'original_sources': sources, 'unchanged_public_modules': len(public),
        'units': len(metadata['units']), 'exports': len(signatures), 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest()}
    output = ROOT / '.sn/sdk-time-date-artifact.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(cases)} canonical Time/Date source/sealed C/Rust cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
