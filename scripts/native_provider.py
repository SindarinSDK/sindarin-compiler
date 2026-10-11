"""Generate provider exports for declared ordinary backing-language functions."""
import json
import re
from native_contract import TYPES, kind, validate, raw_type, record_types, c_array_shape

RUST = {'int':'i64', 'long':'i64', 'uint':'u64', 'int32':'i32', 'uint32':'u32',
        'byte':'u8', 'char':'u8', 'bool':'bool', 'float':'f32', 'double':'f64',
        'string':'Option<Vec<u8>>', 'void':'()', 'string_array':'Option<Vec<Option<Vec<u8>>>>',
        'byte_array':'Option<Vec<u8>>'}
GO = {'int':'int64', 'long':'int64', 'uint':'uint64', 'int32':'int32', 'uint32':'uint32',
      'byte':'uint8', 'char':'uint8', 'bool':'bool', 'float':'float32', 'double':'float64',
      'string':'*string', 'void':'', 'string_array':'[]*string', 'byte_array':'[]byte'}


def contracts(native, unit):
    signatures = [s for s in native.get('signatures', []) if s['binding']['build'] == unit['name']]
    expected = [b for b in native['bindings'] if b['build'] == unit['name'] and 'function' in b]
    if len(signatures) != len(expected):
        raise ValueError('generated provider requires compiler-resolved native declarations')
    symbols = set()
    records = record_types(signatures)
    if records and unit['language'] not in ('C','SN'):
        raise ValueError('package-owned record provider generation currently requires C backing')
    lifecycle_functions = {r[field] for r in records for field in ('create','retain','release','refs')}
    for s in signatures:
        validate(s)
        if unit['language'] in ('RS','GO') and any(kind(t)=='native_array' for t in
            [s['return_type']]+[p['type'] for p in s['params']]):
            raise ValueError('typed native array provider generation requires canonical C storage; RS/GO provider contracts remain unsupported')
        binding = s['binding']
        if binding['function'] in lifecycle_functions:
            raise ValueError('record lifecycle functions must keep their public C symbols; use a separate backing function')
        for field in ('function', 'symbol'):
            pattern = r'[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)?' if field == 'function' and unit['language'] == 'SN' else r'[A-Za-z_][A-Za-z0-9_]*'
            if not re.fullmatch(pattern, binding[field]):
                raise ValueError(f'generated provider {field} must be a root-level function identifier')
        if binding['symbol'] in symbols:
            raise ValueError('duplicate generated provider export symbol')
        symbols.add(binding['symbol'])
        if unit['language'] == 'C' and binding['symbol'] == binding['function']:
            raise ValueError('generated C export symbol must differ from its backing function')
    if signatures and len(expected) != len([b for b in native['bindings'] if b['build'] == unit['name']]):
        raise ValueError('one native build unit cannot mix generated and handwritten exports')
    return signatures


def c_declaration(signature):
    b, params, result = signature['binding'], signature['params'], kind(signature['return_type'])
    status = b['failure'] == 'status'
    raw = [raw_type(p['type']) for p in params]
    if status and result != 'void': raw.append(raw_type(signature['return_type']) + ' *')
    return f'extern {"uint32_t" if status else raw_type(signature["return_type"])} {b["function"]}({", ".join(raw) or "void"});'


def record_includes(signatures, root):
    from pathlib import Path
    return ['#include ' + json.dumps(str((Path(root)/r['header']).resolve()).replace('\\','/'))
            for r in record_types(signatures)]


