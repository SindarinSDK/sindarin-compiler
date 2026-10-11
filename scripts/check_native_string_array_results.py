#!/usr/bin/env python3
"""Preserve string array-producing methods in native canonical storage contexts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (
    'native fn byteLength(values: byte[]): int =>\n  return values.length\n'
    'fn text(): str =>\n  println("once")\n  return "abc"\n'
    'fn main(): void =>\n  var values: byte[] = text().toBytes()\n'
    '  var copied: byte[] = copyOf(values)\n  println(byteLength(values))\n  copied[0]=90\n  println(values.toHex())\n  println(copied.toHex())\n'
    '  var nilText: str = nil\n  var nilBytes: byte[] = nilText.toBytes()\n'
    '  println(nilBytes==nil)\n  println(nilBytes.length)\n'
    '  var empty: byte[] = "".toBytes()\n  println(empty==nil)\n  println(empty.length)\n'
    '  var pieces: str[] = "left|right".split("|")\n  println(pieces.length)\n  println(pieces[0])\n  println(pieces[1])\n'
    '  var lines: str[] = "one\\ntwo".splitLines()\n  println(lines.length)\n  println(lines[0])\n  println(lines[1])\n'
    '  var words: str[] = " a  b\\tc ".splitWhitespace()\n  println(words.length)\n'
    '  println(words[0])\n  println(words[1])\n  println(words[2])\n')
EXPECTED = b'once\n3\n616263\n5a6263\nfalse\n0\nfalse\n0\n2\nleft\nright\n2\none\ntwo\n3\na\nb\nc\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT/'bin'/('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    cases = []
    with tempfile.TemporaryDirectory(prefix='sn-native-string-results-') as folder:
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
    output = ROOT/'.sn/native-string-array-results.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps({'passed':True,'cases':cases,'source_sha256':hashlib.sha256(SOURCE.encode()).hexdigest()},indent=2)+'\n')
    print(f'PASS: {len(cases)} native string array-result C/Rust mode pairs')


if __name__ == '__main__':
    main()
