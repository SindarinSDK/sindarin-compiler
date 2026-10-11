#!/usr/bin/env python3
"""Generate consumer C ABI adapters from compiler-resolved package declarations."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
import re
import contextlib
import io
from build_native_package import build, build_go_graph, load_prebuilt, run

from native_contract import TYPES, kind, validate, raw_type, record_types
from native_provider import record_includes


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
    records = record_types([signature])
    code = [f'static void {alias}_record_destroy_{index}(void *value, uintptr_t context) {{ (void)context; {record["release"]}(value); }}'
            for index,record in enumerate(records)]
    code += [f'extern {return_wire} {symbol}({", ".join(wire_params) or "void"});',
             f'{raw_type(signature["return_type"])} {alias}(' + ', '.join(f'{raw_type(p["type"])} p{i}' for i,p in enumerate(params)) + ') {']
    live_arrays = signature.get('abi') == '1.5'
    version = 'SN_ABI_V1_5_VERSION' if live_arrays else 'SN_ABI_V1_1_VERSION' if signature.get('abi') == '1.1' else 'SN_ABI_V1_VERSION'
    arrays = result == 'string_array' or any(kind(p['type']) == 'string_array' for p in params)
    capabilities = 'SN_ABI_CAP_VALUES | SN_ABI_CAP_VALUE_ARRAYS' if arrays else 'SN_ABI_CAP_VALUES'
    if live_arrays and any(kind(p['type']) == 'string_array' for p in params):
        capabilities += ' | SN_ABI_CAP_NATIVE_STRING_ARRAYS'
    if records: capabilities += ' | SN_ABI_CAP_RESOURCES | SN_ABI_CAP_TYPED_RESOURCES'
    message = json.dumps(f"native package '{package}' requires a compatible shared runtime ABI\n")
    code += ['  SnAbiInfo abi_info;',
             f'  if (sn_abi_v1_query({version}, {capabilities}, &abi_info, sizeof(abi_info)) != SN_ABI_OK ||',
             '      abi_info.pointer_bits != sizeof(void *) * 8 || abi_info.int_bits != 64 ||',
             '      abi_info.char_bits != 8 || abi_info.float_bits != 32 || abi_info.double_bits != 64) {',
             f'    fprintf(stderr, {message}); exit(1);', '  }']
    for i,p in enumerate(params):
        if kind(p['type']) == 'string':
            code += [f'  SnAbiValue *w{i} = NULL;',
                     f'  if (sn_abi_v1_string_copy(p{i}, &w{i}) != SN_ABI_OK) abort();']
        elif kind(p['type']) == 'string_array':
            code += [f'  SnAbiValue *w{i} = NULL;']
            for previous in range(i):
                if kind(params[previous]['type']) == 'string_array':
                    code.append(f'  if (p{i} == p{previous}) w{i} = sn_abi_v1_retain(w{previous}); else')
            if live_arrays:
                code.append(f'  if (sn_abi_v1_native_string_array_borrow(p{i}, &w{i}) != SN_ABI_OK) abort();')
                continue
            code += [f'  if (p{i}) {{',
                     f'    if (sn_abi_v1_value_array_new(&w{i}) != SN_ABI_OK) abort();',
                     f'    for (long long slot = 0; slot < p{i}->len; slot++) {{ SnAbiValue *item = NULL;',
                     f'      if (sn_abi_v1_string_copy(((char **)p{i}->data)[slot], &item) != SN_ABI_OK) abort();',
                     f'      if (sn_abi_v1_value_array_push(w{i}, item) != SN_ABI_OK) abort();',
                     '      sn_abi_v1_release(item);', '    }', '  }']
        elif kind(p['type']) == 'record':
            record = p['type']['native_record']
            destroy = records.index(record)
            code += [f'  SnAbiValue *w{i} = NULL;', f'  if (p{i}) {{ {record["retain"]}(p{i});',
                     f'    if (sn_abi_v1_resource_new_typed({json.dumps(record["identity"])}, p{i}, {alias}_record_destroy_{destroy}, 0, &w{i}) != SN_ABI_OK) {{ {record["release"]}(p{i}); abort(); }}', '  }']
    arguments = [f'w{i}' if kind(p['type']) in ('string','string_array','record') else f'({TYPES[kind(p["type"])][1]})p{i}' for i,p in enumerate(params)]
    if result != 'void': code += [f'  {TYPES[result][1]} wire_result = {{0}};']
    if status:
        if result != 'void': arguments.append('&wire_result')
        code += [f'  uint32_t status = {symbol}({", ".join(arguments)});']
    else:
        call = f'{symbol}({", ".join(arguments)})'
        code += [('  wire_result = ' if result != 'void' else '  ') + call + ';']
    for i,p in enumerate(params):
        if kind(p['type']) in ('string','string_array','record'): code += [f'  sn_abi_v1_release(w{i});']
    if status:
        message = json.dumps(f"native package '{package}' export '{symbol}' failed: %s\n")
        code += [f'  if (status != SN_ABI_OK) {{ fprintf(stderr, {message}, sn_abi_v1_status_message(status)); exit(1); }}']
    if result == 'string':
        code += ['  SnAbiBytes bytes;', '  if (sn_abi_v1_bytes(wire_result, &bytes) != SN_ABI_OK) abort();',
                 '  char *output = NULL;', '  if (bytes.data) { output = malloc((size_t)bytes.length + 1);',
                 '    if (!output) abort(); memcpy(output, bytes.data, (size_t)bytes.length); output[bytes.length] = 0; }',
                 '  sn_abi_v1_release(wire_result);', '  return output;']
    elif result == 'string_array':
        code += ['  if (!wire_result) return NULL;', '  uint64_t length = 0;',
                 '  if (sn_abi_v1_value_array_length(wire_result, &length) != SN_ABI_OK || length > INT64_MAX) abort();',
                 '  SnArray *output = sn_array_new(sizeof(char *), (long long)length);',
                 '  output->elem_tag = SN_TAG_STRING; output->elem_release = sn_package_string_slot_release;',
                 '  output->elem_copy = sn_package_string_slot_copy;',
                 '  for (uint64_t i = 0; i < length; i++) { SnAbiValue *element = NULL; SnAbiBytes bytes;',
                 '    if (sn_abi_v1_value_array_get(wire_result, i, &element) != SN_ABI_OK || sn_abi_v1_string_bytes(element, &bytes) != SN_ABI_OK) abort();',
                 '    char *text = NULL; if (bytes.data) { text = malloc((size_t)bytes.length + 1); if (!text) abort();',
                 '      memcpy(text, bytes.data, (size_t)bytes.length); text[bytes.length] = 0; }',
                 '    sn_array_push(output, &text); sn_abi_v1_release(element);', '  }',
                 '  sn_abi_v1_release(wire_result); return output;']
    elif result == 'record':
        record = signature['return_type']['native_record']
        code += ['  void *pointer = NULL;',
                 f'  if (sn_abi_v1_resource_data_typed(wire_result, {json.dumps(record["identity"])}, &pointer) != SN_ABI_OK) abort();',
                 f'  {raw_type(signature["return_type"])} output = {record["retain"]}(pointer);',
                 '  sn_abi_v1_release(wire_result); return output;']
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
    record_headers=[]
    compiler=args.compiler.resolve()
    runtime=compiler.parent/'lib'/('clang' if __import__('os').name=='nt' else 'gcc')/'libsn_runtime_min.a'
    header=compiler.parent/'include/runtime/sn_abi.h'
    source_lines.append('#include '+json.dumps(str(header).replace('\\','/')))
    source_lines.append('#include '+json.dumps(str(header.with_name('sn_array.h')).replace('\\','/')))
    source_lines += ['static void sn_package_string_slot_release(void *p) { free(*(char **)p); }',
                     'static void sn_package_string_slot_copy(const void *s, void *d) { *(char **)d = *(char *const *)s ? strdup(*(char *const *)s) : NULL; }']
    plans=[]
    for package in request['packages']:
        plan=json.loads(run([compiler,'--native-plan',Path(package['manifest']).resolve(),
                             '--target',request['target'],'-O'+str(request['optimization']),
                             '--'+request['arithmetic']]))
        plans.append({'manifest':package['manifest'],'plan':plan})
    prebuilt_go=sum(any(u['language']=='GO' for u in p['plan']['native']['builds'])
                    for p in plans if 'assembly' in p['plan']['native'])
    source_go=sum(u['language']=='GO' for p in plans if 'assembly' not in p['plan']['native'] for u in p['plan']['native']['builds'])
    if prebuilt_go and (prebuilt_go>1 or source_go):
        raise ValueError('prebuilt Go runtime archives cannot be combined with other Go packages; rebuild one aggregate source graph')
    aggregate=source_go>1
    exports={}
    for package,planned in zip(request['packages'],plans):
        headers=record_includes(package['signatures'],Path(package['manifest']).resolve().parent)
        source_lines+=headers
        record_headers+=headers
        for record in planned['plan']['native'].get('types',[]):
            for role in ('create','retain','release','refs'):
                symbol=record[role]
                previous=exports.get(symbol)
                if previous and previous!=package['manifest']:
                    raise ValueError(f"native record lifecycle symbol '{symbol}' is shared by packages '{previous}' and '{package['manifest']}'")
                exports[symbol]=package['manifest']
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
            parameters=', '.join(f'{raw_type(p["type"])} p{i}' for i,p in enumerate(signature['params'])) or 'void'
            prototypes.append(f'{raw_type(signature["return_type"])} {signature["adapter"]}({parameters});')
        build_args=SimpleNamespace(compiler=compiler,manifest=Path(package['manifest']),
                                  out_dir=args.contract.parent/'artifacts',target=request['target'],
                                  optimization=str(request['optimization']),arithmetic=request['arithmetic'],
                                  validated_plan=planned['plan'],skip_go=aggregate)
        if 'assembly' in planned['plan']['native']:
            metadata,assembly=load_prebuilt(planned['plan'],package['manifest'],compiler)
        else:
            output=io.StringIO()
            with contextlib.redirect_stdout(output): build(build_args)
            built=json.loads(output.getvalue())
            assembly=Path(built['assembly'])
            metadata=json.loads(assembly.read_text())
        for unit in metadata['units']:
            links.append(str(assembly.parent/unit['archive']))
            links += native_link_options(unit['native_link_flags'])
            links += ['-l'+name for name in unit['libraries']]
        source_lines+=adapters
    if aggregate:
        graph=build_go_graph([p for p in plans if 'assembly' not in p['plan']['native']],compiler,args.contract.parent/'go-graphs',
                             str(request['optimization']),request['arithmetic'])
        links.append(graph['archive'])
        links+=native_link_options(graph['native_link_flags'])
    links.append(str(runtime))
    source=args.contract.with_suffix('.c').resolve()
    source.write_text('\n\n'.join(source_lines)+'\n')
    sources.append(str(source))
    header_path=source.with_suffix('.h')
    header_path.write_text('#include <stdint.h>\n#include <stdbool.h>\n#include '+json.dumps(str(header.with_name('sn_array.h')).replace('\\','/'))+'\n'+'\n'.join(list(dict.fromkeys(record_headers))+prototypes)+'\n')
    Path(request['output']).write_text(json.dumps({'sources':sources,'includes':[str(header_path)],'links':list(dict.fromkeys(links))},indent=2)+'\n')


if __name__=='__main__':
    try: main()
    except (ValueError,OSError,KeyError) as error:
        raise SystemExit('error: native package import adapter: '+str(error))