def record_layouts(signatures, namespace):
    values = {}
    records = record_types(signatures)
    for signature in signatures:
        for value in [signature['return_type']] + [p['type'] for p in signature['params']]:
            if kind(value) == 'record': values[value['native_record']['identity']] = value
    def field_type(value):
        name = value['kind']
        # Storage follows the established C record declaration, whose uint
        # members use unsigned long long even on LP64 hosts with uint64_t=long.
        if name == 'uint': return 'unsigned long long'
        if name == 'pointer': return field_type(value['base_type']) + ' *'
        if name == 'opaque': return value['name']
        if name == 'array': return 'SnArray *'
        if name == 'struct':
            found = [r for r in records if r['declaration'].split('::')[1] == value['name']]
            if len(found) != 1:
                raise ValueError('record fields require a resolved canonical C type contract: '+value['name'])
            return found[0]['c_type']+' *'
        return raw_type(value)
    lines = ['#include <stddef.h>'] if records else []
    for index,record in enumerate(records):
        layout = f'__sn_{namespace}_record_layout_{index}'
        fields = values[record['identity']]['fields']
        lines += ['typedef struct { int __rc__;']
        for field in fields:
            name = field.get('c_alias','__sn__'+field['name'])
            lines.append(f'  {field_type(field["type"])} {name};')
        lines += [f'}} {layout};',
                  f'_Static_assert(sizeof({record["c_type"]}) == sizeof({layout}) && _Alignof({record["c_type"]}) == _Alignof({layout}), "package record size/alignment differs");']
        for field in fields:
            name = field.get('c_alias','__sn__'+field['name'])
            lines += [f'_Static_assert(offsetof({record["c_type"]}, {name}) == offsetof({layout}, {name}), "package record field offset differs");',
                      f'_Static_assert(_Generic(&(({record["c_type"]} *)0)->{name}, {field_type(field["type"])} *: 1, default: 0), "package record field type differs");']
    return lines


def c_header(signatures, namespace, root='.'):
    # Preserve source while giving declared backing functions package-private
    # link identities. Otherwise a second archive's ordinary echo() can silently
    # resolve to the first package's echo(), even when wire exports are distinct.
    functions = sorted({s['binding']['function'] for s in signatures})
    names = [f'#define {function} __sn_{namespace}_impl_{function}' for function in functions]
    return '#include <stdint.h>\n#include <stdbool.h>\n#include \"sn_array.h\"\n'+'\n'.join(record_includes(signatures,root)+record_layouts(signatures,namespace)+names+[c_declaration(s) for s in signatures])+'\n'


