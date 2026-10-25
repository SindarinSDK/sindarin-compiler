#!/usr/bin/env python3
"""Verify unchanged TCP callers and resource/alias contracts through C artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from build_sdk_package import checked, prepare
from tcp_artifact_controls import BACKING, CONTROLS, rejected_copy

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases, rejections = [], []
    public = {p.relative_to(sdk).as_posix(): p.read_bytes()
              for p in (sdk / 'src').rglob('*.sn')}
    original = sdk / 'tests/net/test_tcp.sn'
    oracle = original.with_suffix('.expected')
    sources = {p.relative_to(sdk).as_posix(): digest(p.read_bytes())
               for p in (original, oracle)}
    functions = tuple(re.findall(r'^fn (\w+)\(', original.read_text(), re.M))
    programs = [('original', original.read_bytes(),
                 oracle.read_bytes().replace(b'\r\n', b'\n'), functions)]
    programs += [(name, source.encode(), b'true\n' * source.count('\n  println('), required)
                 for name, source, required in CONTROLS]
    modes = [('-O0', 'default')] if quick else [
        (opt, mode) for opt in ('-O0', '-O1', '-O2')
        for mode in ('default', 'checked', 'unchecked')]
    with tempfile.TemporaryDirectory(prefix='sn-sdk-tcp-artifacts-') as folder:
        app = Path(folder)
        package = app / '.sn/sindarin-pkg-sdk'
        manifest = prepare(compiler, sdk, package, ('net/tcp',))
        built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
        assembly = Path(json.loads(built.stdout)['assembly'])
        metadata = json.loads(assembly.read_text())
        assert len(metadata['units']) == 2
        assert {unit['language'] for unit in metadata['units']} == {'C', 'SN'}
        assert len(metadata['provider_signatures']) == 25
        # The original test imports ../../src/net/tcp. Preserve its complete
        # relative path and bytes inside the package rather than rewriting it.
        entry = package / 'tests/net/test_tcp.sn'
        entry.parent.mkdir(parents=True)
        entry.write_bytes(original.read_bytes())
        (app / 'sn.yaml').write_text('name: tcp-client\n')
        (app / 'control.sn.c').write_text(BACKING)
        for artifact in ('source', 'sealed'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent, package / 'sealed')
                descriptor = package / 'sealed' / assembly.name
                manifest.write_text(manifest.read_text().replace('native:\n',
                    f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest(descriptor.read_bytes())}}}\n'))
                for path in package.rglob('*.c'):
                    path.unlink()
                assert not any(package.rglob('*.c'))
            for name, source, expected, required in programs:
                path = entry if name == 'original' else app / 'control.sn'
                path.write_bytes(source)
                if os.name == 'nt':
                    expected = expected.replace(b'\n', b'\r\n')
                for target in ('c', 'rust'):
                    for opt, mode in modes:
                        label = f'{name}/{artifact}/{target}/{opt}/{mode}'
                        print('RUN ' + label, flush=True)
                        # Keep only this invocation's Rust output. A process ID
                        # can be reused, so a set difference of paths is unsafe.
                        rust_build = app / '.sn/build/rust'
                        if target == 'rust' and rust_build.exists():
                            shutil.rmtree(rust_build)
                        exe = app / 'consumer.exe'
                        command = [compiler, path, '--no-install', '--keep-generated',
                                   '--target', target, opt, '-o', exe]
                        if mode != 'default':
                            command.append('--' + mode)
                        compiled = checked(command, app)
                        proof = None
                        if target == 'rust':
                            generated = list(rust_build.rglob('main.rs'))
                            if len(generated) != 1:
                                raise ValueError(f'{name}: expected one generated Rust caller, found {generated}')
                            text = generated[0].read_text()
                            if not all(re.search(r'\bfn ' + re.escape(fn) + r'\(', text)
                                       for fn in required):
                                raise ValueError(f'{name}: caller functions missing from generated Rust')
                            proof = {'sha256': digest(generated[0].read_bytes()),
                                     'caller_functions': list(required)}
                        try:
                            run = subprocess.run([str(exe)], cwd=app, capture_output=True, timeout=30)
                        except subprocess.TimeoutExpired as error:
                            raise ValueError(f'{label}: execution timeout; stdout={error.stdout!r}, stderr={error.stderr!r}') from error
                        if run.returncode or run.stdout != expected or run.stderr:
                            raise ValueError(f'{label}: status={run.returncode} {run.stdout!r} {run.stderr!r}')
                        cases.append(dict(case=name, artifact=artifact, target=target,
                            opt=opt, mode=mode, passed=True, source_sha256=digest(source),
                            compile_status=compiled.returncode, run_status=run.returncode,
                            stdout_hex=run.stdout.hex(), stderr_hex=run.stderr.hex(),
                            expected_stdout_hex=expected.hex(), rust_caller=proof))
            # Legacy C has no TCP copy hooks either. Rejection is a contract
            # check, not a passing language parity or lifetime case.
            for record in ('TcpStream', 'TcpListener'):
                for operation in ('as-val', 'copyOf'):
                    source = rejected_copy(record, operation).encode()
                    (app / 'control.sn').write_bytes(source)
                    for target in ('c', 'rust'):
                        exe = app / 'rejected.exe'
                        exe.unlink(missing_ok=True)
                        run = subprocess.run([str(compiler), 'control.sn', '--no-install',
                            '--target', target, '-O0', '-o', str(exe)], cwd=app,
                            capture_output=True, timeout=180)
                        message = (b'as-val copy' if operation == 'as-val' else b'copyOf()')
                        message += b' requires a native.types copy contract'
                        if run.returncode == 0 or message not in run.stderr or exe.exists():
                            raise ValueError(f'{record}/{operation}/{target}: expected missing-copy-contract error: {run.stderr!r}')
                        rejections.append(dict(record=record, operation=operation,
                            artifact=artifact, target=target, passed=True,
                            compile_status=run.returncode, source_sha256=digest(source),
                            stderr_hex=run.stderr.hex()))
            assert entry.read_bytes() == original.read_bytes()
            assert all((package / path).read_bytes() == data for path, data in public.items())
    report = dict(passed=True, cases=cases, contract_rejections=rejections,
        original_sources=sources, public_source_sha256={name: digest(data) for name, data in public.items()},
        public_sources_preserved=True, original_relative_import_preserved=True,
        backing_sources_removed_for_sealed=True, units=len(metadata['units']),
        exports=len(metadata['provider_signatures']), compiler_sha256=digest(compiler.read_bytes()),
        oracle_script_sha256=digest(Path(__file__).read_bytes()),
        controls_script_sha256=digest((ROOT / 'scripts/tcp_artifact_controls.py').read_bytes()),
        backing_control_sha256=digest(BACKING.encode()),
        environment={name: os.environ.get(name) for name in
            ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')})
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / f'.sn/sdk-tcp-artifact{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {len(cases)} TCP source/sealed C/Rust cases; {len(rejections)} separate copy-contract rejections')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
