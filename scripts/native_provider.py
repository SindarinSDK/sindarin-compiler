"""Generate provider exports for declared ordinary backing-language functions."""
import json
import re
from native_contract import TYPES, kind, validate

RUST = {'int':'i64', 'long':'i64', 'uint':'u64', 'int32':'i32', 'uint32':'u32',
        'byte':'u8', 'char':'u8', 'bool':'bool', 'float':'f32', 'double':'f64',
        'string':'Option<Vec<u8>>', 'void':'()'}
GO = {'int':'int64', 'long':'int64', 'uint':'uint64', 'int32':'int32', 'uint32':'uint32',
      'byte':'uint8', 'char':'uint8', 'bool':'bool', 'float':'float32', 'double':'float64',
      'string':'*string', 'void':''}


def contracts(native, unit):
    signatures = [s for s in native.get('signatures', []) if s['binding']['build'] == unit['name']]
    expected = [b for b in native['bindings'] if b['build'] == unit['name'] and 'function' in b]
    if len(signatures) != len(expected):
        raise ValueError('generated provider requires compiler-resolved native declarations')
    symbols = set()
    for s in signatures:
        validate(s)
        binding = s['binding']
        for field in ('function', 'symbol'):
            if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', binding[field]):
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
    raw = [TYPES[kind(p['type'])][0] for p in params]
    if status and result != 'void': raw.append(TYPES[result][0] + ' *')
    return f'extern {"uint32_t" if status else TYPES[result][0]} {b["function"]}({", ".join(raw) or "void"});'


def c_header(signatures, namespace):
    # Preserve source while giving declared backing functions package-private
    # link identities. Otherwise a second archive's ordinary echo() can silently
    # resolve to the first package's echo(), even when wire exports are distinct.
    functions = sorted({s['binding']['function'] for s in signatures})
    names = [f'#define {function} __sn_{namespace}_impl_{function}' for function in functions]
    return '#include <stdint.h>\n#include <stdbool.h>\n'+'\n'.join(names+[c_declaration(s) for s in signatures])+'\n'


def c_provider(signatures):
    lines = ['#include <stdint.h>', '#include <stdbool.h>', '#include <stdlib.h>', '#include "sn_abi.h"']
    for s in signatures:
        b, params, result = s['binding'], s['params'], kind(s['return_type'])
        status = b['failure'] == 'status'
        lines.append(c_declaration(s))
        wire = [f'{TYPES[kind(p["type"])][1]} p{i}' for i,p in enumerate(params)]
        if status and result != 'void': wire.append(TYPES[result][1] + ' *out')
        lines.append(f'{"uint32_t" if status else TYPES[result][1]} {b["symbol"]}({", ".join(wire) or "void"}) {{')
        if status and result != 'void': lines.append('  if (!out) return SN_ABI_INVALID_ARGUMENT;')
        arguments = []
        for i,p in enumerate(params):
            k = kind(p['type'])
            if k == 'string':
                lines += [f'  SnAbiBytes bytes{i};', f'  uint32_t status{i} = sn_abi_v1_string_bytes(p{i}, &bytes{i});',
                          f'  if (status{i}) {{ ' + (f'return status{i};' if status else 'abort();') + ' }']
                arguments.append(f'(char *)bytes{i}.data')
            else:
                if k == 'bool': lines.append(f'  if (p{i} > 1) {{ '+('return SN_ABI_INVALID_ARGUMENT;' if status else 'abort();')+' }')
                arguments.append(f'({TYPES[k][0]})p{i}')
        if result != 'void': lines.append(f'  {TYPES[result][0]} value = {{0}};')
        if status:
            if result != 'void': arguments.append('&value')
            lines += [f'  uint32_t status = {b["function"]}({", ".join(arguments)});', '  if (status) return status;']
        else: lines.append(('  value = ' if result != 'void' else '  ')+f'{b["function"]}({", ".join(arguments)});')
        if result == 'string':
            lines += ['  SnAbiValue *wire_value = NULL;', '  uint32_t status_copy = sn_abi_v1_string_copy(value, &wire_value);',
                      '  free(value);', '  if (status_copy) { '+('return status_copy;' if status else 'abort();')+' }']
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
             'fn sn_abi_v1_string_copy(v: *const std::ffi::c_char, out: *mut *mut V) -> u32; }']
    for s in signatures:
        b, params, result = s['binding'], s['params'], kind(s['return_type'])
        status = b['failure'] == 'status'
        wire = lambda k: '*mut V' if k == 'string' else 'u8' if k == 'bool' else RUST[k]
        declarations = [f'p{i}: {wire(kind(p["type"]))}' for i,p in enumerate(params)]
        if status and result != 'void': declarations.append(f'out: *mut {wire(result)}')
        lines += [f'#[no_mangle] pub unsafe extern "C" fn {b["symbol"]}({", ".join(declarations)}) -> '+('u32' if status else wire(result))+' {']
        if status and result != 'void': lines.append('  if out.is_null() { return 1; }')
        lines.append(f'  let call = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> Result<{wire(result)}, u32> {{')
        arguments = []
        for i,p in enumerate(params):
            k = kind(p['type'])
            if k == 'string':
                lines += [f'    let mut bytes{i} = Bytes {{ data: std::ptr::null(), length: 0 }};',
                          f'    let status = sn_abi_v1_string_bytes(p{i}, &mut bytes{i}); if status != 0 {{ return Err(status); }}',
                          f'    let length{i} = usize::try_from(bytes{i}.length).map_err(|_| 5u32)?;',
                          f'    if length{i} > isize::MAX as usize {{ return Err(5); }}',
                          f'    let arg{i} = if bytes{i}.data.is_null() {{ None }} else {{ Some(std::slice::from_raw_parts(bytes{i}.data, length{i})) }};']
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
    if any(kind(p['type']) == 'string' for s in signatures for p in s['params']) or any(kind(s['return_type']) == 'string' for s in signatures):
        lines.append('import "unsafe"')
    wire = lambda k: '*C.SnAbiValue' if k == 'string' else 'C.'+TYPES[k][1] if k != 'void' else ''
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
                lines += [f'  var bytes{i} C.SnAbiBytes', f'  if code := C.sn_abi_v1_string_bytes(p{i}, &bytes{i}); code != 0 {{ '+(normal_return('code') if status else 'panic("invalid string ABI")')+' }',
                          f'  var arg{i} *string', f'  if bytes{i}.data != nil {{',
                          f'    if uint64(bytes{i}.length) > uint64(^uint(0)>>1) {{ '+(normal_return('5') if status else 'panic("string length overflow")')+' }',
                          f'    text := string(unsafe.Slice((*byte)(unsafe.Pointer(bytes{i}.data)), int(bytes{i}.length))); arg{i} = &text', '  }']
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
