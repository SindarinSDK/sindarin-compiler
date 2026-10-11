#!/usr/bin/env python3
"""Compare source native entry behavior with C across supported build modes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CASES = {
    'void_early': ('native fn main(): void =>\n  println("before")\n  return\n  println("after")\n', b'before\n', 0),
    'integer': ('native fn main(): int =>\n  println("native exit")\n  return 17\n', b'native exit\n', 17),
    'negative': ('native fn main(): int =>\n  return -1\n', b'', 0xffffffff if os.name == 'nt' else 255),
    'wide_exit': ('native fn main(): int =>\n  return 4294967297\n', b'', 1),
    'argv_integer': ('native fn main(args: str[]): int =>\n  println(args.length)\n  println(args[1])\n'
                     '  println(args[1])\n  println(args[2])\n  return 23\n', b'3\npayload\npayload\nwith space\n', 23),
    'argv_void': ('native fn main(args: str[]): void =>\n  println(args.length)\n  println(args[1])\n'
                  '  args.push("changed")\n  println(args.length)\n  println(args[1])\n  return\n', b'3\npayload\n4\npayload\n', 0),
    'initializer': ('var counter: int = 0\nvar label: str = initialize()\nfn initialize(): str =>\n'
                    '  counter += 1\n  return "initialized"\nnative fn main(): int =>\n'
                    '  println(counter)\n  println(label)\n  return 7\n', b'1\ninitialized\n', 7),
    'private_name_collision': ('fn __sn_native_entry_body_0(): int =>\n  return 31\nnative fn main(): int =>\n'
                               '  println(__sn_native_entry_body_0())\n  return 9\n', b'31\n', 9),
    'recursive_entry': ('var count: int = 0\nnative fn main(): int =>\n  count += 1\n'
                        '  if count < 3 =>\n    return main()\n  println(count)\n  return 5\n', b'3\n', 5),
    'record_field_borrows': ('struct EntryRecord as val =>\n  number: int32\n  label: str\n'
                            'native fn main(): void =>\n  var value: EntryRecord = EntryRecord {number: 7, label: "kept"}\n'
                            '  println(value.number)\n  println(value.label)\n  println(value.label)\n', b'7\nkept\nkept\n', 0),
    'ordinary_native_argv': ('native fn inspect(values: str[]): void =>\n  println(values[1])\n  println(values[1])\n'
                             'fn main(args: str[]): void =>\n  inspect(args)\n', b'payload\npayload\n', 0),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn'))
    args = parser.parse_args(); compiler = args.compiler.resolve()
    records = []
    with tempfile.TemporaryDirectory(prefix='sn-native-entry-') as folder:
        work = Path(folder)
        for name,(source,expected,status) in CASES.items():
            path = work/'main.sn'; path.write_text(source)
            for optimization in ('-O0','-O1','-O2'):
                for mode in ('default','checked','unchecked'):
                    outcomes = {}
                    for target in ('c','rust'):
                        executable = work/(target+'.exe')
                        command = [str(compiler),'main.sn','--no-install','--target',target,optimization,'-o',str(executable)]
                        if mode != 'default': command.append('--'+mode)
                        built = subprocess.run(command,cwd=work,capture_output=True,timeout=180)
                        if built.returncode:
                            raise RuntimeError(f'{name}/{target}/{optimization}/{mode}: '+built.stderr.decode(errors='replace'))
                        run = subprocess.run([str(executable),'payload','with space'],cwd=work,capture_output=True,timeout=15)
                        wanted = expected.replace(b'\n',b'\r\n') if os.name == 'nt' else expected
                        if run.returncode != status or run.stdout != wanted or run.stderr:
                            raise RuntimeError(f'{name}/{target}/{optimization}/{mode}: expected {status}/{wanted!r}, got {run.returncode}/{run.stdout!r}/{run.stderr!r}')
                        outcomes[target] = {'exit':run.returncode,'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex()}
                    if outcomes['c'] != outcomes['rust']: raise RuntimeError(f'target mismatch: {name}')
                    records.append({'case':name,'optimization':optimization,'arithmetic':mode,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'targets':outcomes,'passed':True})
    report = {'cases':records,'compilations':len(records)*2,'passed':True,
              'scope':'Native application entry behavior, argv storage/borrows, initialization, fields, names and exits; unrelated C failures are not parity evidence.'}
    output = ROOT/'.sn/native-entrypoints.json'; output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(records)} native entry/control cases, {len(records)*2} C/Rust compilations')


if __name__ == '__main__': main()