def c_provider(signatures, namespace, root='.'):
    # Backing units use macros to namespace ordinary function identifiers. The
    # provider calls those final names explicitly; macros must not rename its
    # locals when a backing function happens to be called value/status/out.
    lines = ['#undef '+name for name in sorted({s['binding']['function'] for s in signatures})]
    signatures = [dict(s,binding=dict(s['binding'],function=f'__sn_{namespace}_impl_{s["binding"]["function"]}')) for s in signatures]
    lines += ['#include <stdint.h>', '#include <stdbool.h>', '#include <stdlib.h>', '#include "sn_abi.h"']
    records = record_types(signatures)
    lines += record_includes(signatures,root)
    for index, record in enumerate(records):
        lines.append(f'static void sn_package_record_destroy_{index}(void *value, uintptr_t context) {{ (void)context; {record["release"]}(value); }}')
    if any(s.get('abi') not in ('1.5','1.6','1.7') and kind(p['type']) == 'string_array' for s in signatures for p in s['params']):
        lines += ['#include <string.h>',
                  'static void sn_package_borrowed_slot_free(void *p) { free(*(char **)p); }',
                  'static void sn_package_borrowed_slot_copy(const void *s, void *d) {',
                  '  const char *text = *(char *const *)s; *(char **)d = text ? strdup(text) : NULL;',
                  '  if (text && !*(char **)d) abort(); }',
                  'static uint32_t sn_package_borrow_array(SnAbiValue *input, SnArray **out) {',
                  '  if (!input) { *out = NULL; return SN_ABI_OK; }',
                  '  uint64_t length = 0; uint32_t code = sn_abi_v1_value_array_length(input, &length);',
                  '  if (code) return code; if (length > INT64_MAX) return SN_ABI_OUT_OF_RANGE;',
                  '  SnArray *array = sn_array_new(sizeof(char *), (long long)length);',
                  '  array->elem_tag = SN_TAG_STRING; array->elem_release = sn_package_borrowed_slot_free;',
                  '  array->elem_copy = sn_package_borrowed_slot_copy;',
                  '  for (uint64_t slot = 0; slot < length; slot++) { SnAbiValue *item = NULL; SnAbiBytes bytes;',
                  '    code = sn_abi_v1_value_array_get(input, slot, &item);',
                  '    if (!code) code = sn_abi_v1_string_bytes(item, &bytes);',
                  '    if (code) { sn_abi_v1_release(item); sn_array_free(array); return code; }',
                  '    char *text = bytes.data ? strdup((const char *)bytes.data) : NULL;',
                  '    if (bytes.data && !text) abort(); sn_array_push(array, &text); sn_abi_v1_release(item);',
                  '  } *out = array; return SN_ABI_OK;', ' }']
    for s in signatures:
        b, params, result = s['binding'], s['params'], kind(s['return_type'])
        status = b['failure'] == 'status'
        lines.append(c_declaration(s))
        wire = [f'{TYPES[kind(p["type"])][1]} p{i}' for i,p in enumerate(params)]
        if status and result != 'void': wire.append(TYPES[result][1] + ' *out')
        lines.append(f'{"uint32_t" if status else TYPES[result][1]} {b["symbol"]}({", ".join(wire) or "void"}) {{')
        if status and result != 'void': lines.append('  if (!out) return SN_ABI_INVALID_ARGUMENT;')
        array_params = [i for i,p in enumerate(params) if kind(p['type']) in ('string_array','byte_array','native_array')]
        record_params = [i for i,p in enumerate(params) if kind(p['type']) == 'record']
        string_params = [i for i,p in enumerate(params) if kind(p['type']) == 'string']
        managed_params = sorted(array_params + record_params + string_params)
        for i in array_params: lines.append(f'  SnArray *array{i} = NULL;')
        for i in record_params: lines.append(f'  void *record{i} = NULL;')
        guarded = managed_params if s.get('abi') in ('1.5','1.6','1.7') else sorted(record_params + string_params)
        for i in guarded: lines.append(f'  SnAbiValue *argument_owner{i} = sn_abi_v1_retain(p{i});')
        if managed_params: lines.append('  uint32_t provider_status = 0;')
        def failed(code):
            if managed_params: return 'provider_status = '+code+'; goto cleanup_args;'
            return 'return '+code+';' if status else 'abort();'
        arguments = []
        for i,p in enumerate(params):
            k = kind(p['type'])
            if k == 'string':
                lines += [f'  SnAbiBytes bytes{i};', f'  uint32_t status{i} = sn_abi_v1_string_bytes(p{i}, &bytes{i});',
                          f'  if (status{i}) {{ ' + failed(f'status{i}') + ' }']
                arguments.append(f'(char *)bytes{i}.data')
            elif k == 'native_array':
                lines += [f'  provider_status = sn_abi_v1_native_array_data(p{i}, {c_array_shape(p["type"])}, &array{i});',
                          '  if (provider_status) goto cleanup_args;']
                arguments.append(f'array{i}')
            elif k in ('string_array','byte_array'):
                if s.get('abi') not in ('1.5','1.6','1.7'):
                    for previous in array_params:
                        if previous >= i: break
                        lines.append(f'  if (p{i} == p{previous}) array{i} = array{previous}; else')
                operation = 'sn_abi_v1_native_byte_array_data' if k == 'byte_array' else 'sn_abi_v1_native_string_array_data' if s.get('abi') in ('1.5','1.6','1.7') else 'sn_package_borrow_array'
                lines += [f'  provider_status = {operation}(p{i}, &array{i});',
                          '  if (provider_status) goto cleanup_args;']
                arguments.append(f'array{i}')
            elif k == 'record':
                record = p['type']['native_record']
                lines += [f'  provider_status = sn_abi_v1_resource_data_typed(p{i}, {json.dumps(record["identity"])}, &record{i});',
                          '  if (provider_status) goto cleanup_args;']
                arguments.append(f'({raw_type(p["type"])})record{i}')
            else:
                if k == 'bool': lines.append(f'  if (p{i} > 1) {{ '+failed('SN_ABI_INVALID_ARGUMENT')+' }')
                arguments.append(f'({TYPES[k][0]})p{i}')
        if result != 'void': lines.append(f'  {raw_type(s["return_type"])} value = {{0}};')
        if status:
            if result != 'void': arguments.append('&value')
            if managed_params: lines.append(f'  provider_status = {b["function"]}({", ".join(arguments)});')
            else: lines += [f'  uint32_t status = {b["function"]}({", ".join(arguments)});', '  if (status) return status;']
        else: lines.append(('  value = ' if result != 'void' else '  ')+f'{b["function"]}({", ".join(arguments)});')
        if result == 'record' and b['ownership']['result'] == 'borrowed':
            record = s['return_type']['native_record']
            lines.append(('  if (!provider_status) ' if managed_params else '  ') + f'value = {record["retain"]}(value);')
        if managed_params:
            lines.append('cleanup_args:')
            for i in reversed(guarded): lines.append(f'  sn_abi_v1_release(argument_owner{i});')
            if s.get('abi') not in ('1.5','1.6','1.7'):
                for position,i in reversed(list(enumerate(array_params))):
                    unique = ' && '.join(f'array{i} != array{j}' for j in array_params[:position])
                    lines.append(('  if ('+unique+') ' if unique else '  ')+f'sn_array_free(array{i});')
            lines.append('  if (provider_status) { '+('return provider_status;' if status else 'abort();')+' }')
        if result == 'string':
            lines += ['  SnAbiValue *wire_value = NULL;', '  uint32_t status_copy = sn_abi_v1_string_copy(value, &wire_value);',
                      '  free(value);', '  if (status_copy) { '+('return status_copy;' if status else 'abort();')+' }']
            value = 'wire_value'
        elif result == 'string_array':
            lines += ['  SnAbiValue *wire_value = NULL;',
                      '  if (value) { uint32_t code = sn_abi_v1_value_array_new(&wire_value);',
                      '    for (long long i=0; !code && i<value->len; i++) { SnAbiValue *item = NULL;',
                      '      code = sn_abi_v1_string_copy(((char **)value->data)[i], &item);',
                      '      if (!code) code = sn_abi_v1_value_array_push(wire_value, item);',
                      '      sn_abi_v1_release(item);', '    }', '    sn_array_free(value);',
                      '    if (code) { sn_abi_v1_release(wire_value); '+('return code;' if status else 'abort();')+' }', '  }']
            value = 'wire_value'
        elif result == 'byte_array':
            lines += ['  SnAbiValue *wire_value = NULL;',
                      '  uint32_t status_copy = sn_abi_v1_native_byte_array_adopt(value, &wire_value);',
                      '  if (status_copy) { sn_array_free(value); '+('return status_copy;' if status else 'abort();')+' }']
            value = 'wire_value'
        elif result == 'record':
            record = s['return_type']['native_record']
            destroy = records.index(record)
            lines += ['  SnAbiValue *wire_value = NULL;',
                      f'  uint32_t status_copy = value ? sn_abi_v1_resource_new_typed({json.dumps(record["identity"])}, value, sn_package_record_destroy_{destroy}, 0, &wire_value) : SN_ABI_OK;',
                      f'  if (status_copy) {{ {record["release"]}(value); '+('return status_copy;' if status else 'abort();')+' }']
            value = 'wire_value'
        elif result == 'native_array':
            lines += ['  SnAbiValue *wire_value = NULL;',
                      f'  uint32_t status_copy = sn_abi_v1_native_array_adopt(value, {c_array_shape(s["return_type"])}, &wire_value);',
                      '  if (status_copy) { sn_array_free(value); '+('return status_copy;' if status else 'abort();')+' }']
            value = 'wire_value'
        else: value = f'({TYPES[result][1]})value'
        if status:
            if result != 'void': lines.append(f'  *out = {value};')
            lines.append('  return SN_ABI_OK;')
        elif result != 'void': lines.append(f'  return {value};')
        lines.append('}')
    return '\n'.join(lines)+'\n'


