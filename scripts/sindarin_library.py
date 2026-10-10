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
    model = json.loads(model_path.read_text())
    if model.get('globals') or model.get('top_level_statements'):
        raise ValueError('Sindarin library global/storage initialization requires the package lifecycle pipeline')
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
        if any(kind(p['type'])=='string_array' for p in parameters) or kind(function['return_type'])=='string_array':
            raise ValueError('Sindarin library array exports require generated managed-body adapters')
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
        for structure in model.get('structs',[]):
            names.update('__sn__'+structure['name']+'_'+m['name'] for m in structure.get('methods',[]) if not m.get('is_native'))
        prefix = '\n'.join('#define '+name+' __sn_'+stem+'_body_'+name for name in sorted(names))+'\n'
        code = '#include "sn_abi.h"\n'+prefix+emitted.read_text()+'\n'
        code += c_header(adjusted,stem)+'\n'
        for signature,function in zip(adjusted,selected):
            result = kind(signature['return_type']);status=signature['binding']['failure']=='status'
            parameters=[TYPES[kind(p['type'])][0]+f' p{i}' for i,p in enumerate(signature['params'])]
            if status and result!='void':parameters.append(TYPES[result][0]+' *out')
            code += ('uint32_t' if status else TYPES[result][0])+' '+signature['binding']['function']+'('+(', '.join(parameters) or 'void')+') {\n'
            call='__sn__'+function['name']+'('+', '.join('p'+str(i) for i in range(len(signature['params'])))+')'
            code += ('  *out = ' if status and result!='void' else '  return ' if not status and result!='void' else '  ')+call+';\n'
            if status:code+='  return SN_ABI_OK;\n'
            code+='}\n'
        code += c_provider(adjusted,stem)
        emitted.write_text(code)
        obj=work/(stem+'.o');dep=work/(stem+'.d')
        flags=['-std=c11','-D_GNU_SOURCE','-Werror=implicit-function-declaration','-O'+args.optimization,'-fwrapv','-fno-lto']
        includes=[args.compiler.resolve().parent/'include/runtime']+[(root/p).resolve() for p in unit.get('include_dirs',[])]
        for directory in includes:flags+=['-I',directory]
        run(tools['cc']+flags+shlex.split(os.environ.get('SN_CFLAGS',''))+['-MD','-MF',dep,'-c',emitted,'-o',obj],cwd=root)
        run(tools['ar']+['rcs',archive,obj])
        # The parent builder captures the compiler dependency file before publication.
        links=[]
        for pragma in model.get('pragmas',[]):
            if pragma.get('pragma_type')!='link':continue
            tokens=shlex.split(pragma['value'])
            if len(tokens)==1 and not tokens[0].startswith('-'):
                token=tokens[0]
                links.append(str((root/token).resolve()) if token.endswith(('.a','.so','.dylib','.lib')) else '-l'+token)
            else:links.extend(tokens)
        return archive,links,[dep],model,dependencies
    code=emitted.read_text()+'\n'
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
        code+='  let value = '+name+'('+', '.join(arguments)+');\n'
        value='if value.is_nil() { None } else { Some(value.as_bytes().to_vec()) }' if result=='string' else 'value as u32 as u8' if result=='char' else 'value'
        code+='  '+('Ok('+value+')' if status else value)+'\n}\n'
    emitted.write_text(code)
    backing=work/('lib'+stem+'_body.rlib');crate=stem+'_body'
    flags=shlex.split(os.environ.get('SN_RUSTFLAGS',''))
    compile_args=tools['rustc']+['--edition=2021','--crate-type=rlib','--crate-name',crate,'--emit=dep-info,link','-C','opt-level='+args.optimization,emitted,'-o',backing]+flags
    run(compile_args,cwd=root)
    provider=work/(stem+'_exports.rs');provider.write_text(rust_provider(adjusted,crate))
    compile_args=tools['rustc']+['--edition=2021','--crate-type=staticlib','--crate-name',stem,'--emit=dep-info,link','-C','opt-level='+args.optimization,provider,'--extern',crate+'='+str(backing),'-o',archive]+flags
    diagnostics=run(compile_args+['--print=native-static-libs'],cwd=root,include_stderr=True)
    run(compile_args,cwd=root)
    libraries=re.search(r'native-static-libs:\s*(.*)',diagnostics)
    if not libraries:raise ValueError('Rust library did not report native link dependencies')
    return archive,shlex.split(libraries.group(1)),[backing.with_suffix('.d'),archive.with_suffix('.d')],model,dependencies
