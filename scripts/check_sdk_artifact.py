#!/usr/bin/env python3
"""Verify original SDK callers through generated canonical C package artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
from build_sdk_package import prepare, checked, MODULES

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ('io/bytes','io/textfile','io/binaryfile','io/path','io/directory','io/stdio','os/env','os/os','crypto/crypto','time/time','time/date','core/random','core/uuid')
STDIN_CASES = (
    ('words', b'line one\nZalpha beta',
     b'true\ntrue\nfalse\nline one\n90\nalpha\nbeta\ntrue\nfalse\nfalse\n-1\nfalse\n0\nfalse\n0\nline one\n\n',
     b'\nline one\n'),
    ('empty', b'',
     b'false\nfalse\ntrue\n\n-1\n\n\ntrue\nfalse\nfalse\n-1\nfalse\n0\nfalse\n0\n\n\n',
     b'\n\n'),
)


def stream_bytes(value):
    return value.replace(b'\n',b'\r\n') if os.name=='nt' else value


def verify(compiler, sdk, all_modes=True, fixtures=FIXTURES):
    fixtures=tuple(fixtures)
    if not fixtures or len(set(fixtures))!=len(fixtures) or any(f not in FIXTURES for f in fixtures):
        raise ValueError('select distinct original SDK fixture modules')
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    cases=[];sources={}
    with tempfile.TemporaryDirectory(prefix='sn-sdk-artifact-') as folder:
        app=Path(folder);package=app/'.sn/sindarin-pkg-sdk'
        manifest=prepare(compiler,sdk,package)
        public={p.relative_to(sdk).as_posix():p.read_bytes() for p in (sdk/'src').rglob('*.sn')}
        modes=[(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')] if all_modes else [('-O0','default')]
        for archive in ('source','prebuilt'):
            if archive=='prebuilt':
                built=checked([compiler,'--build-native',manifest,'-o',package/'sealed-build'],app)
                assembly=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(assembly.read_text())
                assert len(metadata['units'])==len(MODULES)*2 and {u['language'] for u in metadata['units']}=={'C','SN'}
                shutil.copytree(assembly.parent,package/'dist')
                text=manifest.read_text();manifest.write_text(text.replace('  abi: 1.7\n',
                    '  abi: 1.7\n  assembly: {path: dist/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                for directory in (package/'src',package/'native'):
                    for source in directory.rglob('*.c'):source.unlink()
                if any(source for directory in (package/'src',package/'native') for source in directory.rglob('*.c')):
                    raise ValueError('sealed SDK retains backing C sources')
            for module in fixtures:
                source=sdk/'tests'/module.rsplit('/',1)[0]/('test_'+module.rsplit('/',1)[1]+'.sn')
                oracle=source.with_suffix('.expected')
                sources[source.relative_to(sdk).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
                sources[oracle.relative_to(sdk).as_posix()]=hashlib.sha256(oracle.read_bytes()).hexdigest()
                shutil.copyfile(source,app/'main.sn')
                expected=oracle.read_bytes().replace(b'\r\n',b'\n')
                expected_error=b'OK\n'*4 if module=='io/stdio' else b''
                if os.name=='nt':
                    expected=expected.replace(b'\n',b'\r\n')
                    expected_error=expected_error.replace(b'\n',b'\r\n')
                for opt,mode in modes:
                    for target in ('c','rust'):
                        executable=app/(target+'.exe')
                        command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',executable]
                        if mode!='default':command.append('--'+mode)
                        checked(command,app)
                        run=checked([executable],app)
                        if run.stdout!=expected or run.stderr!=expected_error:
                            raise ValueError(f'{module}/{archive}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append({'module':module,'archive':archive,'target':target,'opt':opt,'mode':mode,'passed':True})
                if module=='io/stdio':
                    source=ROOT/'tests/package/sdk_stdio_input.sn'
                    sources['compiler:'+source.relative_to(ROOT).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
                    shutil.copyfile(source,app/'main.sn')
                    for opt,mode in modes:
                        for target in ('c','rust'):
                            executable=app/(target+'.exe')
                            command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',executable]
                            if mode!='default':command.append('--'+mode)
                            checked(command,app)
                            for name,input_bytes,stdout,stderr in STDIN_CASES:
                                run=checked([executable],app,input_bytes)
                                if run.stdout!=stream_bytes(stdout) or run.stderr!=stream_bytes(stderr):
                                    raise ValueError(f'stdin/{name}/{archive}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                                cases.append({'module':module,'stdin':name,'archive':archive,'target':target,'opt':opt,'mode':mode,'passed':True})
            for name,original in public.items():
                if (package/name).read_bytes()!=original:raise ValueError('public SDK module changed: '+name)
    return {'cases':cases,'passed':True,'source_sha256':sources,'modules':list(MODULES),'fixtures':list(fixtures),
            'public_source_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in public.items()},
            'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
            'public_sources_preserved':True,'backing_sources_removed_for_prebuilt':True,
            'scope':'Canonical C implementations, unchanged SDK callers/oracles; remaining SDK modules and wider provider contracts still require migration'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--sdk',type=Path,default=ROOT/'.sn/sdk-native-integration')
    parser.add_argument('--quick',action='store_true')
    parser.add_argument('--fixtures',nargs='+',choices=FIXTURES,default=list(FIXTURES))
    args=parser.parse_args();report=verify(args.compiler,args.sdk,not args.quick,args.fixtures)
    output=ROOT/'.sn/sdk-artifact-validation.json';output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n')
    inputs=sum('stdin' in case for case in report['cases'])
    print(f'PASS: {len(report["cases"])-inputs} original SDK artifact caller cases and {inputs} stdin controls')


if __name__=='__main__':main()