def rust_provider(signatures, backing_crate):
    lines = [f'extern crate {backing_crate} as backing;',
             '#[repr(C)] pub struct V { _opaque: [u8; 0] }',
             '#[repr(C)] struct Bytes { data: *const u8, length: u64 }',
             'extern "C" { fn sn_abi_v1_string_bytes(v: *const V, out: *mut Bytes) -> u32;',
             'fn sn_abi_v1_string_copy(v: *const std::ffi::c_char, out: *mut *mut V) -> u32;',
             'fn sn_abi_v1_value_array_new(out: *mut *mut V) -> u32; fn sn_abi_v1_value_array_push(array: *mut V, value: *mut V) -> u32;',
             'fn sn_abi_v1_value_array_length(array: *const V, out: *mut u64) -> u32;',
             'fn sn_abi_v1_value_array_get(array: *const V, index: u64, out: *mut *mut V) -> u32;',
             'fn sn_abi_v1_native_string_array_data(value: *const V, out: *mut *mut std::ffi::c_void) -> u32;',
             'fn sn_abi_v1_native_byte_array_data(value: *const V, out: *mut *mut std::ffi::c_void) -> u32;',
             'fn sn_abi_v1_native_byte_array_adopt(value: *mut std::ffi::c_void, out: *mut *mut V) -> u32;',
             'fn sn_abi_v1_native_byte_array_copy_bytes(data: *const u8, length: u64, out: *mut *mut V) -> u32;',
             'fn sn_abi_v1_retain(value: *mut V) -> *mut V;',
             'fn sn_abi_v1_release(value: *mut V); }',
             'struct OwnedV(*mut V); impl Drop for OwnedV { fn drop(&mut self) { unsafe { sn_abi_v1_release(self.0) } } }']
    for s in signatures:
        b, params, result = s['binding'], s['params'], kind(s['return_type'])
        status = b['failure'] == 'status'
        wire = lambda k: '*mut V' if k in ('string','string_array','byte_array') else 'u8' if k == 'bool' else RUST[k]
        declarations = [f'p{i}: {wire(kind(p["type"]))}' for i,p in enumerate(params)]
        if status and result != 'void': declarations.append(f'out: *mut {wire(result)}')
        lines += [f'#[no_mangle] pub unsafe extern "C" fn {b["symbol"]}({", ".join(declarations)}) -> '+('u32' if status else wire(result))+' {']
        if status and result != 'void': lines.append('  if out.is_null() { return 1; }')
        lines.append(f'  let call = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> Result<{wire(result)}, u32> {{')
        arguments = []
        for i,p in enumerate(params):
            k = kind(p['type'])
            if k == 'string':
                lines += [f'    let _owner{i} = OwnedV(sn_abi_v1_retain(p{i}));',
                          f'    let mut bytes{i} = Bytes {{ data: std::ptr::null(), length: 0 }};',
                          f'    let status = sn_abi_v1_string_bytes(p{i}, &mut bytes{i}); if status != 0 {{ return Err(status); }}',
                          f'    let length{i} = usize::try_from(bytes{i}.length).map_err(|_| 5u32)?;',
                          f'    if length{i} > isize::MAX as usize {{ return Err(5); }}',
                          f'    let arg{i} = if bytes{i}.data.is_null() {{ None }} else {{ Some(std::slice::from_raw_parts(bytes{i}.data, length{i})) }};']
                arguments.append(f'arg{i}')
            elif k in ('string_array','byte_array'):
                if s.get('abi') in ('1.5','1.6','1.7'):
                    operation='native_byte_array_data' if k=='byte_array' else 'native_string_array_data'
                    lines += [f'    let _owner{i} = OwnedV(sn_abi_v1_retain(p{i}));',
                              f'    let mut arg{i} = std::ptr::null_mut();',
                              f'    let code = sn_abi_v1_{operation}(p{i}, &mut arg{i}); if code != 0 {{ return Err(code); }}']
                    arguments.append(f'arg{i}')
                    continue
                lines += [f'    let mut count{i} = 0u64;',
                          f'    let code = sn_abi_v1_value_array_length(p{i}, &mut count{i}); if code != 0 {{ return Err(code); }}',
                          f'    let length{i} = usize::try_from(count{i}).map_err(|_| 5u32)?;',
                          f'    if length{i} > isize::MAX as usize / std::mem::size_of::<Option<&[u8]>>() {{ return Err(5); }}',
                          f'    let mut owners{i} = Vec::with_capacity(length{i}); let mut values{i} = Vec::with_capacity(length{i}.max(1));',
                          f'    for slot in 0..count{i} {{ let mut item = OwnedV(std::ptr::null_mut());',
                          f'      let code = sn_abi_v1_value_array_get(p{i}, slot, &mut item.0); if code != 0 {{ return Err(code); }}',
                          '      let mut bytes = Bytes { data: std::ptr::null(), length: 0 };',
                          '      let code = sn_abi_v1_string_bytes(item.0, &mut bytes); if code != 0 { return Err(code); }',
                          '      let n = usize::try_from(bytes.length).map_err(|_| 5u32)?; if n > isize::MAX as usize { return Err(5); }',
                          f'      values{i}.push(if bytes.data.is_null() {{ None }} else {{ Some(std::slice::from_raw_parts(bytes.data, n)) }}); owners{i}.push(item);',
                          '    }']
                aliases = ''.join(f'if p{i} == p{j} {{ arg{j} }} else ' for j in range(i) if kind(params[j]['type']) == 'string_array')
                lines.append(f'    let arg{i} = '+aliases+f'if p{i}.is_null() {{ None }} else {{ Some(values{i}.as_slice()) }};')
                arguments.append(f'arg{i}')
            elif k == 'bool':
                lines.append(f'    if p{i} > 1 {{ return Err(1); }}')
                arguments.append(f'p{i} != 0')
            else: arguments.append(f'p{i}')
        lines.append(f'    let value = backing::{b["function"]}({", ".join(arguments)})'+('?' if status else '')+';')
        if result == 'string':
            lines += ['    let mut output = std::ptr::null_mut();',
                      '    let text = value.map(|v| { let n = v.iter().position(|b| *b == 0).unwrap_or(v.len()); std::ffi::CString::new(&v[..n]).unwrap() });',
                      '    let status = sn_abi_v1_string_copy(text.as_ref().map_or(std::ptr::null(), |t| t.as_ptr()), &mut output);',
                      '    if status != 0 { return Err(status); }', '    Ok(output)']
        elif result == 'string_array':
            lines += ['    let mut output = OwnedV(std::ptr::null_mut());',
                      '    if let Some(values) = value {',
                      '      let code = sn_abi_v1_value_array_new(&mut output.0); if code != 0 { return Err(code); }',
                      '      for value in values { let mut item = OwnedV(std::ptr::null_mut());',
                      '        let text = value.map(|v| { let n=v.iter().position(|b| *b==0).unwrap_or(v.len()); std::ffi::CString::new(&v[..n]).unwrap() });',
                      '        let code=sn_abi_v1_string_copy(text.as_ref().map_or(std::ptr::null(),|t|t.as_ptr()), &mut item.0); if code!=0 { return Err(code); }',
                      '        let code=sn_abi_v1_value_array_push(output.0,item.0); if code!=0 { return Err(code); }',
                      '      }', '    }', '    let pointer=output.0; std::mem::forget(output); Ok(pointer)']
        elif result == 'byte_array':
            if s.get('native_byte_wire_result'):
                lines += ['    Ok(value.cast::<V>())']
            else:
                lines += ['    let mut output = std::ptr::null_mut();',
                          '    let (data, length) = value.as_ref().map_or((std::ptr::null(),0), |v| (v.as_ptr(),v.len() as u64));',
                          '    let code = sn_abi_v1_native_byte_array_copy_bytes(data,length,&mut output);',
                          '    if code != 0 { return Err(code); }', '    Ok(output)']
        elif result == 'bool': lines.append('    Ok(u8::from(value))')
        else: lines.append('    Ok(value)')
        lines += ['  }));', '  match call {', '    Ok(Ok(value)) => {']
        if status:
            if result != 'void': lines.append('      *out = value;')
            lines.append('      0')
        else: lines.append('      value')
        lines += ['    },', '    Ok(Err(_status)) => '+('_status' if status else 'std::process::abort()')+',',
                  '    Err(_) => '+('6' if status else 'std::process::abort()')+',', '  }', '}']
    return '\n'.join(lines)+'\n'


