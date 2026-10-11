#!/usr/bin/env python3
"""Prove SDK as-val copies isolate native buffers, captured strings and JSON trees."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

from build_sdk_package import checked, prepare

ROOT = Path(__file__).resolve().parents[1]
CONTROLS = {
    'encoding/text': '''import "sindarin-pkg-sdk/src/encoding/text"
fn copied(value: StringBuilder as val): StringBuilder =>
  value.append(" copy")
  return value
fn main(): void =>
  var original: StringBuilder = StringBuilder.createWithCapacity(2)
  original.append("saved")
  var snapshot: StringBuilder = copied(original)
  println(original.toString()=="saved")
  println(snapshot.toString()=="saved copy")
  println(original._buf!=snapshot._buf)
  var call = copied
  var indirect: StringBuilder = call(original)
  println(indirect.toString()=="saved copy")
  original.dispose()
  println(snapshot.toString()=="saved copy")
''',
    'encoding/json': '''import "sindarin-pkg-sdk/src/encoding/json"
fn copied(value: Json as val): Json =>
  value.get("child").set("count", Json.ofInt(19))
  return value
fn main(): void =>
  var original: Json = Json.object()
  original.set("child", Json.object())
  original.get("child").set("count", Json.ofInt(7))
  var snapshot: Json = copied(original)
  println(original.get("child").get("count").asInt()==7)
  println(snapshot.get("child").get("count").asInt()==19)
  println(original._obj!=snapshot._obj)
  var call = copied
  var indirect: Json = call(original)
  println(indirect.get("child").get("count").asInt()==19)
  original.dispose()
  println(snapshot.get("child").get("count").asInt()==19)
''',
    'os/process': '''import "sindarin-pkg-sdk/src/os/process"
import "sindarin-pkg-sdk/src/os/env"
fn copied(value: Process as val): Process =>
  value._exitCode=7
  value._stdout="copy"
  return value
fn main(): void =>
  var shell: str = Environment.getOr("COMSPEC", "")
  var original: Process
  if shell=="" =>
    original=Process.runArgs("echo", {"saved"})
  else =>
    original=Process.runArgs(shell, {"/c", "echo", "saved"})
  var snapshot: Process = copied(original)
  println(original.exitCode()==0)
  println(snapshot.exitCode()==7)
  println(original.stdout().contains("saved"))
  var call = copied
  var indirect: Process = call(original)
  println(indirect.stdout()=="copy")
  println(snapshot.stdout()=="copy")
''',
}


# Managed native records must also copy through static and instance parameters.
for module, name in [('encoding/text', 'StringBuilder'), ('encoding/json', 'Json'), ('os/process', 'Process')]:
    methods = ('struct Calls as ref =>\n  value: int\n'
               f'  static fn snapshot(item: {name} as val): {name} =>\n    return item\n'
               f'  fn snapshotInstance(item: {name} as val): {name} =>\n    return item\n')
    control = CONTROLS[module].replace('fn copied(', methods + 'fn copied(', 1)
    location = '  var snapshot: ' + name + ' = copied(original)\n'
    probe = ('  var explicitCopy: ' + name + ' = copyOf(original)\n'
             '  var staticCopy: ' + name + ' = Calls.snapshot(original)\n'
             '  var calls: Calls = {value:0}\n'
             '  var instanceCopy: ' + name + ' = calls.snapshotInstance(original)\n')
    if module == 'encoding/text':
        probe += '  println(explicitCopy._buf!=original._buf)\n  println(staticCopy._buf!=original._buf)\n  println(instanceCopy._buf!=original._buf)\n'
    elif module == 'encoding/json':
        probe += '  println(explicitCopy._obj!=original._obj)\n  println(staticCopy._obj!=original._obj)\n  println(instanceCopy._obj!=original._obj)\n'
    else:
        probe += '  explicitCopy._stdout="explicit"\n  println(original.stdout().contains("saved"))\n  staticCopy._exitCode=9\n  println(original.exitCode()==0)\n  instanceCopy._stdout="method"\n  println(original.stdout().contains("saved"))\n'
    control = control.replace(location, probe + location, 1)
    if module == 'encoding/json':
        nil_functions = ('fn nilSnapshot(item: Json): bool =>\n  var value: Json = copyOf(item)\n  return value!=nil && value.isNull()\n'
                         'fn nilValue(item: Json as val): bool =>\n  return item==nil\n'
                         'fn variableNil(item: Json): bool =>\n  return !nilValue(item)\n')
        control = control.replace('fn main(): void =>', nil_functions + 'fn main(): void =>', 1)
        control += ('  println(nilSnapshot(nil))\n  println(nilValue(nil))\n'
                    '  var nilCall = nilValue\n  println(nilCall(nil))\n'
                    '  println(Calls.snapshot(nil)==nil)\n  println(calls.snapshotInstance(nil)==nil)\n'
                    '  println(variableNil(nil))\n')
    CONTROLS[module] = control


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases = []
    modes = [('-O0', 'default')] if quick else [(o, m) for o in ('-O0', '-O1', '-O2') for m in ('default', 'checked', 'unchecked')]
    for module, control in CONTROLS.items():
        with tempfile.TemporaryDirectory(prefix='sn-sdk-record-copy-') as folder:
            app = Path(folder)
            package = app / '.sn/sindarin-pkg-sdk'
            modules = (module, 'os/env') if module == 'os/process' else (module,)
            manifest = prepare(compiler, sdk, package, modules)
            built = checked([compiler, '--build-native', manifest, '-o', package / 'built'], app)
            assembly = Path(json.loads(built.stdout)['assembly'])
            metadata = json.loads(assembly.read_text())
            assert any(record.get('copy') for record in metadata['types'])
            (app / 'main.sn').write_text(control)
            expected = b'true\n' * (14 if module == 'encoding/json' else 8)
            if os.name == 'nt':
                expected = expected.replace(b'\n', b'\r\n')
            for artifact in ('source', 'sealed'):
                if artifact == 'sealed':
                    shutil.copytree(assembly.parent, package / 'sealed')
                    descriptor = package / 'sealed' / assembly.name
                    digest = hashlib.sha256(descriptor.read_bytes()).hexdigest()
                    manifest.write_text(manifest.read_text().replace('native:\n', f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                    for directory in (package / 'src', package / 'native'):
                        for path in directory.rglob('*.c'):
                            path.unlink()
                for target in ('c', 'rust'):
                    for opt, mode in modes:
                        exe = app / 'consumer.exe'
                        command = [compiler, 'main.sn', '--no-install', '--target', target, opt, '-o', exe]
                        if mode != 'default':
                            command.append('--' + mode)
                        try:
                            checked(command, app)
                        except ValueError as error:
                            raise ValueError(f"{module}/{artifact}/{target}/{opt}/{mode}: {error}") from error
                        run = checked([exe], app)
                        if run.stdout != expected or run.stderr:
                            raise ValueError(f'{module}/{artifact}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append(dict(module=module, artifact=artifact, target=target, opt=opt, mode=mode, passed=True))
    suffix = '-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS', '') else ''
    output = ROOT / f'.sn/sdk-record-copies{suffix}.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(dict(passed=True, cases=cases, compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest()), indent=2) + '\n')
    print(f'PASS: {len(cases)} SDK source/sealed C/Rust native record copy cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    verify(args.compiler, args.sdk, args.quick)
