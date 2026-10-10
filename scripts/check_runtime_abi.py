#!/usr/bin/env python3
"""Link C, Rust and Go clients against the single staged C runtime archive."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / 'tests/runtime_abi'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checked(command, env=None, cwd=ROOT):
    result = subprocess.run([str(arg) for arg in command], cwd=cwd, env=env,
                            capture_output=True, timeout=180)
    if result.returncode:
        raise RuntimeError(f'{command!r}\n{result.stderr.decode(errors="replace")}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true', help='Instrument the C implementation and C client')
    args = parser.parse_args()
    windows = os.name == 'nt'
    runtime = ROOT / 'bin/lib' / ('clang' if windows else 'gcc') / 'libsn_runtime_min.a'
    if not runtime.is_file():
        raise FileNotFoundError(runtime)
    if digest(ROOT / 'bin/include/runtime/sn_abi.h') != digest(ROOT / 'src/runtime/sn_abi.h'):
        raise ValueError('staged ABI header does not match the implementation source')
    report = {'platform': platform.system(), 'runtime_sha256': digest(runtime), 'cases': [],
              'scope': 'C runtime ABI values, resources, and ABI 1.3 array replacement; mutable package adapters remain required.'}
    output = ROOT / '.sn' / ('runtime-abi-sanitizers.json' if args.sanitize else 'runtime-abi.json')
    output.parent.mkdir(exist_ok=True)
    cc = shlex.split(os.environ.get('SN_CC') or ('clang' if windows or platform.system() == 'Darwin' else 'gcc'))
    with tempfile.TemporaryDirectory(prefix='sn-runtime-abi-') as folder:
        work = Path(folder)
        executable = work / ('client.exe' if windows else 'client')
        flags = ['-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-I', ROOT / 'bin/include/runtime']
        for source, label, wanted in [('client.c', 'C', b'shared runtime ABI: pass'),
                                     ('value_array_assign.c', 'C array replacement', b'managed array replacement: pass')]:
            if args.sanitize:
                runtime_sources = [ROOT / 'src/runtime' / name for name in
                                   ('sn_abi.c', 'sn_array.c', 'sn_string.c', 'sn_byte.c')]
                build = cc + ['-std=c11', '-D_GNU_SOURCE', '-g', '-O0', '-fno-omit-frame-pointer',
                              '-fsanitize=address,undefined'] + flags + [SOURCES / source] + runtime_sources
            else:
                build = cc + ['-std=c99'] + flags + [SOURCES / source, runtime]
            checked(build + ['-pthread', '-o', executable])
            run = checked([executable])
            if run.stdout.splitlines() != [wanted] or run.stderr:
                raise RuntimeError(f'{label} ABI client output mismatch: {run.stdout!r} {run.stderr!r}')
            report['cases'].append({'client': label, 'passed': True, 'sanitized': args.sanitize,
                                    **({'reference_credit_operations': 160000} if source == 'client.c' else {})})
        if not args.sanitize:
            rustc = shlex.split(os.environ.get('SN_RUSTC', 'rustc'))
            rustflags = shlex.split(os.environ.get('SN_RUSTFLAGS', ''))
            for source, label, wanted in [('client.rs', 'Rust', b'shared runtime ABI: pass'),
                                         ('value_array_assign.rs', 'Rust array replacement', b'managed array replacement: pass')]:
                checked(rustc + ['--edition=2021', SOURCES / source, '-L', runtime.parent,
                                '-o', executable] + rustflags)
                run = checked([executable])
                if run.stdout.splitlines() != [wanted] or run.stderr:
                    raise RuntimeError(f'{label} ABI client output mismatch: {run.stdout!r} {run.stderr!r}')
                report['cases'].append({'client': label, 'passed': True})
            go_project = work / 'go-client'
            shutil.copytree(SOURCES / 'go', go_project)
            # Go tracks headers in the package directory, rather than arbitrary
            # external include directories. Rebuild clients when the ABI changes.
            shutil.copyfile(ROOT / 'bin/include/runtime/sn_abi.h', go_project / 'sn_abi.h')
            env = os.environ.copy()
            env['CGO_ENABLED'] = '1'
            env['CC'] = shlex.join(cc)
            env['CGO_CPPFLAGS'] = ''  # cgo includes its package directory automatically
            env['CGO_LDFLAGS'] = shlex.join(['-L' + str(runtime.parent), '-lsn_runtime_min'])
            # The experiment catches retained Go pointers, including across GC.
            env['GOEXPERIMENT'] = 'cgocheck2'
            env['GOTOOLCHAIN'] = 'local'
            checked(['go', 'build', '-o', executable, '.'], env=env, cwd=go_project)
            run = checked([executable], env=env)
            if run.stdout.splitlines() != [b'shared runtime ABI: pass'] or run.stderr:
                raise RuntimeError(f'Go ABI client output mismatch: {run.stdout!r} {run.stderr!r}')
            report['cases'].append({'client': 'Go', 'passed': True, 'cgo_pointer_check': 'cgocheck2'})
            # Build the original Go backing as a native archive, then consume
            # its C-ABI exports from Rust in the same shared runtime process.
            archive = work / 'libgo_runtime_abi.a'
            checked(['go', 'build', '-buildmode=c-archive', '-o', archive, '.'], env=env, cwd=go_project)
            extra = ['-C', 'link-arg=-pthread'] if not windows else []
            checked(rustc + ['--edition=2021', '--cfg', 'go_bridge', SOURCES / 'client.rs',
                            '-L', work, '-L', runtime.parent, '-o', executable] + rustflags + extra)
            run = checked([executable], env=env)
            if run.stdout.splitlines() != [b'shared runtime ABI: pass'] or run.stderr:
                raise RuntimeError(f'Rust/Go ABI client output mismatch: {run.stdout!r} {run.stderr!r}')
            report['cases'].append({'client': 'Rust with Go-native archive', 'passed': True,
                                    'go_archive_sha256': digest(archive)})
        report['passed'] = all(case['passed'] for case in report['cases'])
        paths = [p for p in sorted(SOURCES.rglob('*')) if p.is_file()]
        paths += [ROOT / 'src/runtime/sn_abi.c', ROOT / 'src/runtime/sn_abi.h',
                  ROOT / 'scripts/check_runtime_abi.py']
        report['source_sha256'] = {p.relative_to(ROOT).as_posix(): digest(p) for p in paths}
        output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: ' + ', '.join(case['client'] for case in report['cases']) + ' shared C runtime ABI clients')


if __name__ == '__main__':
    main()
