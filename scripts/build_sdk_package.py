#!/usr/bin/env python3
"""Stage canonical SDK modules for independent C artifact construction.

Public Sindarin modules and C implementations are copied unchanged. Generated
backing functions forward to their original C names, so provider namespacing
cannot change the symbols called by unchanged Sindarin facade bodies.
"""
import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

MODULES = ('io/bytes', 'io/textfile', 'io/binaryfile', 'io/path',
           'io/directory', 'io/stdio', 'os/os', 'os/env', 'crypto/crypto')
RECORDS = {
    'io/textfile': ('TextFile', 'SnSdkTextFileRecord', 'sn_sdk_text_file'),
    'io/binaryfile': ('BinaryFile', 'SnSdkBinaryFileRecord', 'sn_sdk_binary_file'),
}
RAW = {'int': 'long long', 'long': 'long long', 'int32': 'int32_t',
       'uint': 'uint64_t', 'uint32': 'uint32_t', 'byte': 'unsigned char',
       'char': 'char', 'bool': 'bool', 'float': 'float', 'double': 'double',
       'string': 'char *', 'void': 'void'}


def checked(command, cwd):
    result = subprocess.run([str(v) for v in command], cwd=cwd, capture_output=True, timeout=180)
    if result.returncode:
        raise ValueError(result.stderr.decode(errors='replace'))
    return result


def raw_type(value, module):
    kind = value['kind']
    if kind in RAW:
        return RAW[kind]
    if kind == 'array':
        from native_contract import native_array_shape
        native_array_shape(value)
        return 'SnArray *'
    if kind == 'struct' and module in RECORDS and value['name'] == RECORDS[module][0]:
        return RECORDS[module][1]+' *'
    raise ValueError(f'SDK artifact type still needs a package contract: {module}: {value}')


def ownership(function, receiver=False):
    parameters = ['self: borrowed'] if receiver else []
    for parameter in function['params']:
        managed = parameter['type']['kind'] in ('string', 'array', 'struct')
        parameters.append(parameter['name']+(': borrowed' if managed else ': value'))
    managed = function['return_type']['kind'] in ('string', 'array', 'struct')
    return '{parameters: {'+', '.join(parameters)+'}, result: '+('owned' if managed else 'value')+'}'


