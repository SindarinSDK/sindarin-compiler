"""Compile Sindarin implementation units into standalone C/Rust ABI libraries."""
import json
import os
from pathlib import Path
import re
import shlex

from native_contract import TYPES, kind
from native_provider import RUST, c_header, c_provider, rust_provider


def build_library(args, root, work, stem, unit, plan, signatures, tools, run):
    runtime = plan['package']['runtime']
    if runtime == 'GO':
        raise ValueError('Sindarin package bodies select GO; the Sindarin Go backend is not implemented')
    if runtime not in ('C','RS'):
        raise ValueError('Sindarin implementation runtime must resolve to C or RS')
    if not signatures:
        raise ValueError('Sindarin implementation exports require resolved generated function bindings')
    target = 'c' if runtime=='C' else 'rust'
    entry = (root/unit['entry']).resolve()
    model_path = work/(stem+'.model.json')
    common = [args.compiler,entry,'--package-body','--no-install','--target',target,
              '-O'+args.optimization,'--'+args.arithmetic]
    run(common+['--emit-model','-o',model_path],cwd=root)
    # Compiler models/C source can contain arbitrary language string bytes.
    # Decode structural text while round-tripping byte payloads unchanged.
    model = json.loads(model_path.read_text(encoding="utf-8", errors="surrogateescape"))
    if model.get('top_level_statements'):
        raise ValueError('Sindarin library top-level statements require explicit package initializer contracts')
    lifecycle = bool(model.get('globals'))
    if lifecycle and model.get('threads'):
        raise ValueError('Package-global thread lifetimes require managed lifecycle adapters')
    def sources(node):
        if isinstance(node,dict):
            path=node.get('source_file')
            if isinstance(path,str) and Path(path).is_file():dependencies.add(Path(path).resolve())
            for child in node.values():sources(child)
        elif isinstance(node,list):
            for child in node:sources(child)
    dependencies = set();sources(model)
    public_files={(root/path).resolve() for path in plan['native']['declarations']}
    if any(f.get('is_native') and Path(f.get('source_file','')).resolve() in public_files for f in model['functions']):
        raise ValueError('Sindarin implementation cannot import its consumer API; implementation dependencies need separate contracts')
    functions = {f['name']:f for f in model['functions']}
    declarations = {f['name']:f for f in model.get('package_implementation_declarations',[])}
    selected = []
    for signature in signatures:
        name = signature['binding']['function']
        function = functions.get(name)
        if not function or function.get('is_native') or not function.get('has_body'):
            raise ValueError('Sindarin package export requires a defined implementation function: '+name)
        declared = declarations.get(name,{})
        if declared.get('return_mem_qual','default')!='default' or declared.get('type_param_count',0):
            raise ValueError('Sindarin exported return qualifiers/generic bodies require wider package contracts: '+name)
        parameters = function['params']
        if (len(parameters)!=len(signature['params']) or
            kind(function['return_type'])!=kind(signature['return_type']) or
            any(kind(p['type'])!=kind(q['type']) or p.get('mem_qual','default')!='default' or
                p.get('sync_mod','none')!='none' for p,q in zip(parameters,signature['params']))):
            raise ValueError('Sindarin implementation type/ownership differs from its public declaration: '+name)
        if any(kind(p['type'])=='string_array' for p in parameters):
            if signature.get('abi') != '1.5':
                raise ValueError('Sindarin library array inputs require generated mutable/borrowed-body contracts in native ABI 1.5')
            if runtime != 'C':
                raise ValueError('Rust Sindarin library mutable array inputs require canonical native-array body emission')
        selected.append(function)
    emitted = work/(stem+('.c' if runtime=='C' else '.rs'))
    run(common+['--emit-source','-o',emitted],cwd=root)
    archive = work/('lib'+stem+'.a')
    adjusted = []
    for signature in signatures:
        name = signature['binding']['function']
        adjusted.append(dict(signature,binding=dict(signature['binding'],function='__sn_body_'+name)))
    if runtime=='C':
        # Generated functions and methods are internal to this package unit.
        names = {'__sn__'+f['name'] for f in model['functions'] if not f.get('is_native')}
        names.update('__sn__'+g['name'] for g in model.get('globals',[]))
        names.update('__sn__'+g['name']+'_mutex' for g in model.get('globals',[]) if g.get('sync_mod')=='atomic')
        for structure in model.get('structs',[]):
            names.update('__sn__'+structure['name']+'_'+m['name'] for m in structure.get('methods',[]) if not m.get('is_native'))
        prefix = '\n'.join('#define '+name+' __sn_'+stem+'_body_'+name for name in sorted(names))+'\n'
        code = '#include "sn_abi.h"\n'+prefix+emitted.read_text(encoding="utf-8", errors="surrogateescape")+'\n'
        code += c_header(adjusted,stem)+'\n'
        for signature,function in zip(adjusted,selected):
            result = kind(signature['return_type']);status=signature['binding']['failure']=='status'
            parameters=[TYPES[kind(p['type'])][0]+f' p{i}' for i,p in enumerate(signature['params'])]
            if status and result!='void':parameters.append(TYPES[result][0]+' *out')
            code += ('uint32_t' if status else TYPES[result][0])+' '+signature['binding']['function']+'('+(', '.join(parameters) or 'void')+') {\n'
            if lifecycle:
                code += '  SnAbiPackageCall *call = NULL; uint32_t initialized = __sn_package_begin(&call);\n'
                code += '  if (initialized) { '+('return initialized;' if status else 'abort();')+' }\n'
            call='__sn__'+function['name']+'('+', '.join('p'+str(i) for i in range(len(signature['params'])))+')'
            if lifecycle and result!='void':
                code+='  '+TYPES[result][0]+' result = '+call+';\n'
            else:code += ('  *out = ' if status and result!='void' else '  return ' if not status and result!='void' else '  ')+call+';\n'
            if lifecycle:
                code+='  sn_abi_v1_package_end(call);\n'
                if result!='void':code+=('  *out = result;\n' if status else '  return result;\n')
            if status:code+='  return SN_ABI_OK;\n'
            code+='}\n'
        code += c_provider(adjusted,stem)
        if lifecycle:
            code += f'uint32_t __sn_{stem}_initialize(void) {{ SnAbiPackageCall *call = NULL; uint32_t code = __sn_package_begin(&call); if (!code) sn_abi_v1_package_end(call); return code; }}\n'
            code += f'uint32_t __sn_{stem}_shutdown(void) {{ if (pthread_once(&__sn_package_once, __sn_package_create)) return SN_ABI_FOREIGN_ERROR; return sn_abi_v1_package_shutdown(__sn_package_control); }}\n'
        emitted.write_text(code, encoding="utf-8", errors="surrogateescape")
        obj=work/(stem+'.o');dep=work/(stem+'.d')
        flags=['-std=c11','-D_GNU_SOURCE','-Werror=implicit-function-declaration','-O'+args.optimization,'-fwrapv','-fno-lto']
        includes=[args.compiler.resolve().parent/'include/runtime']+[(root/p).resolve() for p in unit.get('include_dirs',[])]
        for directory in includes:flags+=['-I',directory]
        run(tools['cc']+flags+shlex.split(os.environ.get('SN_CFLAGS',''))+['-MD','-MF',dep,'-c',emitted,'-o',obj],cwd=root)
        run(tools['ar']+['rcs',archive,obj])
        # The parent builder captures the compiler dependency file before publication.
        links=[]
        if lifecycle:links.append('-pthread')
        for pragma in model.get('pragmas',[]):
            if pragma.get('pragma_type')!='link':continue
            tokens=shlex.split(pragma['value'])
            if len(tokens)==1 and not tokens[0].startswith('-'):
                token=tokens[0]
                links.append(str((root/token).resolve()) if token.endswith(('.a','.so','.dylib','.lib')) else '-l'+token)
            else:links.extend(tokens)
        return archive,links,[dep],model,dependencies
    code=emitted.read_text(encoding="utf-8", errors="surrogateescape")+'\n'
    for signature,function in zip(adjusted,selected):
        parameters=[];arguments=[]
        for index,p in enumerate(signature['params']):
            k=kind(p['type']);parameters.append(f'p{index}: '+('Option<&[u8]>' if k=='string' else RUST[k]))
            arguments.append(f'p{index}.map_or_else(SnString::nil, SnString::from_c_bytes)' if k=='string' else f'p{index} as char' if k=='char' else f'p{index}')
        result=kind(signature['return_type']);status=signature['binding']['failure']=='status'
        output=RUST[result]
        if status:output='Result<'+output+',u32>'
        # Raw identifiers preserve source names that are Rust keywords.
        name='r#'+function['name']
        code+='pub fn '+signature['binding']['function']+'('+', '.join(parameters)+') -> '+output+' {\n'
        if lifecycle:
            code+='  let _call = __sn_package_enter()'+('?' if status else '.unwrap_or_else(|_|std::process::abort())')+';\n'
        code+='  let value = '+name+'('+', '.join(arguments)+');\n'
        value=('if value.is_nil() { None } else { Some(value.as_bytes().to_vec()) }' if result=='string' else
               'if value.is_nil() { None } else { Some(value.iter().map(|text| if text.is_nil() { None } else { Some(text.as_bytes().to_vec()) }).collect()) }' if result=='string_array' else
               'value as u32 as u8' if result=='char' else 'value')
        code+='  '+('Ok('+value+')' if status else value)+'\n}\n'
    emitted.write_text(code, encoding="utf-8", errors="surrogateescape")
    backing=work/('lib'+stem+'_body.rlib');crate=stem+'_body'
    flags=shlex.split(os.environ.get('SN_RUSTFLAGS',''))
    compile_args=tools['rustc']+['--edition=2021','--crate-type=rlib','--crate-name',crate,'--emit=dep-info,link','-C','opt-level='+args.optimization,emitted,'-o',backing]+flags
    run(compile_args,cwd=root)
    provider=work/(stem+'_exports.rs');exports=rust_provider(adjusted,crate)
    if lifecycle:
        exports+=f'#[no_mangle] pub extern "C" fn __sn_{stem}_initialize()->u32 {{ backing::__sn_body_initialize() }}\n'
        exports+=f'#[no_mangle] pub extern "C" fn __sn_{stem}_shutdown()->u32 {{ backing::__sn_body_shutdown() }}\n'
    provider.write_text(exports)
    compile_args=tools['rustc']+['--edition=2021','--crate-type=staticlib','--crate-name',stem,'--emit=dep-info,link','-C','opt-level='+args.optimization,provider,'--extern',crate+'='+str(backing),'-o',archive]+flags
    diagnostics=run(compile_args+['--print=native-static-libs'],cwd=root,include_stderr=True)
    run(compile_args,cwd=root)
    libraries=re.search(r'native-static-libs:\s*(.*)',diagnostics)
    if not libraries:raise ValueError('Rust library did not report native link dependencies')
    native_links=shlex.split(libraries.group(1))
    if lifecycle:native_links.append('-pthread')
    return archive,native_links,[backing.with_suffix('.d'),archive.with_suffix('.d')],model,dependencies