def go_provider(signatures, module, main=True):
    lines = ['package main', '/* #include <stdint.h>\n#include <stdlib.h>\n#include "sn_abi.h" */', 'import "C"',
             f'import {"backing" if signatures else "_"} {json.dumps(module)}']
    if any(kind(p['type']) in ('string','string_array','byte_array') for s in signatures for p in s['params']) or any(kind(s['return_type']) in ('string','string_array','byte_array') for s in signatures):
        lines.append('import "unsafe"')
    wire = lambda k: '*C.SnAbiValue' if k in ('string','string_array','byte_array') else 'C.'+TYPES[k][1] if k != 'void' else ''
    for s in signatures:
        b, params, result = s['binding'], s['params'], kind(s['return_type'])
        status = b['failure'] == 'status'
        declarations = [f'p{i} {wire(kind(p["type"]))}' for i,p in enumerate(params)]
        if status and result != 'void': declarations.append(f'out *{wire(result)}')
        returns = ' (status C.uint32_t)' if status else (' '+wire(result) if result != 'void' else '')
        lines += [f'//export {b["symbol"]}', f'func {b["symbol"]}({", ".join(declarations)}){returns} {{']
        normal_return = lambda expression: 'completed = true; return ' + expression
        if status:
            lines += ['  completed := false', '  defer func() { if !completed { _ = recover(); status = 6 } }()']
            if result != 'void': lines.append('  if out == nil { '+normal_return('1')+' }')
        arguments = []
        for i,p in enumerate(params):
            k = kind(p['type'])
            if k == 'string':
                lines += [f'  owner{i} := C.sn_abi_v1_retain(p{i}); defer C.sn_abi_v1_release(owner{i})',
                          f'  var bytes{i} C.SnAbiBytes', f'  if code := C.sn_abi_v1_string_bytes(p{i}, &bytes{i}); code != 0 {{ '+(normal_return('code') if status else 'panic("invalid string ABI")')+' }',
                          f'  var arg{i} *string', f'  if bytes{i}.data != nil {{',
                          f'    if uint64(bytes{i}.length) > uint64(^uint(0)>>1) {{ '+(normal_return('5') if status else 'panic("string length overflow")')+' }',
                          f'    text := string(unsafe.Slice((*byte)(unsafe.Pointer(bytes{i}.data)), int(bytes{i}.length))); arg{i} = &text', '  }']
                arguments.append(f'arg{i}')
            elif k in ('string_array','byte_array'):
                error = lambda code: normal_return(code) if status else 'panic("invalid array ABI")'
                if s.get('abi') in ('1.5','1.6','1.7'):
                    operation='native_byte_array_data' if k=='byte_array' else 'native_string_array_data'
                    lines += [f'  owner{i} := C.sn_abi_v1_retain(p{i}); defer C.sn_abi_v1_release(owner{i})',
                              f'  var arg{i} *C.struct_SnArray',
                              f'  if code := C.sn_abi_v1_{operation}(p{i}, &arg{i}); code != 0 {{ '+error('code')+' }']
                    arguments.append(f'unsafe.Pointer(arg{i})')
                    continue
                lines += [f'  var count{i} C.uint64_t',
                          f'  if code := C.sn_abi_v1_value_array_length(p{i}, &count{i}); code != 0 {{ '+error('code')+' }',
                          f'  if uint64(count{i}) > uint64(^uint(0)>>1) {{ '+error('5')+' }',
                          f'  var arg{i} []*string; if p{i} != nil {{ capacity := int(count{i}); if capacity == 0 {{ capacity = 1 }}; arg{i} = make([]*string, int(count{i}), capacity) }}',
                          f'  for slot := uint64(0); slot < uint64(count{i}); slot++ {{ var item *C.SnAbiValue; var bytes C.SnAbiBytes',
                          f'    code := C.sn_abi_v1_value_array_get(p{i}, C.uint64_t(slot), &item)',
                          '    if code == 0 { code = C.sn_abi_v1_string_bytes(item, &bytes) }',
                          '    if code != 0 { C.sn_abi_v1_release(item); '+error('code')+' }',
                          '    if uint64(bytes.length) > uint64(^uint(0)>>1) { C.sn_abi_v1_release(item); '+error('5')+' }',
                          '    if bytes.data != nil { text := string(unsafe.Slice((*byte)(unsafe.Pointer(bytes.data)), int(bytes.length)));',
                          f'      arg{i}[slot] = &text }}', '    C.sn_abi_v1_release(item)', '  }']
                for previous in range(i):
                    if kind(params[previous]['type']) == 'string_array':
                        lines.append(f'  if p{i} == p{previous} {{ arg{i} = arg{previous} }}')
                arguments.append(f'arg{i}')
            elif k == 'bool':
                lines.append(f'  if p{i} > 1 {{ '+(normal_return('1') if status else 'panic("invalid boolean ABI")')+' }')
                arguments.append(f'p{i} != 0')
            else: arguments.append(f'{GO[k]}(p{i})')
        call = f'backing.{b["function"]}({", ".join(arguments)})'
        if status:
            lines.append(('  value, code := ' if result != 'void' else '  code := ')+call)
            lines.append('  if code != 0 { '+normal_return('C.uint32_t(code)')+' }')
        else: lines.append(('  value := ' if result != 'void' else '  ')+call)
        if result == 'string':
            lines += ['  var output *C.SnAbiValue', '  var text *C.char', '  if value != nil { text = C.CString(*value); defer C.free(unsafe.Pointer(text)) }',
                      '  if code := C.sn_abi_v1_string_copy(text, &output); code != 0 { '+(normal_return('code') if status else 'panic("string result ABI")')+' }']
            value = 'output'
        elif result == 'string_array':
            lines += ['  var output *C.SnAbiValue', '  adopted := false',
                      '  defer func() { if !adopted { C.sn_abi_v1_release(output) } }()',
                      '  if value != nil {',
                      '    if code:=C.sn_abi_v1_value_array_new(&output); code!=0 { '+(normal_return('code') if status else 'panic("array result ABI")')+' }',
                      '    for _, entry := range value { var text *C.char; var item *C.SnAbiValue',
                      '      if entry != nil { text=C.CString(*entry) }',
                      '      code:=C.sn_abi_v1_string_copy(text,&item); C.free(unsafe.Pointer(text))',
                      '      if code==0 { code=C.sn_abi_v1_value_array_push(output,item) }; C.sn_abi_v1_release(item)',
                      '      if code!=0 { '+(normal_return('code') if status else 'panic("array element ABI")')+' }',
                      '    }', '  }', '  adopted = true']
            value = 'output'
        elif result == 'byte_array':
            lines += ['  var output *C.SnAbiValue', '  var data *C.uint8_t',
                      '  if value != nil { if len(value) == 0 { value = make([]byte,1); value = value[:0] }; data = (*C.uint8_t)(unsafe.Pointer(unsafe.SliceData(value))) }',
                      '  if code := C.sn_abi_v1_native_byte_array_copy_bytes(data,C.uint64_t(len(value)),&output); code != 0 { '+(normal_return('code') if status else 'panic("byte array result ABI")')+' }']
            value = 'output'
        elif result == 'bool':
            lines += ['  var output C.uint8_t', '  if value { output = 1 }']
            value = 'output'
        else: value = f'{wire(result)}(value)'
        if status:
            if result != 'void': lines.append(f'  *out = {value}')
            lines.append('  '+normal_return('0'))
        elif result != 'void': lines.append(f'  return {value}')
        lines.append('}')
    if main: lines.append('func main() {}')
    return '\n'.join(lines)+'\n'