def prepare(compiler, sdk, destination, modules=MODULES):
    compiler, sdk, destination = (Path(p).resolve() for p in (compiler, sdk, destination))
    modules = tuple(modules)
    if not modules or len(set(modules)) != len(modules) or any(m not in MODULES for m in modules):
        raise ValueError('select distinct implemented SDK modules: '+', '.join(MODULES))
    if destination.exists():
        raise ValueError('SDK artifact destination already exists: '+str(destination))
    if destination.is_relative_to(sdk):
        raise ValueError('stage the SDK outside its source checkout')
    original_manifest = (sdk/'sn.yaml').read_text()
    if re.search(r'^native:', original_manifest, re.M):
        raise ValueError('SDK already has native metadata; use its manifest directly')
    if re.search(r'^runtime:', original_manifest, re.M):
        raise ValueError('SDK artifact staging expects the legacy manifest without a fixed runtime')
    # Validate all inputs before creating the output package.
    for module in modules:
        for suffix in ('.sn', '.sn.c'):
            if not (sdk/'src'/(module+suffix)).is_file():
                raise ValueError('SDK implementation missing: '+module+suffix)
        if module in RECORDS or module == 'io/bytes':
            for suffix in ('.native.c', '.native.h'):
                if not (sdk/'src'/(module+suffix)).is_file():
                    raise ValueError('canonical SDK module missing: '+module+suffix)
    destination.mkdir(parents=True)
    shutil.copytree(sdk/'src', destination/'src')
    (destination/'sn.yaml').write_text(original_manifest)
    native = destination/'native'; native.mkdir()
    # Preserve the compiler's third-party header installation as build inputs.
    # Libraries retain the original @link names and compiler library search path.
    dependency_headers = compiler.parent/'deps/include'
    include_dirs = ['native']
    if dependency_headers.is_dir():
        shutil.copytree(dependency_headers, native/'include', symlinks=False)
        include_dirs.append('native/include')
    models = {}
    for module in modules:
        stem = module.replace('/', '_')
        model_path = native/(stem+'.model.json')
        checked([compiler,destination/'src'/(module+'.sn'),'--package-body','--no-install',
                 '--emit-model','-o',model_path], destination)
        models[module] = json.loads(model_path.read_text())
    lines = [original_manifest.rstrip(), 'runtime: C', 'native:', '  abi: 1.7',
             '  declarations: ['+', '.join('src/'+m+'.sn' for m in modules)+']']
    records = [m for m in modules if m in RECORDS]
    if records:
        lines.append('  types:')
        for module in records:
            name, c_type, prefix = RECORDS[module]
            lines += ['    - declaration: src/'+module+'.sn::'+name,
                      '      identity: SindarinSDK/sindarin-pkg-sdk:'+module.rsplit('/', 1)[0].replace('/', '.')+'.'+name+'@1',
                      '      c_type: '+c_type, '      header: src/'+module+'.native.h']
            lines += ['      '+field+': '+prefix+'_'+field for field in ('create','retain','release','refs')]
            lines.append('      owner: atomic')
    lines.append('  builds:')
    bindings = []
    for module in modules:
        stem = module.replace('/', '_'); model = models[module]
        backing = module+('.native.c' if module in RECORDS or module == 'io/bytes' else '.sn.c')
        source = ['#include "sn_minimal.h"', '#include "../src/'+backing+'"']
        for function in model['functions']:
            if not function.get('is_native'):
                raise ValueError('SDK module root implementation requires an SN export: '+function['name'])
            name = function['name']; wrapper = 'sn_sdk_artifact_'+stem+'_'+name
            params = [raw_type(p['type'],module)+' p'+str(i) for i,p in enumerate(function['params'])]
            arguments = ', '.join('p'+str(i) for i in range(len(params)))
            result = raw_type(function['return_type'],module)
            source.append(result+' '+wrapper+'('+(', '.join(params) or 'void')+') { '+
                          ('return ' if result!='void' else '')+name+'('+arguments+'); }')
            bindings += ['    - declaration: src/'+module+'.sn::'+name,
                         '      function: '+wrapper, '      symbol: sdk_'+stem+'_helper_'+name,
                         '      build: native_'+stem, '      convention: C', '      failure: abort',
                         '      ownership: '+ownership(function)]
        (native/(stem+'.c')).write_text('\n'.join(source)+'\n')
        lines += ['    - name: native_'+stem, '      language: C', '      sources: [native/'+stem+'.c]',
                  '      provides_sources: [src/'+module+'.sn.c]',
                  '      include_dirs: ['+', '.join(include_dirs)+']']
        libraries = re.findall(r'^@link\s+(\w+)', (destination/'src'/(module+'.sn')).read_text(), re.M)
        if libraries:
            lines.append('      libraries: ['+', '.join(libraries)+']')
        lines += ['    - name: facade_'+stem, '      language: SN', '      entry: src/'+module+'.sn',
                  '      sources: [src/'+module+'.sn]', '      include_dirs: ['+', '.join(include_dirs)+']']
        for structure in model['structs']:
            for method in structure['methods']:
                # Validate every declared method; never silently omit wider APIs.
                raw_type(method['return_type'],module)
                for parameter in method['params']: raw_type(parameter['type'],module)
                name = structure['name']+'.'+method['name']
                if not method['is_static'] and module not in RECORDS:
                    raise ValueError('SDK instance storage still needs a canonical record: '+name)
                bindings += ['    - declaration: src/'+module+'.sn::'+name,
                             '      function: '+name, '      symbol: sdk_'+stem+'_method_'+name.replace('.', '_'),
                             '      build: facade_'+stem, '      convention: C', '      failure: abort',
                             '      ownership: '+ownership(method, not method['is_static'])]
    lines += ['  bindings:']+bindings
    manifest = destination/'sn.yaml'; manifest.write_text('\n'.join(lines)+'\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--modules', nargs='+', choices=MODULES, default=list(MODULES))
    parser.add_argument('--build', action='store_true')
    args = parser.parse_args()
    args.compiler = args.compiler.resolve()
    manifest = prepare(args.compiler,args.sdk,args.output,args.modules)
    if args.build:
        print(checked([args.compiler,'--build-native',manifest,'-o',manifest.parent/'artifacts'],manifest.parent).stdout.decode(),end='')
    else:
        print(manifest)


if __name__ == '__main__': main()
