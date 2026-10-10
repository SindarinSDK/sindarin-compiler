#!/usr/bin/env python3
"""Exercise shared C package lifecycle from C/Rust/Go and under sanitizers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]

def checked(command, cwd, env=None):
    result=subprocess.run([str(a) for a in command],cwd=cwd,env=env,capture_output=True,timeout=180)
    if result.returncode:raise RuntimeError(f'{command!r}\n'+result.stderr.decode(errors='replace'))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    runtime=ROOT/'bin/lib'/('clang' if os.name=='nt' else 'gcc')/'libsn_runtime_min.a'
    includes=ROOT/'bin/include/runtime'
    cc=shlex.split(os.environ.get('SN_CC') or ('clang' if os.name=='nt' or platform.system()=='Darwin' else 'gcc'))
    report={'platform':platform.system(),'sanitized':args.sanitize,'cases':[],
            'runtime_sha256':hashlib.sha256(runtime.read_bytes()).hexdigest(),
            'scope':'Shared package lifecycle coordination; generated globals/SDK artifact migration remains required'}
    with tempfile.TemporaryDirectory(prefix='sn-package-lifecycle-') as folder:
        work=Path(folder);executable=work/'client.exe'
        source=ROOT/'tests/runtime_abi/package_lifecycle.c'
        flags=['-std=c11','-D_GNU_SOURCE','-Wall','-Wextra','-Werror','-UNDEBUG','-I',includes]
        if args.sanitize:
            flags+=['-g','-O0','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            sources=[ROOT/'src/runtime'/p for p in ('sn_package.c','sn_abi.c','sn_array.c','sn_string.c','sn_byte.c')]
        else:sources=[runtime]
        checked(cc+flags+[source]+sources+['-pthread','-o',executable],work)
        run=checked([executable],work)
        assert run.stdout.splitlines()==[b'package lifecycle: pass'] and not run.stderr
        report['cases'].append({'client':'C','passed':True,'concurrent_callers':16})
        if not args.sanitize:
            rustc=shlex.split(os.environ.get('SN_RUSTC','rustc'))
            flags=shlex.split(os.environ.get('SN_RUSTFLAGS',''))
            checked(rustc+['--edition=2021',ROOT/'tests/runtime_abi/package_lifecycle.rs','-L',runtime.parent,
                          '-C','link-arg=-pthread','-o',executable]+flags,work)
            run=checked([executable],work)
            assert run.stdout.splitlines()==[b'package lifecycle: pass'] and not run.stderr
            report['cases'].append({'client':'Rust','passed':True})
            project=work/'go';shutil.copytree(ROOT/'tests/runtime_abi/package_go',project)
            shutil.copyfile(includes/'sn_abi.h',project/'sn_abi.h')
            environment=dict(os.environ,CGO_ENABLED='1',CC=shlex.join(cc),
                CGO_CPPFLAGS='',CGO_LDFLAGS=shlex.join(['-L'+str(runtime.parent),'-lsn_runtime_min','-pthread']))
            checked(shlex.split(os.environ.get('SN_GO','go'))+['build','-o',executable,'.'],project,environment)
            run=checked([executable],work)
            assert run.stdout.splitlines()==[b'package lifecycle: pass'] and not run.stderr
            report['cases'].append({'client':'Go','passed':True})
    report['passed']=True
    (ROOT/'.sn').mkdir(exist_ok=True)
    (ROOT/'.sn'/('package-lifecycle-sanitizers.json' if args.sanitize else 'package-lifecycle.json')).write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: shared C package lifecycle '+('sanitizers' if args.sanitize else 'C/Rust/Go clients'))

if __name__=='__main__':main()
