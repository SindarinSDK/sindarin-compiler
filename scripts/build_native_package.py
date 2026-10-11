#!/usr/bin/env python3
"""Build native backing archives from compiler-validated package metadata."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
import uuid
from native_provider import contracts, c_header, c_provider, rust_provider, go_provider
from sindarin_library import build_library


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def key(data):
    return hashlib.sha256(json.dumps(data, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def run(args, cwd=None, env=None, include_stderr=False, input_bytes=None):
    result = subprocess.run([str(a) for a in args], cwd=cwd, env=env,
                            capture_output=True, timeout=300, input=input_bytes)
    if result.returncode:
        raise ValueError(f'native tool failed ({result.returncode}): {args[0]}\n'
                         + result.stderr.decode(errors='replace'))
    return (result.stdout + (result.stderr if include_stderr else b'')).decode(errors='replace')


def command(env, default):
    return shlex.split(os.environ.get(env) or default)


def tool_identity(args, version_args=None, optional_version=False):
    executable = shutil.which(args[0])
    if not executable:
        raise ValueError(f'native toolchain unavailable: {args[0]}')
    try:
        version = run(args + (version_args or ['--version'])).strip()
    except ValueError:
        if not optional_version: raise
        version = None  # BSD tools may have no version option; binary hash is authoritative.
    return {'command': args, 'executable_sha256': sha(executable), 'version': version}


def host_target(languages, cc, rustc, go):
    expected_arch = {'amd64': 'x86_64', 'x86_64': 'x86_64', 'arm64': 'aarch64',
                     'aarch64': 'aarch64'}.get(platform.machine().lower())
    expected_os = {'Linux': 'linux', 'Darwin': 'macos', 'Windows': 'windows'}[platform.system()]
    bits = struct.calcsize('P') * 8
    if languages & {'C', 'GO'}:
        macros = run(cc + shlex.split(os.environ.get('SN_CFLAGS', '')) +
                     ['-dM', '-E', '-x', 'c', '-'], input_bytes=b'')
        arch = 'x86_64' if '#define __x86_64__ ' in macros else 'aarch64' if '#define __aarch64__ ' in macros else None
        system = 'windows' if '#define _WIN32 ' in macros else 'macos' if '#define __APPLE__ ' in macros else 'linux' if '#define __linux__ ' in macros else None
        width = re.search(r'^#define __SIZEOF_POINTER__ (\d+)$', macros, re.M)
        if (arch, system, int(width.group(1)) * 8 if width else None) != (expected_arch, expected_os, bits):
            raise ValueError('C native target differs from this compiler/runtime host; cross-platform artifact builds are not implemented')
    if 'RS' in languages:
        cfg = run(rustc + ['--print', 'cfg'] + shlex.split(os.environ.get('SN_RUSTFLAGS', '')))
        values = dict(re.findall(r'^(target_arch|target_os|target_pointer_width)="([^"]+)"$', cfg, re.M))
        if (values.get('target_arch'), values.get('target_os'), values.get('target_pointer_width')) != (expected_arch, expected_os, str(bits)):
            raise ValueError('Rust native target differs from this compiler/runtime host; cross-platform artifact builds are not implemented')
    if 'GO' in languages:
        values = run(go + ['env', 'GOOS', 'GOARCH']).splitlines()
        go_arch = {'x86_64': 'amd64', 'aarch64': 'arm64'}.get(expected_arch)
        go_os = 'darwin' if expected_os == 'macos' else expected_os
        if values != [go_os, go_arch]:
            raise ValueError('Go native target differs from this compiler/runtime host; cross-platform artifact builds are not implemented')


def source_path(root, value):
    path = (root / value).resolve()
    if not path.is_file():
        raise ValueError(f'native input is missing: {path}')
    return path


def tree_inputs(root, output):
    files = {}
    for folder, directories, names in os.walk(root):
        directory = Path(folder)
        directories[:] = [d for d in directories if d not in ('.git', '.sn') and
                          not (directory / d).resolve().is_relative_to(output)]
        for name in names:
            path = directory / name
            if path.is_file() and not path.resolve().is_relative_to(output):
                files[str(path.resolve())] = sha(path)
    return files


def dependencies(path):
    """Read the first Make dep-info rule, including escaped spaces/drive paths."""
    text = path.read_text().replace('\\\n', '')
    line = next((s for s in text.splitlines() if re.search(r'(?<!\\):\s', s)), '')
    match = re.search(r'(?<!\\):\s', line)
    if not match:
        raise ValueError(f'native dependency information is missing: {path}')
    words, word = [], ''
    i = match.end()
    while i < len(line):
        char = line[i]
        if char == '\\' and i + 1 < len(line) and (line[i + 1].isspace() or line[i + 1] in '\\#$:'):
            i += 1
            word += line[i]
        elif char == '$' and i + 1 < len(line) and line[i + 1] == '$':
            word += '$'
            i += 1
        elif char.isspace():
            if word: words.append(word); word = ''
        else:
            word += char
        i += 1
    if word: words.append(word)
    return words


def go_dependencies(go, root, unit, env):
    text = run(go + ['list', '-mod=readonly', '-deps', '-json', '.'],
               cwd=(root / unit['module']).resolve(), env=env)
    decoder, position, files, flags = json.JSONDecoder(), 0, {}, []
    while position < len(text):
        while position < len(text) and text[position].isspace(): position += 1
        if position == len(text): break
        package, position = decoder.raw_decode(text, position)
        if not package.get('Dir'): continue
        directory = Path(package['Dir'])
        for field in ('GoFiles', 'CgoFiles', 'CFiles', 'HFiles', 'CXXFiles', 'FFiles', 'SFiles', 'SysoFiles'):
            for name in package.get(field, []):
                path = (directory / name).resolve()
                if path.is_file(): files[str(path)] = sha(path)
        module = package.get('Module') or {}
        for name in ('GoMod',):
            if module.get(name) and Path(module[name]).is_file():
                path = Path(module[name]).resolve(); files[str(path)] = sha(path)
        for flag in package.get('CgoLDFLAGS', []):
            flag = flag.replace('${SRCDIR}', str(directory))
            if flag.startswith('-L') and len(flag) > 2 and not Path(flag[2:]).is_absolute():
                flag = '-L' + str((directory / flag[2:]).resolve())
            flags.append(flag)
    return files, flags


def cached(entry, fingerprint):
    pointer = entry / 'current.json'
    if not pointer.is_file():
        return None
    try:
        generation = json.loads(pointer.read_text())['generation']
        if not re.fullmatch(r'[0-9a-f]{64}', generation): return None
        metadata = entry / generation / 'assembly.json'
        result = json.loads(metadata.read_text())
        if key(result) != generation: return None
        if result['fingerprint'] != fingerprint or not result['cache_reusable']:
            return None
        for name, expected in result['dependency_sha256'].items():
            if sha(name) != expected: return None
        for unit in result['units']:
            if sha(metadata.parent / unit['archive']) != unit['archive_sha256']: return None
        return metadata
    except (OSError, ValueError, KeyError, TypeError):
        return None


def load_prebuilt(plan, manifest, compiler):
    """Validate a sealed package artifact without opening backing sources/tools."""
    try:
        return validate_prebuilt(plan,manifest,compiler)
    except (KeyError,TypeError,AttributeError) as error:
        raise ValueError('malformed prebuilt native assembly descriptor: '+str(error)) from error


def validate_prebuilt(plan, manifest, compiler):
    root = Path(manifest).resolve().parent
    native = plan['native']
    reference = native['assembly']
    descriptor = (root / reference['path']).resolve()
    if not descriptor.is_file() or sha(descriptor) != reference['sha256']:
        raise ValueError('prebuilt native assembly descriptor is missing or its sha256 differs')
    metadata = json.loads(descriptor.read_text())
    if metadata.get('schema') != 1 or metadata.get('kind') != 'native-backing-artifacts':
        raise ValueError('unsupported prebuilt native assembly schema/kind')
    if metadata.get('package') != plan['package']:
        raise ValueError('prebuilt native assembly package identity/version/runtime differs')
    if metadata.get('abi') != native['abi'] or native['abi'] not in ('1.0','1.1','1.5'):
        raise ValueError('prebuilt native assembly ABI differs from the declared supported ABI')
    host = {'system':platform.system(), 'machine':platform.machine(),
            'pointer_bits':struct.calcsize('P') * 8}
    if metadata.get('compatibility') != host:
        raise ValueError('prebuilt native assembly platform/architecture/pointer width differs')
    if metadata.get('bindings') != native['bindings']:
        raise ValueError('prebuilt native assembly binding/ownership contract differs')
    if metadata.get('types',[]) != native.get('types',[]):
        raise ValueError('prebuilt native assembly record storage/lifecycle contract differs')
    if metadata.get('provider_signatures',[]) != native.get('signatures',[]):
        raise ValueError('prebuilt native assembly resolved provider type contract differs')
    for unit in native['builds']:
        contracts(native,unit)
    declarations = metadata.get('declarations',[])
    if [item.get('path') for item in declarations] != native['declarations']:
        raise ValueError('prebuilt native assembly public declaration inventory differs')
    for item in declarations:
        source = source_path(root,item['path'])
        recorded = base64.b64decode(item['source_base64'],validate=True)
        if hashlib.sha256(recorded).hexdigest() != item['sha256'] or source.read_bytes() != recorded:
            raise ValueError('prebuilt native assembly public declaration bytes differ: '+item['path'])
    public_headers = metadata.get('record_headers',[])
    paths = [item['path'] for item in public_headers]
    if len(paths) != len(set(paths)) or not {r['header'] for r in native.get('types',[])} <= set(paths):
        raise ValueError('prebuilt native assembly public record header inventory differs')
    for item in public_headers:
        source = source_path(root,item['path'])
        if not source.is_relative_to(root):
            raise ValueError('prebuilt record header is outside the package')
        recorded = base64.b64decode(item['source_base64'],validate=True)
        if hashlib.sha256(recorded).hexdigest() != item['sha256'] or source.read_bytes() != recorded:
            raise ValueError('prebuilt native assembly public record header bytes differ: '+item['path'])
    units = metadata.get('units',[])
    builds = native['builds']
    if [unit.get('name') for unit in units] != [unit['name'] for unit in builds]:
        raise ValueError('prebuilt native assembly build-unit inventory differs')
    archives = set()
    for unit,build_unit in zip(units,builds):
        symbols = [b['symbol'] for b in native['bindings'] if b['build']==unit['name']]
        generated = [b['symbol'] for b in native['bindings'] if b['build']==unit['name'] and 'function' in b]
        initialization = 'Go toolchain runtime' if build_unit['language']=='GO' else 'native toolchain'
        if (unit.get('language') != build_unit['language'] or unit.get('symbols') != symbols or
                unit.get('generated_provider_exports') != generated or
                unit.get('libraries') != build_unit.get('libraries',[]) or
                unit.get('initialization') != initialization or
                unit.get('requires_go_aggregation') != (build_unit['language']=='GO')):
            raise ValueError('prebuilt native assembly build/export/initialization contract differs: '+unit['name'])
        if build_unit['language']=='SN' and unit.get('implementation_runtime')!=plan['package']['runtime']:
            raise ValueError('prebuilt Sindarin implementation runtime differs')
        if 'package_lifecycle' in unit:
            stem='sn_native_'+f'{native["builds"].index(build_unit):03d}_'+key({'package':plan['package'],'build':build_unit['name']})[:12]
            expected={'abi':'1.2','initialize':'__sn_'+stem+'_initialize','shutdown':'__sn_'+stem+'_shutdown'}
            if build_unit['language']!='SN' or unit['package_lifecycle']!=expected:
                raise ValueError('prebuilt package lifecycle contract differs')
        if not isinstance(unit.get('native_link_flags'),list) or not all(isinstance(f,str) for f in unit['native_link_flags']):
            raise ValueError('prebuilt native assembly linker options must be strings')
        archive = (descriptor.parent / unit['archive']).resolve()
        if (not archive.is_relative_to(descriptor.parent) or not archive.is_file() or
                sha(archive) != unit['archive_sha256']):
            raise ValueError('prebuilt native assembly archive is missing, outside the generation or corrupt: '+unit['name'])
        if unit['language']=='GO': archives.add(archive)
    if len(archives)>1:
        raise ValueError('prebuilt native assembly contains multiple Go runtime archives; one aggregate bridge is required')
    runtime = Path(compiler).resolve().parent/'lib'/('clang' if os.name=='nt' else 'gcc')/'libsn_runtime_min.a'
    if not runtime.is_file(): raise ValueError('consumer shared C runtime archive is missing')
    # Transport ABI 1.0 remains supported by 1.1. Producer locations/hashes are
    # provenance; the application links the consumer's canonical shared runtime.
    result = dict(metadata,shared_runtime={'archive':str(runtime),'sha256':sha(runtime)})
    return result,descriptor


def build(args):
    root = args.manifest.resolve().parent
    output = args.out_dir.resolve()
    compiler = args.compiler.resolve()
    plan = getattr(args, 'validated_plan', None) or json.loads(run([compiler, '--native-plan', args.manifest.resolve(), '--target', args.target,
                           '-O' + args.optimization, '--' + args.arithmetic]))
    native = plan['native']
    if 'assembly' in native:
        metadata, descriptor = load_prebuilt(plan, args.manifest, compiler)
        print(json.dumps({'assembly':str(descriptor), 'cache_hit':True, 'prebuilt':True,
                          'shared_runtime':metadata['shared_runtime']}))
        return
    providers = {u['name']: contracts(native, u) for u in native['builds']}
    generated_exports = set()
    for signatures in providers.values():
        for signature in signatures:
            symbol = signature['binding']['symbol']
            if symbol in generated_exports:
                raise ValueError('duplicate generated provider export symbol across build units: ' + symbol)
            generated_exports.add(symbol)
    skip_go = getattr(args, 'skip_go', False)
    selected_builds = [u for u in native['builds'] if not (skip_go and u['language'] == 'GO')]
    go_builds = [u for u in selected_builds if u['language'] == 'GO']
    compiler_dir = Path(plan['compiler_dir']).resolve()
    runtime = compiler_dir / 'lib' / ('clang' if os.name == 'nt' else 'gcc') / 'libsn_runtime_min.a'
    header = compiler_dir / 'include/runtime/sn_abi.h'
    if not runtime.is_file() or not header.is_file():
        raise ValueError('shared C runtime archive/header is missing from this compiler installation')
    cc = command('SN_CC', 'clang' if os.name == 'nt' or sys.platform == 'darwin' else 'gcc')
    rustc = command('SN_RUSTC', 'rustc')
    go = command('SN_GO', 'go')
    ar = command('SN_AR', 'llvm-ar' if shutil.which('llvm-ar') else 'ar')
    nm = command('SN_NM', 'llvm-nm' if shutil.which('llvm-nm') else 'nm')
    tools = {'nm': tool_identity(nm, optional_version=True)}
    languages = {u['language'] for u in selected_builds}
    if 'SN' in languages:
        if plan['package']['runtime']=='GO':
            raise ValueError('Sindarin package bodies select GO; the Sindarin Go backend is not implemented')
        languages.remove('SN');languages.add(plan['package']['runtime'])
    if languages & {'C', 'GO'}: tools['cc'] = tool_identity(cc)
    if 'C' in languages: tools['ar'] = tool_identity(ar, optional_version=True)
    if 'RS' in languages: tools['rustc'] = tool_identity(rustc)
    if 'GO' in languages: tools['go'] = tool_identity(go, ['version'])
    host_target(languages, cc, rustc, go)
    for unit in native['builds']:
        for path in unit['sources']: source_path(root, path)
        if unit['language'] in ('RS','SN'): source_path(root, unit['entry'])
        for path in unit.get('include_dirs', []):
            if not (root / path).is_dir(): raise ValueError(f'native include directory is missing: {path}')
        if unit['language'] == 'GO' and not (root / unit['module'] / 'go.mod').is_file():
            raise ValueError(f'Go module go.mod is missing: {unit["module"]}')
    declarations = []
    for name in native['declarations']:
        path = source_path(root, name)
        declarations.append({'path': name, 'sha256': sha(path),
                             'source_base64': base64.b64encode(path.read_bytes()).decode()})
    inputs = tree_inputs(root, output)
    for name in native['declarations']:
        path = source_path(root, name); inputs[str(path)] = sha(path)
    for unit in native['builds']:
        for name in unit['sources']:
            path = source_path(root, name); inputs[str(path)] = sha(path)
    identity = {'plan': plan, 'skip_go': skip_go, 'tools': tools, 'inputs': inputs,
                'driver_sha256': sha(Path(__file__)), 'compiler_sha256': sha(compiler),
                'provider_sha256': sha(Path(__file__).with_name('native_provider.py')),
                'library_driver_sha256': sha(Path(__file__).with_name('sindarin_library.py')),
                'contract_sha256': sha(Path(__file__).with_name('native_contract.py')),
                'environment_sha256': key(dict(os.environ)), 'runtime_sha256': sha(runtime),
                'abi_header_sha256': sha(header), 'system': platform.system(),
                'machine': platform.machine(), 'pointer_bits': struct.calcsize('P') * 8}
    fingerprint = key(identity)
    output.mkdir(parents=True, exist_ok=True)
    entry = output / fingerprint
    previous = cached(entry, fingerprint)
    if previous:
        print(json.dumps({'assembly': str(previous), 'cache_hit': True}))
        return
    aggregate_go = (build_go_graph([{'manifest':str(args.manifest.resolve()), 'plan':plan}], compiler,
                    output/'go-graphs', args.optimization, args.arithmetic, input_exclusion=output)
                    if len(go_builds) > 1 else None)
    with tempfile.TemporaryDirectory(prefix='.native-build-', dir=output) as folder:
        work = Path(folder)
        units, deps = [], dict(inputs)
        defined_symbols = set()
        public_headers = set()
        if native.get('types'):
            probe, depfile = work/'record_public.c', work/'record_public.d'
            lines = ['#include <stdint.h>']
            for record in native['types']:
                public = source_path(root,record['header'])
                if not public.is_relative_to(root): raise ValueError('public record header must belong to the package')
                lines.append('#include '+json.dumps(str(public).replace('\\','/')))
                c_type = record['c_type']
                expected = {'create':c_type+' *(*)(void)', 'retain':c_type+' *(*)( '+c_type+' *)',
                            'release':'void (*)( '+c_type+' *)', 'refs':'int (*)( '+c_type+' *)'}
                for role,signature in expected.items():
                    lines.append(f'_Static_assert(_Generic(&{record[role]}, {signature}: 1, default: 0), "record {role} prototype differs");')
            probe.write_text('\n'.join(lines)+'\n')
            flags=['-std=c11','-D_GNU_SOURCE','-I',str(header.parent)]+shlex.split(os.environ.get('SN_CFLAGS',''))
            for unit in native['builds']:
                for directory in unit.get('include_dirs',[]):flags+=['-I',str((root/directory).resolve())]
            run(cc+flags+['-fsyntax-only',probe],cwd=root)
            run(cc+flags+['-M','-MT','record_public','-MF',depfile,probe],cwd=root)
            for name in dependencies(depfile):
                dependency=(root/name).resolve()
                if dependency.is_relative_to(root) and not dependency.is_relative_to(work):
                    public_headers.add(dependency)
                    deps[str(dependency)]=sha(dependency)
        for index, unit in enumerate(selected_builds):
            stem = f'sn_native_{index:03d}_' + key({'package':plan['package'], 'build':unit['name']})[:12]
            archive = work / ('lib' + stem + '.a')
            includes = [str((root / p).resolve()) for p in unit.get('include_dirs', [])]
            includes.append(str(header.parent))
            native_link_flags = []
            signatures = providers[unit['name']]
            def capture_dependencies(depfile):
                for name in dependencies(depfile):
                    dependency = (root / name).resolve()
                    # Generated inputs are covered by plan/helper fingerprints;
                    # their temporary paths cannot enter reusable cache keys.
                    if not dependency.is_relative_to(work):
                        deps[str(dependency)] = sha(dependency)
            if unit['language'] == 'SN':
                archive,native_link_flags,depfiles,library_model,body_sources=build_library(args,root,work,stem,unit,plan,signatures,
                    {'cc':cc,'rustc':rustc,'ar':ar},run)
                for depfile in depfiles:capture_dependencies(depfile)
                for source in body_sources:deps[str(source)]=sha(source)
            elif unit['language'] == 'C':
                objects = []
                sources = [source_path(root, source) for source in unit['sources']]
                contract_flags = []
                if signatures:
                    provider = work / (stem + '_provider.c')
                    provider.write_text(c_provider(signatures, stem, root))
                    sources.append(provider)
                    backing_header = work / (stem + '_backing.h')
                    backing_header.write_text(c_header(signatures, stem, root))
                    contract_flags = ['-include', str(backing_header)]
                for number, source in enumerate(sources):
                    obj, depfile = work / f'{stem}-{number}.o', work / f'{stem}-{number}.d'
                    flags = ['-std=c11', '-D_GNU_SOURCE', '-Werror=implicit-function-declaration', '-O' + args.optimization, '-fno-lto']
                    for directory in includes: flags += ['-I', directory]
                    run(cc + flags + contract_flags + shlex.split(os.environ.get('SN_CFLAGS', '')) +
                        ['-MD', '-MF', depfile, '-c', source, '-o', obj], cwd=root)
                    capture_dependencies(depfile)
                    objects.append(obj)
                run(ar + ['rcs', archive] + objects)
            elif unit['language'] == 'RS':
                flags = shlex.split(os.environ.get('SN_RUSTFLAGS', ''))
                source = source_path(root, unit['entry'])
                extra = []
                if signatures:
                    if any(s['binding']['failure'] == 'status' for s in signatures) and any('panic=abort' in f for f in flags):
                        raise ValueError('status provider exports require Rust panic=unwind')
                    backing_crate = stem + '_backing'
                    backing = work / ('lib' + backing_crate + '.rlib')
                    run(rustc + ['--edition=2021', '--crate-type=rlib', '--crate-name', backing_crate,
                                 '--emit=dep-info,link', '-C', 'opt-level=' + args.optimization,
                                 source, '-o', backing] + flags, cwd=root)
                    capture_dependencies(backing.with_suffix('.d'))
                    source = work / (stem + '_provider.rs')
                    source.write_text(rust_provider(signatures, backing_crate))
                    extra = ['--extern', backing_crate + '=' + str(backing)]
                compile_args = rustc + ['--edition=2021', '--crate-type=staticlib', '--crate-name', stem,
                             '--emit=dep-info,link', '-C', 'opt-level=' + args.optimization,
                             source, '-o', archive] + flags + extra
                diagnostics = run(compile_args + ['--print=native-static-libs'], cwd=root, include_stderr=True)
                run(compile_args, cwd=root)
                libraries = re.search(r'native-static-libs:\s*(.*)', diagnostics)
                if not libraries: raise ValueError('Rust toolchain did not report native static library dependencies')
                native_link_flags = shlex.split(libraries.group(1))
                capture_dependencies(archive.with_suffix('.d'))
                # External crate files supplied through rustc flags affect the archive.
                for flag in flags:
                    candidate = flag.split('=', 1)[-1]
                    if (root / candidate).is_file():
                        path = (root / candidate).resolve(); deps[str(path)] = sha(path)
            elif aggregate_go:
                archive = work / 'libsn_go_graph.a'
                if not archive.exists(): shutil.copyfile(aggregate_go['archive'], archive)
                native_link_flags = aggregate_go['native_link_flags']
                aggregate_metadata = json.loads(Path(aggregate_go['assembly']).read_text())
                deps.update(aggregate_metadata['dependency_sha256'])
            else:
                env = os.environ.copy()
                env['CC'] = shlex.join(cc)
                env['CGO_ENABLED'] = '1'
                env['GOTOOLCHAIN'] = 'local'
                env['CGO_CPPFLAGS'] = shlex.join(['-I' + p for p in includes])
                env['CGO_LDFLAGS'] = shlex.join(['-L' + str(runtime.parent), '-lsn_runtime_min'] +
                                               ['-l' + p for p in unit.get('libraries', [])])
                module = (root / unit['module']).resolve()
                go_unit = unit
                if signatures:
                    env['GOWORK'] = 'off'
                    config = json.loads(run(go + ['mod', 'edit', '-json'], cwd=module, env=env))
                    module_name = config['Module']['Path']
                    major = re.search(r'(?:/v|\.v)([2-9][0-9]*)$', module_name)
                    version = 'v' + (major.group(1) if major else '0') + '.0.0'
                    bridge = work / (stem + '_go_provider')
                    bridge.mkdir()
                    shutil.copyfile(module / 'go.mod', bridge / 'go.mod')
                    if (module / 'go.sum').is_file(): shutil.copyfile(module / 'go.sum', bridge / 'go.sum')
                    edits = ['-module=sindarin.generated/' + stem, '-require=' + module_name + '@' + version,
                             '-replace=' + module_name + '=' + str(module)]
                    for replace in config.get('Replace') or []:
                        new = replace['New']
                        if not new.get('Version') and not Path(new['Path']).is_absolute():
                            old = replace['Old']['Path'] + ('@' + replace['Old']['Version'] if replace['Old'].get('Version') else '')
                            edits.append('-replace=' + old + '=' + str((module / new['Path']).resolve()))
                    run(go + ['mod', 'edit'] + edits, cwd=bridge, env=env)
                    (bridge / 'main.go').write_text(go_provider(signatures, module_name))
                    module = bridge
                    go_unit = dict(unit, module=str(bridge))
                go_inputs, native_link_flags = go_dependencies(go, root, go_unit, env)
                deps.update({p: v for p,v in go_inputs.items() if not Path(p).is_relative_to(work)})
                if os.name != 'nt' and '-pthread' not in native_link_flags:
                    native_link_flags.append('-pthread')
                run(go + ['build', '-mod=readonly', '-buildmode=c-archive', '-o', archive, '.'],
                    cwd=module, env=env)
            symbols = {line.split()[-1] for line in run(nm + ['-g', '-U' if sys.platform == 'darwin' else '--defined-only', archive]).splitlines()
                       if line.split() and not line.rstrip().endswith(':')}
            defined_symbols.update(symbols)
            exports = [b['symbol'] for b in native['bindings'] if b['build'] == unit['name']]
            if unit['language']=='SN' and ('main' in symbols or '_main' in symbols):
                raise ValueError('Sindarin implementation archive contains a competing application main')
            for symbol in exports:
                if symbol not in symbols and '_' + symbol not in symbols:
                    raise ValueError(f'native export missing: {unit["name"]}::{symbol}')
            units.append({'name': unit['name'], 'language': unit['language'], 'archive': archive.name,
                          'archive_sha256': sha(archive), 'symbols': exports,
                          'libraries': unit.get('libraries', []), 'native_link_flags': native_link_flags,
                          'initialization': 'Go toolchain runtime' if unit['language'] == 'GO' else 'native toolchain',
                          'generated_provider_exports': [s['binding']['symbol'] for s in signatures],
                          'requires_go_aggregation': unit['language'] == 'GO'})
            if unit['language']=='SN':
                units[-1]['implementation_runtime']=plan['package']['runtime']
                units[-1]['implementation_exports']=[{'name':s['binding']['function'],
                    'return_type':s['return_type'],'params':s['params']} for s in signatures]
                if library_model.get('globals'):
                    lifecycle={'abi':'1.2','initialize':'__sn_'+stem+'_initialize','shutdown':'__sn_'+stem+'_shutdown'}
                    for name in (lifecycle['initialize'],lifecycle['shutdown']):
                        if name not in symbols and '_'+name not in symbols:raise ValueError('package lifecycle export missing: '+name)
                    units[-1]['package_lifecycle']=lifecycle
        for record in native.get('types',[]):
            for role in ('create','retain','release','refs'):
                symbol=record[role]
                if symbol not in defined_symbols and '_'+symbol not in defined_symbols:
                    raise ValueError(f'native record lifecycle export missing: {record["declaration"]}::{role} ({symbol})')
        current = tree_inputs(root, output)
        for name in native['declarations']:
            path = source_path(root, name); current[str(path)] = sha(path)
        for unit in native['builds']:
            for name in unit['sources']:
                path = source_path(root, name); current[str(path)] = sha(path)
        if (current != inputs or any(sha(path) != expected for path, expected in deps.items()) or
                sha(runtime) != identity['runtime_sha256'] or sha(header) != identity['abi_header_sha256']):
            raise ValueError('package/dependency inputs changed during the native build; no artifact was published')
        metadata = {'schema': 1, 'kind': 'native-backing-artifacts', 'complete_package': False,
                    'requires_generated_adapters': True, 'fingerprint': fingerprint,
                    'package': plan['package'], 'abi': native['abi'], 'declarations': declarations,
                    'bindings': native['bindings'], 'units': units, 'provenance': identity,
                    'provider_signatures': native.get('signatures', []),
                    'types': native.get('types',[]),
                    'record_headers': [{'path':path.relative_to(root).as_posix(),'sha256':sha(path),
                                        'source_base64':base64.b64encode(path.read_bytes()).decode()}
                                       for path in sorted(public_headers)],
                    'compatibility': {k: identity[k] for k in ('system', 'machine', 'pointer_bits')},
                    'shared_runtime': {'sha256': sha(runtime), 'archive': str(runtime)},
                    'dependency_sha256': deps,
                    # Go's own compiler cache remains active. Artifact reuse waits
                    # for complete module/cgo transitive input capture and aggregation.
                    'cache_reusable': not go_builds, 'publication_nonce': uuid.uuid4().hex}
        (work / 'assembly.json').write_text(json.dumps(metadata, indent=2) + '\n')
        entry.mkdir(exist_ok=True)
        generation = key(metadata)
        destination = entry / generation
        # Published generations are immutable. A new build cannot overwrite
        # archives referenced by a consumer already using an older generation.
        os.rename(work, destination)
        fd, pointer = tempfile.mkstemp(prefix='.current-', dir=entry)
        try:
            with os.fdopen(fd, 'w') as stream: json.dump({'generation': generation}, stream)
            os.replace(pointer, entry / 'current.json')
        finally:
            if os.path.exists(pointer): os.unlink(pointer)
    print(json.dumps({'assembly': str(destination / 'assembly.json'), 'cache_hit': False}))


def build_go_graph(packages, compiler, out_dir, optimization, arithmetic, input_exclusion=None):
    """Build one Go runtime bridge from original library modules in a graph."""
    compiler = Path(compiler).resolve()
    output = Path(out_dir).resolve()
    excluded = Path(input_exclusion).resolve() if input_exclusion else output
    cc = command('SN_CC', 'clang' if os.name == 'nt' or sys.platform == 'darwin' else 'gcc')
    go, nm = command('SN_GO', 'go'), command('SN_NM', 'llvm-nm' if shutil.which('llvm-nm') else 'nm')
    tools = {'cc': tool_identity(cc), 'go': tool_identity(go, ['version']),
             'nm': tool_identity(nm, optional_version=True)}
    host_target({'GO'}, cc, [], go)
    runtime = compiler.parent / 'lib' / ('clang' if os.name == 'nt' else 'gcc') / 'libsn_runtime_min.a'
    header = compiler.parent / 'include/runtime/sn_abi.h'
    modules, units, inputs, roots, explicit = {}, [], {}, [], {}
    replacements, includes, libraries, sums, versions = {}, [str(header.parent)], [], set(), []
    env = os.environ.copy()
    env.update(CC=shlex.join(cc), CGO_ENABLED='1', GOTOOLCHAIN='local', GOWORK='off')
    for package in packages:
        root = Path(package['manifest']).resolve().parent
        plan = package['plan']
        roots.append(root)
        inputs.update(tree_inputs(root, excluded))
        for declaration in plan['native']['declarations']:
            path = source_path(root, declaration); explicit[str(path)] = sha(path)
        for unit in plan['native']['builds']:
            if unit['language'] != 'GO': continue
            signatures = contracts(plan['native'], unit)
            bindings = [b for b in plan['native']['bindings'] if b['build'] == unit['name']]
            if any('function' not in b for b in bindings):
                raise ValueError('Go graph aggregation requires generated providers from importable library modules; handwritten main archives cannot be combined')
            for source in unit['sources']:
                path = source_path(root, source); explicit[str(path)] = sha(path)
            directory = (root / unit['module']).resolve()
            config = json.loads(run(go + ['mod', 'edit', '-json'], cwd=directory, env=env))
            for filename in ('go.mod', 'go.sum'):
                path = directory / filename
                if path.is_file(): explicit[str(path)] = sha(path)
            module = config['Module']['Path']
            if module in modules and modules[module] != str(directory):
                raise ValueError('Go module path refers to different source roots in the package graph: ' + module)
            modules[module] = str(directory)
            versions.append(config.get('Go') or '1.16')
            if (directory / 'go.sum').is_file(): sums.update((directory / 'go.sum').read_text().splitlines())
            for item in config.get('Replace') or []:
                old, new = item['Old'], dict(item['New'])
                if not new.get('Version'): new['Path'] = str((directory / new['Path']).resolve())
                identity = (old['Path'], old.get('Version', ''))
                if identity in replacements and replacements[identity] != new:
                    raise ValueError('conflicting Go module replacements in the package graph: ' + old['Path'])
                replacements[identity] = new
            for value in unit.get('include_dirs', []):
                path = (root / value).resolve()
                if not path.is_dir(): raise ValueError('native include directory is missing: ' + str(path))
                includes.append(str(path))
            libraries += unit.get('libraries', [])
            units.append({'manifest': str(Path(package['manifest']).resolve()), 'name':unit['name'],
                          'module':module, 'source_root':str(directory), 'signatures':signatures})
    for module, directory in modules.items():
        # Dependency replacements cannot silently redirect an explicitly selected
        # root library. Version-specific redirects of those roots are ambiguous.
        for identity, replacement in list(replacements.items()):
            if identity[0] == module and (identity[1] or replacement.get('Version') or replacement['Path'] != directory):
                raise ValueError('Go replacement conflicts with selected root module: ' + module)
        replacements[(module, '')] = {'Path':directory}
    env['CGO_CPPFLAGS'] = shlex.join(['-I' + p for p in dict.fromkeys(includes)])
    env['CGO_LDFLAGS'] = shlex.join(['-L' + str(runtime.parent), '-lsn_runtime_min'] +
                                  ['-l' + p for p in dict.fromkeys(libraries)])
    inputs.update(explicit)
    identity = {'packages':packages, 'tools':tools, 'inputs':inputs,
                'compiler_sha256':sha(compiler), 'driver_sha256':sha(Path(__file__)),
                'provider_sha256':sha(Path(__file__).with_name('native_provider.py')),
                'contract_sha256':sha(Path(__file__).with_name('native_contract.py')),
                'runtime_sha256':sha(runtime), 'abi_header_sha256':sha(header),
                'environment_sha256':key(dict(os.environ)), 'optimization':optimization,
                'arithmetic':arithmetic, 'system':platform.system(), 'machine':platform.machine(),
                'pointer_bits':struct.calcsize('P') * 8}
    fingerprint = key(identity)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.go-graph-', dir=output) as folder:
        work = Path(folder)
        bridge = work / 'bridge'; bridge.mkdir()
        go_version = max(versions, key=lambda v:tuple(int(p) for p in v.split('.')))
        (bridge / 'go.mod').write_text('module sindarin.generated/graph\n\ngo ' + go_version + '\n')
        if sums: (bridge / 'go.sum').write_text('\n'.join(sorted(sums)) + '\n')
        edits = []
        for module in modules:
            major = re.search(r'(?:/v|\.v)([2-9][0-9]*)$', module)
            edits.append('-require=' + module + '@v' + (major.group(1) if major else '0') + '.0.0')
        for (module, version), replacement in replacements.items():
            old = module + ('@' + version if version else '')
            new = replacement['Path'] + ('@' + replacement['Version'] if replacement.get('Version') else '')
            edits.append('-replace=' + old + '=' + new)
        run(go + ['mod', 'edit'] + edits, cwd=bridge, env=env)
        (bridge / 'main.go').write_text('package main\nfunc main() {}\n')
        exports = []
        for index, unit in enumerate(units):
            (bridge / f'exports_{index:03d}.go').write_text(go_provider(unit['signatures'], unit['module'], main=False))
            exports += [s['binding']['symbol'] for s in unit['signatures']]
        if len(set(exports)) != len(exports): raise ValueError('duplicate Go export symbols in the package graph')
        # Resolve the combined module graph into generated files only. Original
        # manifests and sources remain readonly; Go applies its normal MVS rules.
        run(go + ['list', '-mod=mod', '-deps', '.'], cwd=bridge, env=env)
        deps, flags = go_dependencies(go, bridge, {'module':str(bridge)}, env)
        deps = {p:v for p,v in deps.items() if not Path(p).is_relative_to(work)}
        if os.name != 'nt' and '-pthread' not in flags: flags.append('-pthread')
        flags += ['-l' + p for p in dict.fromkeys(libraries)]
        archive = work / 'libsn_go_graph.a'
        run(go + ['build', '-mod=readonly', '-buildmode=c-archive', '-o', archive, '.'], cwd=bridge, env=env)
        symbols = {line.split()[-1] for line in run(nm + ['-g', '-U' if sys.platform == 'darwin' else '--defined-only', archive]).splitlines()
                   if line.split() and not line.rstrip().endswith(':')}
        if any(s not in symbols and '_' + s not in symbols for s in exports):
            raise ValueError('generated Go graph is missing an export')
        current = {}
        for root in roots: current.update(tree_inputs(root, excluded))
        current.update({p:sha(p) for p in explicit})
        if (current != inputs or any(sha(p) != v for p,v in deps.items()) or
                sha(runtime) != identity['runtime_sha256'] or sha(header) != identity['abi_header_sha256']):
            raise ValueError('Go graph inputs changed during build; no artifact was published')
        metadata = {'schema':1, 'kind':'aggregate-go-native-bridge', 'complete_package':False,
                    'fingerprint':fingerprint, 'units':units, 'archive':archive.name,
                    'archive_sha256':sha(archive), 'native_link_flags':flags,
                    'provenance':identity, 'dependency_sha256':deps,
                    'module_manifest':(bridge/'go.mod').read_text(),
                    'shared_runtime':{'archive':str(runtime),'sha256':sha(runtime)},
                    'cache_reusable':False, 'publication_nonce':uuid.uuid4().hex}
        (work / 'assembly.json').write_text(json.dumps(metadata, indent=2) + '\n')
        destination = output / fingerprint / key(metadata)
        destination.parent.mkdir(exist_ok=True)
        os.rename(work, destination)
    return {'archive':str(destination / archive.name), 'native_link_flags':flags,
            'assembly':str(destination/'assembly.json')}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--target', choices=('c', 'rust'), required=True)
    parser.add_argument('--optimization', choices=('0', '1', '2'), required=True)
    parser.add_argument('--arithmetic', choices=('checked', 'unchecked'), required=True)
    try:
        build(parser.parse_args())
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f'error: {error}', file=sys.stderr)
        sys.exit(1)
