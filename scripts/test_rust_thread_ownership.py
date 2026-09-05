#!/usr/bin/env python3
"""Fresh tagged smoke followed by raw O0 thread ownership comparisons on spark1."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess


def capture(command, cwd, env, output, limit):
    p = subprocess.Popen(command, cwd=cwd, env=env, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, start_new_session=True)
    try:
        stdout, stderr = p.communicate(timeout=limit)
        status = p.returncode
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL)
        stdout, stderr = p.communicate()
        status = 124
    output.with_suffix('.stdout').write_bytes(stdout)
    output.with_suffix('.stderr').write_bytes(stderr)
    output.with_suffix('.json').write_text(json.dumps({'command': command, 'cwd': str(cwd), 'status': status}) + '\n')
    return status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--filter', help='Run feature filenames containing this text (smoke always runs)')
    parser.add_argument('--inventory', action='store_true', help='After smoke, record Rust-only catalog outcomes; no parity comparison')
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=False)
    repo = Path(__file__).resolve().parent.parent
    ref = json.loads(Path('/tmp/sindarin-tagged-control-reference.json').read_text())
    # Canonical shared reference schema; retain the verified Spark1 v1 layout.
    control_cwd = ref.get('repository', {}).get('worktree') or ref.get('cwd')
    if not control_cwd: raise SystemExit('HARNESSISSUE: reference has no repository.worktree')
    tag = Path(control_cwd); compiler = ref['compiler']['path']
    (out/'provenance.json').write_text(json.dumps({'reference': '/tmp/sindarin-tagged-control-reference.json', 'tag_peeled': ref['tag_peeled'], 'tag_compiler_verified_sha256': ref['compiler']['sha256'], 'rust_compiler_sha256': hashlib.sha256((repo/'bin/sn').read_bytes()).hexdigest(), 'mode': '-O0', 'inventory_only': args.inventory, 'filter': args.filter, 'control_worktree': str(tag), 'reference_schema': ref.get('schema'), 'reference_sha256': hashlib.sha256(Path('/tmp/sindarin-tagged-control-reference.json').read_bytes()).hexdigest()}, indent=2)+'\n')
    smoke = out/'smoke'; (smoke/'tmp').mkdir(parents=True)
    clean = {'HOME': '/home/gavin', 'PATH': '/usr/bin:/bin', 'TMPDIR': str(smoke/'tmp')}
    executable = smoke/'program'
    status = capture([compiler, 'tests/integration/test_int_negative.sn', '-O0', '--no-install', '-o', str(executable)], tag, clean, smoke/'compile', 60)
    ok = status == 0 and executable.is_file() and os.access(executable, os.X_OK)
    if ok:
        status = capture([str(executable)], tag, {'PATH': '/usr/bin:/bin'}, smoke/'run', 15)
        ok = status == 0 and (smoke/'run.stdout').read_bytes() == (tag/'tests/integration/test_int_negative.expected').read_bytes()
    (smoke/'result.json').write_text(json.dumps({'reference': '/tmp/sindarin-tagged-control-reference.json', 'pass': ok})+'\n')
    if not ok:
        print('HARNESSISSUE: tagged control failed; no feature interpretation'); return 125
    print('TAGGED_CONTROL_PASS', flush=True)
    sources = [tag/p for p in ['tests/integration/test_lambda_closure_thread_spawn.sn', 'tests/exploratory/test_limitation_fn_param.sn', 'tests/exploratory/test_limitation_lambda_param.sn', 'tests/exploratory/test_thread_with_lambda.sn', 'tests/integration/test_thread_array_spawn_loop.sn', 'tests/integration/test_thread_array_struct.sn', 'tests/integration/test_thread_panic_propagate.sn', 'tests/integration/test_thread_as_ref_spawn.sn', 'tests/integration/test_using_dispose.sn']]
    sources += sorted((repo/'tests/rust-thread-ownership').glob('*.sn'))
    if args.inventory:
        catalog = json.loads(Path('/tmp/sindarin-tagged-rust-corpus-evidence/failure-backlog.json').read_text())
        sources = [tag/p for p in sorted({p for group in catalog if 'threads yet' in group['diagnostic'] or 'global variables yet' in group['diagnostic'] for p in group['fixtures']})]
    results = []
    if args.filter:
        sources = [source for source in sources if args.filter in source.name]
        if not sources: raise SystemExit("No matching feature cases")
    for i, source in enumerate(sources):
        row = {'source': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
        source_repo = tag if source.is_relative_to(tag) else repo
        source_path = str(source.relative_to(source_repo))
        committed = subprocess.check_output(['git', 'show', 'HEAD:' + source_path], cwd=source_repo)
        if committed != source.read_bytes(): raise SystemExit('HARNESSISSUE: source differs from committed blob: ' + source_path)
        row['fixture_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source_repo, text=True).strip()
        row['git_blob'] = subprocess.check_output(['git', 'rev-parse', 'HEAD:' + source_path], cwd=source_repo, text=True).strip()
        backends = [('rust', str(repo/'bin/sn'), repo)] if args.inventory else [('tag', compiler, tag), ('rust', str(repo/'bin/sn'), repo)]
        for backend, cc, cwd in backends:
            d = out/str(i)/backend; (d/'tmp').mkdir(parents=True)
            env = (clean.copy() if backend == 'tag' else os.environ.copy()) | {'TMPDIR': str(d/'tmp')}
            if backend == 'rust': env['SN_CC'] = str(d/'missing-c-compiler')
            exe = d/'program'
            cmd = [cc, os.path.relpath(source, cwd), '-O0', '--no-install', '-o', str(exe)]
            if backend == 'rust': cmd += ['--target', 'rust']
            c = capture(cmd, cwd, env, d/'compile', 60)
            row[backend] = {'compile': c, 'executable': exe.is_file() and os.access(exe, os.X_OK)}
            if c == 0 and row[backend]['executable']:
                row[backend]['run'] = capture([str(exe)], cwd, env, d/'run', 15)
        if args.inventory:
            row['scope'] = 'Rust-only inventory, not parity'
            results.append(row); print(source.name, row, flush=True)
            (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
            continue
        row['expected_exit'] = 1 if source.name == 'test_thread_panic_propagate.sn' else 0
        row['equal'] = row['tag'].get('run') == row['rust'].get('run') == row['expected_exit']
        if row['equal']:
            row['equal'] = all((out/str(i)/'tag'/('run.'+stream)).read_bytes() == (out/str(i)/'rust'/('run.'+stream)).read_bytes() for stream in ['stdout', 'stderr'])
        oracle = source.with_suffix('.expected')
        if source.is_relative_to(tag) and row['expected_exit'] == 0 and oracle.exists() and row['tag'].get('run') == 0:
            row['tag_fixture_stdout_oracle'] = (out/str(i)/'tag/run.stdout').read_bytes() == oracle.read_bytes()
            row['equal'] = row['equal'] and row['tag_fixture_stdout_oracle']
        results.append(row); print(source.name, row, flush=True)
        (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return 0 if args.inventory else int(not all(x['equal'] for x in results))

if __name__ == '__main__':
    raise SystemExit(main())
