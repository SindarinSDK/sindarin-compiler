#!/usr/bin/env python3
"""Force 32/64-bit foreign output slots with strict C pointer diagnostics."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
from build_sdk_package import prepare

ROOT=Path(__file__).resolve().parents[1]
HEADER='''#ifndef TEST_ZLIB_H
#define TEST_ZLIB_H
#include <stdint.h>
#include <string.h>
typedef WIDTH uLongf;
typedef WIDTH uLong;
static inline int compress2(unsigned char *out,uLongf *size,const unsigned char *source,uLong length,int level) {
    uLongf capacity=*size;
    if (level<0) return -2;
    if (!length) { out[0]=(void *)size==(const void *)source ? 99 : 7; *size=1; return 0; }
    *size=0;
    if (capacity<length) return -5;
    memcpy(out,source,length);*size=length;return 0;
}
static inline int compress(unsigned char *out,uLongf *size,const unsigned char *source,uLong length) { return compress2(out,size,source,length,6); }
static inline int uncompress(unsigned char *out,uLongf *size,const unsigned char *source,uLong length) { return compress2(out,size,source,length,6); }
static inline uLong compressBound(uLong length) { return length+16; }
#endif
'''
BACKING='''#include "sn_types.h"
unsigned char *byteView(uint64_t *value) { return (unsigned char *)value; }
'''
CONTROL='''@source "main.sn.c"
import "sindarin-pkg-sdk/src/encoding/zlib" as zip
native fn byteView(value: uint as ref): *byte
native fn __sn_header_zlib_compress_0(): int =>
  return 77
native fn __sn_header_zlib_compress2_0(): int =>
  return 78
native fn __sn_header_zlib_uncompress_0(): int =>
  return 79
native fn inspect(): void =>
  var source: byte[] = {1b,2b,3b,4b}
  var output: byte[16]
  var length: uint = 16
  println(zip.compress(addressOf(output),length,addressOf(source),4) == 0 && length == 4 && output[3] == 4)
  length = 16
  println(zip.compress2(addressOf(output),length,addressOf(source),4,6) == 0 && length == 4 && output[2] == 3)
  length = 16
  println(zip.uncompress(addressOf(output),length,addressOf(source),4) == 0 && length == 4 && output[1] == 2)
  length = 16
  println(zip.compress2(addressOf(output),length,addressOf(source),4,-2) == -2)
  length = 2
  println(zip.compress(addressOf(output),length,addressOf(source),4) == -5 && length == 0)
  length = 4294967312u
  println(zip.compress(addressOf(output),length,addressOf(source),4) == 0 && length == 4)
  length = 16
  println(zip.compress(addressOf(output),length,byteView(length),0) == 0 && length == 1 && output[0] == MARKER)
  println(zip.compressBound(4) == 20)
fn main(): void =>
  inspect()
  println(__sn_header_zlib_compress_0()+__sn_header_zlib_compress2_0()+__sn_header_zlib_uncompress_0() == 234)
'''


def verify(compiler,sdk,quick=False):
    compiler,sdk=Path(compiler).resolve(),Path(sdk).resolve();cases=[];builds=[]
    original_env=os.environ.copy();modes=[('-O0','default')] if quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]
    public={str(p.relative_to(sdk)):p.read_bytes() for p in (sdk/'src').rglob('*.sn')}
    def run(command,cwd):
        p=subprocess.run([str(x) for x in command],cwd=cwd,capture_output=True,timeout=180)
        if p.returncode:raise ValueError(str(command)+'\n'+p.stderr.decode(errors='replace'))
        return p
    try:
        with tempfile.TemporaryDirectory(prefix='sn-zlib-header-widths-') as folder:
            root=Path(folder).resolve()
            for width,c_type,marker in [(32,'uint32_t',7),(64,'uint64_t',99)]:
                app=root/str(width);app.mkdir();include=app/'include';include.mkdir()
                (include/'zlib.h').write_text(HEADER.replace('WIDTH',c_type))
                flags=shlex.split(original_env.get('SN_CFLAGS',''))+['-I'+include.as_posix(),'-Werror=incompatible-pointer-types']
                os.environ['SN_CFLAGS']=subprocess.list2cmdline(flags) if os.name=='nt' else shlex.join(flags)
                source=CONTROL.replace('MARKER',str(marker));(app/'main.sn').write_text(source);(app/'main.sn.c').write_text(BACKING)
                package=app/'.sn/sindarin-pkg-sdk';package.parent.mkdir()
                # Preserve legacy package imports and public source exactly.
                package.symlink_to(sdk,target_is_directory=True)
                for artifact in ('legacy','source','sealed'):
                    if artifact=='source':
                        package.unlink();manifest=prepare(compiler,sdk,package,('encoding/zlib',))
                        # Package-local dependency headers precede SN_CFLAGS. Force
                        # the same mock contract for providers and consumers.
                        staged_header=package/'native/include/zlib.h'
                        staged_header.parent.mkdir(parents=True,exist_ok=True)
                        staged_header.write_text((include/'zlib.h').read_text())
                        built=run([compiler,'--build-native',manifest,'-o',package/'built'],app)
                        assembly=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(assembly.read_text())
                        assert len(metadata['provider_signatures'])==28
                        builds.append({'width':width,'units':len(metadata['units']),'exports':28})
                    if artifact=='sealed':
                        shutil.copytree(assembly.parent,package/'sealed')
                        manifest.write_text(manifest.read_text().replace('native:\n','native:\n  assembly: {path: sealed/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                        for directory in (package/'src',package/'native'):
                            for path in directory.rglob('*.c'):path.unlink()
                    for target in ('c','rust'):
                        for opt,mode in modes:
                            exe=app/'client.exe';command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',exe]
                            if mode!='default':command.append('--'+mode)
                            run(command,app);result=run([exe],app)
                            expected=(b'true\r\n' if os.name=='nt' else b'true\n')*9
                            if result.stdout!=expected or result.stderr:raise ValueError(f'{width}/{artifact}/{target}/{opt}/{mode}: {result.stdout!r} {result.stderr!r}')
                            cases.append({'width':width,'artifact':artifact,'target':target,'opt':opt,'mode':mode,'stdout_hex':result.stdout.hex(),'passed':True})
                    if artifact!='legacy':assert all((package/n).read_bytes()==v for n,v in public.items())
    finally:
        if 'SN_CFLAGS' in original_env:os.environ['SN_CFLAGS']=original_env['SN_CFLAGS']
        else:os.environ.pop('SN_CFLAGS',None)
    report={'passed':True,'cases':cases,'builds':builds,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
            'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'header_template':HEADER,'control':CONTROL,'backing':BACKING,
            'scope':'Strict32/64-bit header contracts; scalar copyback, negative returns, high-bit clearing, compatible-pointer identity and helper-name hygiene. Mock C bodies verify adapters; real zlib unchanged caller/all-export checks remain separate.',
            'environment':{n:original_env.get(n) for n in ('SN_CC','SN_CFLAGS','SN_RUSTFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')}}
    suffix='-sanitized' if 'sanitize=' in original_env.get('SN_CFLAGS','') else ''
    (ROOT/f'.sn/zlib-header-widths{suffix}.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(cases)} strict32/64-bit zlib header legacy/source/sealed cases')
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--sdk',type=Path,default=ROOT/'.sn/sdk-native-integration');parser.add_argument('--quick',action='store_true')
    args=parser.parse_args();verify(args.compiler,args.sdk,args.quick)
