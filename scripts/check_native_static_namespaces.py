#!/usr/bin/env python3
"""Preserve C-supported empty native static namespaces without record transport."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    args = parser.parse_args(); compiler = args.compiler.resolve(); results = []
    with tempfile.TemporaryDirectory(prefix='sn-empty-native-namespace-') as folder:
        work = Path(folder)
        header = work/'operations.h'
        header.write_text('#include "sn_array.h"\n'
            'static inline long long namespace_number(void) { return 23; }\n'
            'static inline SnArray *namespace_bytes(void) { SnArray *a=sn_array_new(1,3); '
            'a->elem_tag=SN_TAG_BYTE; unsigned char bytes[]={0,128,255}; '
            'for(int i=0;i<3;i++) sn_array_push(a,&bytes[i]); return a; }\n')
        include = '@include '+json.dumps(header.as_posix())+'\n'
        cases = {
            'ordinary_static': ('native struct Empty =>\n  static fn text(): str =>\n'
                '    return "namespace"\nfn main(): void =>\n  println(Empty.text())\n'
                '  println(sizeof(Empty))\n',b'namespace\n0\n'),
            'native_static': (include+'native struct Empty =>\n  @alias "namespace_number"\n'
                '  static native fn number(): int\nfn main(): void =>\n'
                '  println(Empty.number())\n  println(sizeof(Empty))\n',b'23\n0\n'),
            'owned_byte_result': (include+'native struct Empty =>\n  @alias "namespace_bytes"\n'
                '  static native fn decode(): byte[]\n'
                'fn main(): void =>\n  var held: byte[] = Empty.decode()\n'
                '  println(held.toHex())\n  held[0]=42\n  println(Empty.decode().toHex())\n'
                '  println(held.toHex())\n  println(sizeof(Empty))\n',b'0080ff\n0080ff\n2a80ff\n0\n'),
        }
        for name,(source,expected) in cases.items():
            (work/'main.sn').write_text(source)
            for opt in ('-O0','-O1','-O2'):
                for mode in ('default','checked','unchecked'):
                    outcomes = {}
                    for target in ('c','rust'):
                        exe = work/(target+'.exe')
                        command = [str(compiler),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                        if mode!='default': command.append('--'+mode)
                        built = subprocess.run(command,cwd=work,capture_output=True,timeout=180)
                        if built.returncode: raise ValueError(f'{name}/{target}/{opt}/{mode}: '+built.stderr.decode(errors='replace'))
                        run = subprocess.run([str(exe)],cwd=work,capture_output=True,timeout=15)
                        wanted = expected.replace(b'\n',b'\r\n') if os.name=='nt' else expected
                        if run.returncode or run.stdout!=wanted or run.stderr:
                            raise ValueError(f'{name}/{target}/{opt}/{mode}: {run.returncode} {run.stdout!r} {run.stderr!r}')
                        outcomes[target] = {'exit':run.returncode,'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex()}
                    results.append({'case':name,'opt':opt,'mode':mode,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                                    'targets':outcomes,'passed':True})
    destination = ROOT/'.sn/native-static-namespaces.json';destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps({'cases':results,'passed':True,'compilations':len(results)*2,
        'scope':'Empty static owners, ordinary/native methods, independent owned byte results and exact C sizeof; no new empty record ABI transport.'},indent=2)+'\n')
    print(f'PASS: {len(results)} native namespace cases, {len(results)*2} C/Rust compilations')


if __name__=='__main__': main()
