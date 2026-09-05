#!/usr/bin/env python3
"""Fresh tagged controls for thread array identity; no reference rewriting."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from test_rust_thread_ownership import capture, read_reference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reference', type=Path, default=Path('/tmp/sindarin-tagged-control-reference.json'))
    parser.add_argument('--filter')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    ref, tag, tag_sha, compiler, reference = read_reference(args.reference)
    if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=tag, text=True).strip() != tag_sha:
        raise SystemExit('HARNESSISSUE: tagged worktree identity mismatch')
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip()
    provenance = {'head': head, 'compiler': str(repo/'bin/sn'),
                  'compiler_sha256': hashlib.sha256((repo/'bin/sn').read_bytes()).hexdigest(),
                  'reference_path': str(args.reference), 'reference': reference,
                  'reference_sha256': hashlib.sha256(args.reference.read_bytes()).hexdigest(),
                  'tag': tag_sha, 'modes': ['default', 'checked', 'unchecked'], 'optimization': ['-O0', '-O2']}
    (out/'provenance.json').write_text(json.dumps(provenance, indent=2)+'\n')

    def run(source, cwd, cc, directory, flags, rust=False):
        (directory/'tmp').mkdir(parents=True)
        env = os.environ.copy() if rust else {'HOME': '/home/gavin', 'PATH': '/usr/bin:/bin'}
        env['TMPDIR'] = str(directory/'tmp')
        if rust:
            env['SN_CC'] = str(directory/'missing-c-compiler')
        executable = directory/'program'
        command = [cc, os.path.relpath(source, cwd), *flags, '--no-install', '-o', str(executable)]
        if rust:
            command += ['--target', 'rust']
        status = capture(command, cwd, env, directory/'compile', 60)
        if status or not executable.is_file() or not os.access(executable, os.X_OK):
            return {'compile': status, 'executable': executable.is_file()}
        status = capture([str(executable)], cwd, env, directory/'run', 15)
        return {'compile': 0, 'executable': True, 'run': status}

    smoke = run(tag/'tests/integration/test_int_negative.sn', tag, compiler, out/'smoke', ['-O0'])
    if smoke.get('run') != 0 or (out/'smoke/run.stdout').read_bytes() != (tag/'tests/integration/test_int_negative.expected').read_bytes():
        raise SystemExit('HARNESSISSUE: fresh tagged smoke failed')
    print('TAGGED_CONTROL_PASS', flush=True)
    sources = sorted((repo/'tests/rust-thread-array-identity').glob('*.sn*'))
    if args.filter:
        sources = [p for p in sources if args.filter in p.name]
    if not sources:
        raise SystemExit('No matching cases')
    results = []
    for original in sources:
        relative = original.relative_to(repo)
        committed = subprocess.check_output(['git', 'show', f'HEAD:{relative}'], cwd=repo)
        if committed != original.read_bytes():
            raise SystemExit(f'HARNESSISSUE: fixture differs from committed bytes: {relative}')
        source = original
        if original.suffix == '.raw':
            # Preserve the independent reviewer's exact unformatted source.
            source = out/original.name.removesuffix('.raw')
            source.write_bytes(committed)
        for mode in ('default', 'checked', 'unchecked'):
            for opt in ('-O0', '-O2'):
                directory = out/(original.name+'-'+mode+opt)
                flags = [opt]+([] if mode == 'default' else ['--'+mode])
                row = {'source': str(relative), 'sha256': hashlib.sha256(committed).hexdigest(), 'mode': mode, 'opt': opt}
                row['tag'] = run(source, tag, compiler, directory/'tag', flags)
                row['rust'] = run(source, repo, str(repo/'bin/sn'), directory/'rust', flags, True)
                row['equal'] = row['tag'].get('run') == row['rust'].get('run') == 0
                if row['equal']:
                    row['equal'] = all((directory/'tag'/('run.'+stream)).read_bytes() == (directory/'rust'/('run.'+stream)).read_bytes() for stream in ('stdout', 'stderr'))
                results.append(row)
                (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
                print(row, flush=True)
    return int(not all(row['equal'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
