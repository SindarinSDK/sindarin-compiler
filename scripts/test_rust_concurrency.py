#!/usr/bin/env python3
"""Run identical deterministic concurrency sources through tagged C and Rust.

Requires an independently built v0.0.83 compiler via --tag-compiler. The supplied
compiler is never built or modified. Results include raw logs and byte hashes;
no output normalization or expected-output replacement is performed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, cwd, env, log, seconds):
    with log.open('wb') as stream:
        process = subprocess.Popen(command, cwd=cwd, env=env, stdout=stream,
                                   stderr=subprocess.STDOUT,
                                   start_new_session=os.name != 'nt')
        try:
            return process.wait(timeout=seconds)
        except subprocess.TimeoutExpired:
            if os.name == 'nt':
                process.kill()
            else:
                os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            return 124


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=Path('bin/sn'))
    parser.add_argument('--tag-compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tag-ldlibs', help='Explicit supplemental tagged toolchain link flags; recorded verbatim')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    compilers = {'tag': args.tag_compiler.resolve(), 'rust': args.compiler.resolve()}
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    report = {'optimization': '-O0', 'normalization': 'none', 'tag_ldlibs': args.tag_ldlibs,
              'compiler_hashes': {k: digest(v) for k, v in compilers.items()},
              'compiler_paths': {k: str(v) for k, v in compilers.items()},
              'results': []}
    sources = sorted((repo / 'tests/rust-concurrency').glob('*.sn'))
    sources += sorted((repo / 'tests/rgen/concurrency-promoted').glob('*.sn'))
    sources += sorted((repo / 'tests/rust-concurrency/errors').glob('*.sn'))
    for source in sources:
        result = {'path': str(source.relative_to(repo)), 'sha256': digest(source),
                  'kind': 'negative' if source.parent.name == 'errors' else 'execution'}
        for backend, compiler in compilers.items():
            work = output / source.stem / backend
            (work / 'tmp').mkdir(parents=True)
            executable = work / ('program.exe' if os.name == 'nt' else 'program')
            env = os.environ | {'TMPDIR': str(work / 'tmp')}
            command = [str(compiler), str(source.relative_to(repo)), '--no-install', '-O0', '-o', str(executable)]
            if backend == 'tag' and args.tag_ldlibs:
                env['SN_LDLIBS'] = args.tag_ldlibs
            if backend == 'rust':
                command += ['--target', 'rust']
                # A pure Rust program must not consult the Sindarin C driver.
                env['SN_CC'] = str(work / 'nonexistent-c-compiler')
            status = run(command, repo, env, work / 'compile.log', 60)
            evidence = {'compile': status}
            if status == 0:
                evidence['run'] = run([str(executable)], repo, env, work / 'run.log', 15)
                evidence['output_sha256'] = digest(work / 'run.log')
            result[backend] = evidence
        result['equal'] = (result['tag']['compile'] == result['rust']['compile'] == 0
                           and result['tag'].get('run') == result['rust'].get('run') == 0
                           and result['tag']['output_sha256'] == result['rust']['output_sha256'])
        if result['kind'] == 'negative':
            result['equal'] = (result['tag']['compile'] == result['rust']['compile'] == 1
                               and (output / source.stem / 'tag/compile.log').read_bytes()
                               == (output / source.stem / 'rust/compile.log').read_bytes())
        report['results'].append(result)
        print(f"{source.name}: {'PASS' if result['equal'] else 'FAIL'}", flush=True)
        (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return int(not all(r['equal'] for r in report['results']))


if __name__ == '__main__':
    sys.exit(main())
