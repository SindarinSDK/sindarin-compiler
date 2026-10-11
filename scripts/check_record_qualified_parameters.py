#!/usr/bin/env python3
"""Verify record rebinding, aliases and as-val string/array isolation in C and Rust."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]


def verify(compiler):
    source=ROOT/'tests/package/record_qualified_parameters.sn'
    oracle=source.with_suffix('.expected')
    expected=oracle.read_bytes().replace(b'\r\n',b'\n')
    if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
    cases=[]
    with tempfile.TemporaryDirectory(prefix='sn-record-qualified-') as folder:
        work=Path(folder)
        for opt in ('-O0','-O1','-O2'):
            for mode in ('default','checked','unchecked'):
                for target in ('c','rust'):
                    executable=work/(target+'.exe')
                    command=[str(compiler),str(source),'--no-install','--target',target,opt,'-o',str(executable)]
                    if mode!='default':command.append('--'+mode)
                    build=subprocess.run(command,cwd=work,capture_output=True,timeout=180)
                    if build.returncode:raise ValueError(build.stderr.decode(errors='replace'))
                    run=subprocess.run([str(executable)],cwd=work,capture_output=True,timeout=30)
                    if (run.returncode,run.stdout,run.stderr)!=(0,expected,b''):
                        raise ValueError(f'{target}/{opt}/{mode}: {run.returncode} {run.stdout!r} {run.stderr!r}')
                    cases.append(dict(target=target,opt=opt,mode=mode,passed=True))
    suffix='-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS','') else ''
    output=ROOT/f'.sn/record-qualified-parameters{suffix}.json';output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(dict(passed=True,cases=cases,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),oracle_sha256=hashlib.sha256(oracle.read_bytes()).hexdigest(),compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest()),indent=2)+'\n')
    print(f'PASS: {len(cases)} qualified record rebinding and managed-field copy cases')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    args=parser.parse_args();verify(args.compiler.resolve())
