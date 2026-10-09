#!/usr/bin/env python3
"""Generate consumer C ABI adapters from compiler-resolved package declarations."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
import re
import contextlib
import io
from build_native_package import build

from native_contract import TYPES, kind, validate


def adapter(signature, package):
    binding = signature['binding']
    symbol, alias = binding['symbol'], signature['adapter']
    if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', symbol):
        raise ValueError('package native export is not a C-callable identifier')
    validate(signature)
    params = signature['params']
    result = kind(signature['return_type'])
    wire_params = [TYPES[kind(p['type'])][1] for p in params]
    status = binding['failure'] == 'status'
    if status and result != 'void': wire_params.append(TYPES[result][1] + ' *')
    return_wire = 'uint32_t' if status else TYPES[result][1]
    code = [f'extern {return_wire} {symbol}({", ".join(wire_params) or "void"});',
            f'{TYPES[result][0]} {alias}(' + ', '.join(f'{TYPES[kind(p["type"])][0]} p{i}' for i,p in enumerate(params)) + ') {']
    for i,p in enumerate(params):
        if kind(p['type']) == 'string':
            code += [f'  SnAbiValue *w{i} = NULL;',
                     f'  if (sn_abi_v1_string_copy(p{i}, &w{i}) != SN_ABI_OK) abort();']
    arguments = [f'w{i}' if kind(p['type']) == 'string' else f'({TYPES[kind(p["type"])][1]})p{i}' for i,p in enumerate(params)]
    if result != 'void': code += [f'  {TYPES[result][1]} wire_result = {{0}};']
    if status:
        if result != 'void': arguments.append('&wire_result')
        code += [f'  uint32_t status = {symbol}({", ".join(arguments)});']
    else:
        call = f'{symbol}({", ".join(arguments)})'
        code += [('  wire_result = ' if result != 'void' else '  ') + call + ';']
    for i,p in enumerate(params):
        if kind(p['type']) == 'string': code += [f'  sn_abi_v1_release(w{i});']
    if status:
        message = json.dumps(f"native package '{package}' export '{symbol}' failed: %s\n")
        code += [f'  if (status != SN_ABI_OK) {{ fprintf(stderr, {message}, sn_abi_v1_status_message(status)); exit(1); }}']
    if result == 'string':
        code += ['  SnAbiBytes bytes;', '  if (sn_abi_v1_bytes(wire_result, &bytes) != SN_ABI_OK) abort();',
                 '  char *output = NULL;', '  if (bytes.data) { output = malloc((size_t)bytes.length + 1);',
                 '    if (!output) abort(); memcpy(output, bytes.data, (size_t)bytes.length); output[bytes.length] = 0; }',
                 '  sn_abi_v1_release(wire_result);', '  return output;']
    elif result != 'void':
        if result == 'bool': code += ['  if (wire_result > 1) abort();']
        code += [f'  return ({TYPES[result][0]})wire_result;']
    code += ['}']
    return '\n'.join(code)


def native_link_options(flags):
    """Keep paired driver options intact across individual @link pragmas."""
    result = []
    iterator = iter(flags)
    for flag in iterator:
        if flag in ('-framework', '-weak_framework', '-Xlinker', '-L', '-F', '-l'):
            argument = next(iterator, None)
            if argument is None:
                raise ValueError('native linker option is missing its argument: ' + flag)
            if flag in ('-framework', '-weak_framework'):
                result.append('-Wl,' + flag + ',' + argument)
            elif flag == '-Xlinker':
                result.append('-Wl,' + argument)
            else:
                result.append(flag + argument)
        else:
            result.append(flag)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contract', type=Path, required=True)
    parser.add_argument('--compiler', type=Path, required=True)
    args=parser.parse_args()
    request=json.loads(args.contract.read_text())
    source_lines=['#include <stdint.h>','#include <stdbool.h>','#include <stdlib.h>',
                  '#include <stdio.h>','#include <string.h>']
    sources,links,prototypes=[],[],[]
    compiler=args.compiler.resolve()
    runtime=compiler.parent/'lib'/('clang' if __import__('os').name=='nt' else 'gcc')/'libsn_runtime_min.a'
    header=compiler.parent/'include/runtime/sn_abi.h'
    source_lines.append('#include '+json.dumps(str(header).replace('\\','/')))
    go_units=0
    exports={}
    for package in request['packages']:
        for signature in package['signatures']:
            symbol=signature['binding']['symbol']
            previous=exports.get(symbol)
            if previous and previous!=package['manifest']:
                raise ValueError(f"native export symbol '{symbol}' is shared by packages '{previous}' and '{package['manifest']}'")
            exports[symbol]=package['manifest']
        # Validate ownership/representations before running a backing toolchain.
        adapters=[adapter(s,package['manifest']) for s in package['signatures']]
        for signature in package['signatures']:
            result=kind(signature['return_type'])
            parameters=', '.join(f'{TYPES[kind(p["type"])][0]} p{i}' for i,p in enumerate(signature['params'])) or 'void'
            prototypes.append(f'{TYPES[result][0]} {signature["adapter"]}({parameters});')
        build_args=SimpleNamespace(compiler=compiler,manifest=Path(package['manifest']),
                                  out_dir=args.contract.parent/'artifacts',target=request['target'],
                                  optimization=str(request['optimization']),arithmetic=request['arithmetic'])
        output=io.StringIO()
        with contextlib.redirect_stdout(output): build(build_args)
        built=json.loads(output.getvalue())
        assembly=Path(built['assembly'])
        metadata=json.loads(assembly.read_text())
        for unit in metadata['units']:
            if unit['language']=='GO': go_units+=1
            links.append(str(assembly.parent/unit['archive']))
            links += native_link_options(unit['native_link_flags'])
            links += ['-l'+name for name in unit['libraries']]
        source_lines+=adapters
    if go_units>1: raise ValueError('native package graph requires Go bridge aggregation, which is not implemented yet')
    links.append(str(runtime))
    source=args.contract.with_suffix('.c').resolve()
    source.write_text('\n\n'.join(source_lines)+'\n')
    sources.append(str(source))
    header_path=source.with_suffix('.h')
    header_path.write_text('#include <stdint.h>\n#include <stdbool.h>\n'+'\n'.join(prototypes)+'\n')
    Path(request['output']).write_text(json.dumps({'sources':sources,'includes':[str(header_path)],'links':list(dict.fromkeys(links))},indent=2)+'\n')


if __name__=='__main__':
    try: main()
    except (ValueError,OSError,KeyError) as error:
        raise SystemExit('error: native package import adapter: '+str(error))
