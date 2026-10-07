#!/usr/bin/env python3
"""Run one CI group against the verified shared compiler, retaining every result."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time

from build_bundle import verify


CATALOG = Path(__file__).with_name('gates.json')


def gates_for(catalog, group, system):
    return [gate for gate in catalog['gates'] if gate['group'] == group
            and (not gate['linux_only'] or system == 'Linux')]


def run_group(group):
    manifest = verify()
    catalog = json.loads(CATALOG.read_text())
    selected = gates_for(catalog, group, platform.system())
    if not selected:
        raise ValueError(f'no gates for {group} on {platform.system()}')
    destination = Path('.sn/ci-reports') / group
    destination.mkdir(parents=True, exist_ok=True)
    report = {'group': group, 'platform': platform.system(),
              'revision': manifest['revision'], 'compiler_sha256': manifest['compiler_sha256'],
              'catalog_sha256': hashlib.sha256(CATALOG.read_bytes()).hexdigest(), 'gates': []}
    summary = os.environ.get('GITHUB_STEP_SUMMARY')
    for gate in selected:
        command = [sys.executable if arg == '{python}' else arg for arg in gate['command']]
        # Windows retains the original C harness's arena sanitizer override.
        if platform.system() == 'Windows' and gate['id'] == 'c-default':
            command.append('ARENA_SANITIZE=')
        environment = os.environ.copy()
        environment.update(gate.get('env', {}))
        print(f"::group::{gate['name']}", flush=True)
        started = time.monotonic()
        log = destination / (gate['id'] + '.log')
        try:
            with log.open('wb') as output:
                process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT, env=environment)
                while chunk := process.stdout.read1(8192):
                    output.write(chunk)
                    sys.stdout.buffer.write(chunk)
                    sys.stdout.buffer.flush()
                status = process.wait()
        except OSError as error:
            log.write_text(str(error) + '\n')
            print(error, file=sys.stderr)
            status = 127
        result = {'id': gate['id'], 'name': gate['name'], 'command': command,
                  'status': status, 'seconds': round(time.monotonic() - started, 3),
                  'log': log.as_posix()}
        report['gates'].append(result)
        report['passed'] = all(entry['status'] == 0 for entry in report['gates'])
        (destination / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
        print('::endgroup::', flush=True)
        label = 'PASS' if status == 0 else f'FAIL (exit {status})'
        line = f"- {label}: {gate['name']} ({result['seconds']:.1f}s)\n"
        if summary:
            with open(summary, 'a', encoding='utf-8') as output:
                output.write(line)
        print(line, end='', flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--group', required=True, choices=json.loads(CATALOG.read_text())['groups'])
    args = parser.parse_args()
    raise SystemExit(run_group(args.group))
