#!/usr/bin/env python3
"""Verify search over canonical string/char slots with C-defined equality and order."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'native fn byteLength(values: byte[]): int =>\n  return values.length\nfn receiver(): str[] =>\n  println("receiver")\n  return {"left","right"}\nfn needle(): str =>\n  println("needle")\n  return "right"\nfn main(): void =>\n  var bytes: byte[] = {1}\n  println(byteLength(bytes))\n  var missing: str = nil\n  var values: str[] = {"same",missing,"","same"}\n  var same: str = "sa" + "me"\n  println(values.contains(same))\n  println(values.indexOf(same))\n  println(values.contains(missing))\n  println(values.indexOf(missing))\n  println(values.contains(""))\n  println(values.indexOf(""))\n  println(values.contains("absent"))\n  println(values.indexOf("absent"))\n  println(values.contains("same\\0tail"))\n  println(values.indexOf("same\\0tail"))\n  println(receiver().contains(needle()))\n  var chars: char[] = {\'a\',\'z\'}\n  println(chars.contains(\'z\'))\n  println(chars.indexOf(\'z\'))\n  println(chars.contains(\'x\'))\n  println(chars.indexOf(\'x\'))\n  var bits: bool[] = {false,true}\n  println(bits.contains(true))\n  println(bits.indexOf(false))\n'
EXPECTED = b'1\ntrue\n0\ntrue\n1\ntrue\n2\nfalse\n-1\ntrue\n0\nreceiver\nneedle\ntrue\ntrue\n1\nfalse\n-1\ntrue\n0\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    cases = []
    with tempfile.TemporaryDirectory(prefix='sn-native-array-search-') as folder:
        work = Path(folder)
        (work/'main.sn').write_text(SOURCE, encoding='utf-8')
        modes = [('-O0','default')] if args.quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]
        for opt, mode in modes:
            outcomes = {}
            for target in ('c','rust'):
                exe = work/(target+'.exe')
                command = [str(compiler),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                if mode != 'default':
                    command.append('--'+mode)
                build = subprocess.run(command,cwd=work,capture_output=True,timeout=180)
                if build.returncode:
                    raise ValueError(f'{target}/{opt}/{mode}: {build.stderr.decode(errors="replace")}')
                run = subprocess.run([str(exe)],cwd=work,capture_output=True,timeout=15)
                expected = EXPECTED.replace(b'\n',b'\r\n') if os.name == 'nt' else EXPECTED
                if (run.returncode,run.stdout,run.stderr) != (0,expected,b''):
                    raise ValueError(f'{target}/{opt}/{mode}: {run.returncode} {run.stdout!r} {run.stderr!r}')
                outcomes[target] = {'exit':0,'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex()}
            cases.append({'opt':opt,'mode':mode,'targets':outcomes,'passed':True})
    output = ROOT/'.sn/native-array-search.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps({'passed':True,'cases':cases,'source_sha256':hashlib.sha256(SOURCE.encode()).hexdigest()},indent=2)+'\n')
    print(f'PASS: {len(cases)} native array search C/Rust mode pairs')


if __name__ == '__main__':
    main()
