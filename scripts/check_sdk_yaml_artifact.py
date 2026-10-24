#!/usr/bin/env python3
"""Consume unchanged Yaml API and native state/array lifetimes from canonical SDK artifacts."""
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
    with tempfile.TemporaryDirectory(prefix='sn-sdk-yaml-artifacts-') as folder:
        app = Path(folder)
        package = app / '.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('encoding/yaml', 'os/env'))
        built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        assert len(metadata['units']) == 4 and {u['language'] for u in metadata['units']} == {'C', 'SN'}
        signatures = metadata['provider_signatures']
        assert len(signatures) == 54
        control = 'import "sindarin-pkg-sdk/src/encoding/yaml"\nfn ownedChild(): Yaml =>\n  var parent: Yaml = Yaml.mapping()\n  parent.set("child", Yaml.scalar("alive"))\n  return parent.get("child")\nfn savedText(): str =>\n  var parent: Yaml = Yaml.scalar("owned")\n  return parent.value()\nfn copied(item: Yaml as val): Yaml =>\n  item.set("count", Yaml.scalar("19"))\n  return item\nfn nilSnapshot(item: Yaml): bool =>\n  var value: Yaml = copyOf(item)\n  return value!=nil && value.value()==""\nfn nilValue(item: Yaml as val): bool =>\n  return item==nil\nfn variableNil(item: Yaml): bool =>\n  return !nilValue(item)\nfn main(): void =>\n  var parent: Yaml = Yaml.mapping()\n  parent.set("child", Yaml.mapping())\n  var alias: Yaml = parent\n  var child: Yaml = parent.get("child")\n  child.set("count", Yaml.scalar("7"))\n  println(alias.get("child").get("count").asInt()==7)\n  var copy: Yaml = parent.copy()\n  copy.get("child").set("count", Yaml.scalar("19"))\n  println(child.get("count").asInt()==7)\n  println(copy.get("child").get("count").asInt()==19)\n  parent.remove("child")\n  alias.dispose()\n  alias.dispose()\n  println(child.get("count").asInt()==7)\n  println(ownedChild().value()=="alive")\n  println(savedText()=="owned")\n  var original: Yaml = Yaml.mapping()\n  original.set("count", Yaml.scalar("7"))\n  var snapshot: Yaml = copied(original)\n  println(original.get("count").asInt()==7)\n  println(snapshot.get("count").asInt()==19)\n  var explicitCopy: Yaml = copyOf(original)\n  explicitCopy.set("count", Yaml.scalar("23"))\n  println(original.get("count").asInt()==7)\n  println(explicitCopy.get("count").asInt()==23)\n  var indirect = copied\n  var indirectCopy: Yaml = indirect(original)\n  println(indirectCopy.get("count").asInt()==19)\n  println(nilSnapshot(nil))\n  println(nilValue(nil))\n  var nilCall = nilValue\n  println(nilCall(nil))\n  println(variableNil(nil))\n'
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
            for name in ('yaml', 'native-lifetime'):
                if name == 'native-lifetime':
                    source, expected = control.encode(), b'true\n'*15
                else:
                    path = sdk / 'tests/encoding' / ('test_' + name + '.sn')
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
                        compiled = checked(command, app)
                        run = checked([exe], app)
                        if run.stdout != expected or run.stderr:
                            raise ValueError(f'{name}/{artifact}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append({'case': name, 'artifact': artifact, 'target': target,
                            'opt': opt, 'mode': mode, 'passed': True,
                            'source_sha256': hashlib.sha256(source).hexdigest(),
                            'compile_status': compiled.returncode, 'run_status': run.returncode,
                            'stdout_hex': run.stdout.hex(), 'stderr_hex': run.stderr.hex(),
                            'expected_stdout_hex': expected.hex()})
            assert all((package / path).read_bytes() == data for path, data in public.items())
    report = {'passed': True, 'cases': cases, 'original_sources': sources, 'unchanged_public_modules': len(public),
        'units': len(metadata['units']), 'exports': len(signatures), 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
        'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'environment': {name: os.environ.get(name) for name in
                        ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS',
                         'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / f'.sn/sdk-yaml-artifact{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(cases)} canonical Yaml source/sealed C/Rust cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
