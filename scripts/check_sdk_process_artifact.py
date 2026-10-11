#!/usr/bin/env python3
"""Consume unchanged Process API and native state/array lifetimes from canonical SDK artifacts."""
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
    with tempfile.TemporaryDirectory(prefix='sn-sdk-process-artifacts-') as folder:
        app = Path(folder)
        package = app / '.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('os/process','os/env'))
        built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        assert len(metadata['units']) == 4 and {u['language'] for u in metadata['units']} == {'C', 'SN'}
        signatures = metadata['provider_signatures']
        assert len(signatures) == 19
        control = 'import "sindarin-pkg-sdk/src/os/process"\nimport "sindarin-pkg-sdk/src/os/env"\nfn process(): Process =>\n  if Environment.getOr("COMSPEC", "").length>0 =>\n    return Process.runArgs("cmd", {"/c","echo hello"})\n  return Process.runArgs("echo", {"hello"})\nfn text(): str =>\n  var result: Process = process()\n  return result.stdout()\nfn main(): void =>\n  var original: Process = process()\n  var alias: Process = original\n  println(alias._exitCode==0)\n  println(original._stdout.contains("hello"))\n  alias._exitCode=7\n  println(original.exitCode()==7)\n  var saved: str = alias.stdout()\n  println(saved.contains("hello"))\n  println(original._stderr.length==0)\n  println(text().contains("hello"))\n'
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
            for name in ('process', 'native-lifetime'):
                if name == 'native-lifetime':
                    source, expected = control.encode(), b'true\ntrue\ntrue\ntrue\ntrue\ntrue\n'
                else:
                    path = sdk / 'tests/os' / ('test_' + name + '.sn')
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
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / f'.sn/sdk-process-artifact{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(cases)} canonical Process source/sealed C/Rust cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
