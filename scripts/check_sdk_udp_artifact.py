#!/usr/bin/env python3
"""Consume unchanged UDP API and native state/array lifetimes from canonical SDK artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

from build_sdk_package import checked, prepare
from native_array_field_contexts import FIELD_CASES, combined_controls

ROOT = Path(__file__).resolve().parents[1]


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases = []
    public = {p.relative_to(sdk).as_posix(): p.read_bytes() for p in (sdk / 'src').rglob('*.sn')}
    sources = {}
    with tempfile.TemporaryDirectory(prefix='sn-sdk-udp-artifacts-') as folder:
        app = Path(folder)
        package = app / '.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('net/udp',))
        built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        assert len(metadata['units']) == 2 and {u['language'] for u in metadata['units']} == {'C', 'SN'}
        signatures = metadata['provider_signatures']
        assert len(signatures) == 12
        control = 'import "sindarin-pkg-sdk/src/net/udp"\nnative fn noResult(): UdpReceiveResult =>\n  return nil\nnative fn noSocket(): UdpSocket =>\n  return nil\nnative fn makeResult(): UdpReceiveResult =>\n  return UdpReceiveResult { _data: {1b,2b}, _sender: "before" }\nfn heldResult(): UdpReceiveResult =>\n  var server: UdpSocket = UdpSocket.bind(":0")\n  var client: UdpSocket = UdpSocket.bind(":0")\n  client.sendTo("owned".toBytes(), $"127.0.0.1:{server.port()}")\n  var result: UdpReceiveResult = server.receiveFrom(32)\n  client.dispose()\n  server.dispose()\n  return result\nfn heldBytes(): byte[] =>\n  var result: UdpReceiveResult = heldResult()\n  return result.data()\nfn heldSender(): str =>\n  var result: UdpReceiveResult = heldResult()\n  return result.sender()\nfn main(): void =>\n  var missing: UdpReceiveResult = noResult()\n  println(missing.data().length == 0)\n  println(missing.sender() == "")\n  var socket: UdpSocket = noSocket()\n  println(socket.port() == 0)\n  println(socket.sendTo({1b}, "ignored") == 0)\n  var empty: UdpReceiveResult = socket.receiveFrom(0)\n  println(empty.data().length == 0 && empty.sender() == "")\n  println(empty._data.length == 0 && empty._sender == "")\n  var result: UdpReceiveResult = makeResult()\n  var alias: UdpReceiveResult = result\n  println(alias._sender == "before")\n  alias._sender = "after"\n  println(result.sender() == "after")\n  result._data.push(3b)\n  println(alias.data().length == 3)\n  var snapshot: byte[] = alias.data()\n  alias._data.clear()\n  println(snapshot.length == 3 && result._data.length == 0)\n  println(heldBytes().toString() == "owned")\n  println(heldSender().startsWith("127.0.0.1:"))\n'
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
            for name in ('udp', 'native-lifetime', 'array-field-contexts'):
                if name == 'native-lifetime':
                    source, expected = control.encode(), b'true\n'*12
                elif name == 'array-field-contexts':
                    source, expected = combined_controls()
                    source = source.encode()
                else:
                    path = sdk / 'tests/net' / ('test_' + name + '.sn')
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
        'array_field_controls': [name for name, _, _ in FIELD_CASES],
        'units': len(metadata['units']), 'exports': len(signatures), 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
        'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'field_controls_script_sha256': hashlib.sha256((ROOT / 'scripts/native_array_field_contexts.py').read_bytes()).hexdigest(),
        'environment': {name: os.environ.get(name) for name in
                        ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS',
                         'ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / f'.sn/sdk-udp-artifact{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(cases)} canonical UDP source/sealed C/Rust cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
