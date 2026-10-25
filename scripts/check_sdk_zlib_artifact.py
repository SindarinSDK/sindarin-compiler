#!/usr/bin/env python3
"""Verify the unchanged zlib API through legacy imports and independent C artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import tempfile

from build_sdk_package import checked, prepare

ROOT = Path(__file__).resolve().parents[1]
CONSTANTS = {'zlibOk':0,'zlibStreamEnd':1,'zlibNeedDict':2,'zlibErrno':-1,
             'zlibStreamError':-2,'zlibDataError':-3,'zlibMemError':-4,'zlibBufError':-5,
             'zlibVersionError':-6,'noCompression':0,'bestSpeed':1,'bestCompression':9,'defaultCompression':-1}
CONTROL = '''import "sindarin-pkg-sdk/src/encoding/zlib" as zlib
native fn rawRoundtrip(): bool =>
  var source: byte[] = {0b,128b,255b,0b,42b,1b}
  var bound: uint = zlib.compressBound(source.length.toUint())
  var output: byte[bound.toInt()]
  var count: uint = bound
  var code: int = zlib.compress(addressOf(output), count, addressOf(source), source.length.toUint())
  if code != 0 =>
    return false
  var restored: byte[6]
  var size: uint = 6
  code = zlib.uncompress(addressOf(restored), size, addressOf(output), count)
  if code != 0 || size != 6 =>
    return false
  for index in 0..6 =>
    if restored[index] != source[index] =>
      return false
  count = bound
  code = zlib.compress2(addressOf(output), count, addressOf(source), source.length.toUint(), 9)
  if code != 0 =>
    return false
  size = 6
  code = zlib.uncompress(addressOf(restored), size, addressOf(output), count)
  if code != 0 || size != 6 =>
    return false
  for index in 0..6 =>
    if restored[index] != source[index] =>
      return false
  return true
fn escaped(): byte[] =>
  var original: byte[] = {0b,128b,255b,0b,42b,1b}
  return zlib.compressData(original)
fn main(): void =>
'''+''.join(f'  println(zlib.{name}() == {value})\n' for name,value in CONSTANTS.items())+'''  var sizedint: int[2]
  println(sizedint.length == 2 && sizedint[0] == 0)
  var sizedlong: long[2]
  println(sizedlong.length == 2 && sizedlong[0] == 0)
  var sizedint32: int32[2]
  println(sizedint32.length == 2 && sizedint32[0] == 0)
  var sizeduint: uint[2]
  println(sizeduint.length == 2 && sizeduint[0] == 0)
  var sizeduint32: uint32[2]
  println(sizeduint32.length == 2 && sizeduint32[0] == 0)
  var sizedbyte: byte[2]
  println(sizedbyte.length == 2 && sizedbyte[0] == 0)
  var sizedchar: char[2]
  println(sizedchar.length == 2 && sizedchar[0] == '\0')
  var sizedbool: bool[2]
  println(sizedbool.length == 2 && sizedbool[0] == false)
  var sizedfloat: float[2]
  println(sizedfloat.length == 2 && sizedfloat[0] == 0)
  var sizeddouble: double[2]
  println(sizeddouble.length == 2 && sizeddouble[0] == 0)
  println(rawRoundtrip())
  var source: byte[] = {0b,128b,255b,0b,42b,1b}
  var compressed: byte[] = escaped()
  var restored: byte[] = zlib.decompressData(compressed, 6)
  println(restored.length == 6 && restored[0] == 0 && restored[1] == 128 && restored[2] == 255 && restored[4] == 42)
  println(zlib.isCompressed(compressed))
  var empty: byte[0]
  println(zlib.compressData(empty).length == 0 && zlib.compressDataLevel(empty, -2).length == 0)
  println(zlib.decompressData(empty, 9).length == 0)
  println(zlib.compressDataLevel(source, -2).length == 0)
  var garbage: byte[] = {1b,2b,3b}
  println(zlib.decompressData(garbage, 20).length == 0)
  println(zlib.decompressData(compressed, 1).length == 0)
  var bound: int = zlib.maxCompressedSize(6)
  var destination: byte[bound]
  var written: int = zlib.compressTo(source, destination)
  println(written > 0 && written <= bound && destination.length == bound)
  var output: byte[6]
  var read: int = zlib.decompressTo(destination[0..written], output)
  println(read == 6 && output[1] == 128 && output[2] == 255 && source[1] == 128)
  var tiny: byte[1]
  println(zlib.compressTo(source, tiny) == -1)
  println(zlib.compressToLevel(source, destination, -2) == -1)
  println(zlib.decompressTo(garbage, output) == -1)
  println(zlib.compressToLevel(source, destination, 0) > 0)
  println(zlib.compressionRatio(8, 2) == 25.0 && zlib.compressionRatio(0, 0) == 0.0)
  println(zlib.spaceSaved(8, 2) == 75.0)
  println(zlib.errorMessage(0) == "OK" && zlib.errorMessage(-3) == "Data error (corrupted or incomplete)" && zlib.errorMessage(99) == "Unknown error")
'''


def verify(compiler, sdk, quick=False):
    compiler, sdk = Path(compiler).resolve(), Path(sdk).resolve()
    public = {str(p.relative_to(sdk)):p.read_bytes() for p in (sdk/'src').rglob('*.sn')}
    original = sdk/'tests/encoding/test_zlib.sn';oracle=original.with_suffix('.expected')
    sources={str(p.relative_to(sdk)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (original,oracle)}
    modes=[('-O0','default')] if quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]
    cases=[]
    with tempfile.TemporaryDirectory(prefix='sn-sdk-zlib-artifacts-') as folder:
        app=Path(folder);package=app/'.sn/sindarin-pkg-sdk'
        manifest=prepare(compiler,sdk,package,('encoding/zlib',))
        model=json.loads((package/'native/encoding_zlib.model.json').read_text())
        exports={f['name'] for f in model['functions']}
        covered=set(re.findall(r'zlib\.(\w+)\(',CONTROL))
        assert len(exports) == 28 and exports == covered, (exports-covered, covered-exports)
        built=checked([compiler,'--build-native',manifest,'-o',package/'built'],app)
        assembly=Path(json.loads(built.stdout)['assembly']);metadata=json.loads(assembly.read_text())
        assert len(metadata['units']) == 2 and len(metadata['provider_signatures']) == 28 and metadata['abi'] == '1.9'
        assert {u['language'] for u in metadata['units']} == {'C','SN'}
        outputs=CONTROL.count('  println(')
        for artifact in ('source','sealed','legacy'):
            if artifact == 'sealed':
                shutil.copytree(assembly.parent,package/'sealed')
                manifest.write_text(manifest.read_text().replace('native:\n','native:\n  assembly: {path: sealed/assembly.json, sha256: '+hashlib.sha256(assembly.read_bytes()).hexdigest()+'}\n'))
                for directory in (package/'src',package/'native'):
                    for p in directory.rglob('*.c'):p.unlink()
                assert not any(p for directory in (package/'src',package/'native') for p in directory.rglob('*.c'))
            if artifact == 'legacy':
                assert all((package/name).read_bytes()==data for name,data in public.items())
                shutil.rmtree(package);package.symlink_to(sdk,target_is_directory=True)
            for name,source,expected in [('original',original.read_bytes(),oracle.read_bytes().replace(b'\r\n',b'\n')),
                                          ('all-exports',CONTROL.encode(),b'true\n'*outputs)]:
                (app/'main.sn').write_bytes(source)
                if os.name == 'nt':expected=expected.replace(b'\n',b'\r\n')
                for target in ('c','rust'):
                    for opt,mode in modes:
                        exe=app/'consumer.exe';command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',exe]
                        if mode != 'default':command.append('--'+mode)
                        checked(command,app);run=checked([exe],app)
                        if run.stdout != expected or run.stderr:raise ValueError(f'{name}/{artifact}/{target}/{opt}/{mode}: {run.stdout!r} {run.stderr!r}')
                        cases.append({'case':name,'artifact':artifact,'target':target,'opt':opt,'mode':mode,
                                      'source_sha256':hashlib.sha256(source).hexdigest(),'stdout_hex':run.stdout.hex(),'stderr_hex':run.stderr.hex(),'passed':True})
    report={'passed':True,'cases':cases,'exports':sorted(exports),'control_source':CONTROL,'control_outputs':outputs,
            'original_source_sha256':sources,'public_source_sha256':{n:hashlib.sha256(v).hexdigest() for n,v in public.items()},
            'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
            'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'environment':{n:os.environ.get(n) for n in ('SN_CC','SN_CFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')}}
    suffix='-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS','') else ''
    (ROOT/f'.sn/sdk-zlib-artifact{suffix}.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(cases)} original/all28-export zlib legacy/source/sealed cases; {outputs} control outputs')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--sdk',type=Path,default=ROOT/'.sn/sdk-native-integration')
    parser.add_argument('--quick',action='store_true');args=parser.parse_args()
    verify(args.compiler,args.sdk,args.quick)
