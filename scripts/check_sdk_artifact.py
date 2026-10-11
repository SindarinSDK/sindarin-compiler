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
FIXTURES = ('io/bytes','io/textfile','io/binaryfile','io/path','io/directory','os/env','os/os','crypto/crypto')


def verify(compiler, sdk, all_modes=True):
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
            for module in FIXTURES:
                source=sdk/'tests'/module.rsplit('/',1)[0]/('test_'+module.rsplit('/',1)[1]+'.sn')
                oracle=source.with_suffix('.expected')
                sources[source.relative_to(sdk).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
                sources[oracle.relative_to(sdk).as_posix()]=hashlib.sha256(oracle.read_bytes()).hexdigest()
                shutil.copyfile(source,app/'main.sn')
                expected=oracle.read_bytes().replace(b'\r\n',b'\n')
                if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                for opt,mode in modes:
                    for target in ('c','rust'):
                        executable=app/(target+'.exe')
                        command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',executable]
                        if mode!='default':command.append('--'+mode)
                        checked(command,app)
                        run=checked([executable],app)
                        if run.stdout!=expected or run.stderr:
                            raise ValueError(f'{module}/{archive}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append({'module':module,'archive':archive,'target':target,'opt':opt,'mode':mode,'passed':True})
            for name,original in public.items():
                if (package/name).read_bytes()!=original:raise ValueError('public SDK module changed: '+name)
    return {'cases':cases,'passed':True,'source_sha256':sources,'modules':list(MODULES),
            'public_source_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in public.items()},
            'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
            'public_sources_preserved':True,'backing_sources_removed_for_prebuilt':True,
            'scope':'Canonical C implementations, unchanged SDK callers/oracles; remaining SDK modules and wider provider contracts still require migration'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--sdk',type=Path,default=ROOT/'.sn/sdk-native-integration')
    parser.add_argument('--quick',action='store_true')
    args=parser.parse_args();report=verify(args.compiler,args.sdk,not args.quick)
    output=ROOT/'.sn/sdk-artifact-validation.json';output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(report["cases"])} original SDK artifact caller cases')


if __name__=='__main__':main()
