#!/usr/bin/env python3
"""Require every platform/group/gate, then assemble the existing evidence files."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from run_gates import CATALOG, gates_for

SYSTEMS = {'ubuntu-latest': 'Linux', 'macos-latest': 'Darwin', 'windows-latest': 'Windows'}


def collect(incoming, destination):
    catalog = json.loads(CATALOG.read_text())
    catalog_sha256 = hashlib.sha256(CATALOG.read_bytes()).hexdigest()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    seen = set()
    compilers = {}
    results = []
    for runner, system in SYSTEMS.items():
        root = destination / runner
        root.mkdir(parents=True, exist_ok=True)
        for group in catalog['groups']:
            expected = gates_for(catalog, group, system)
            if not expected:
                continue
            artifact = incoming / f'ci-evidence-{runner}-{group}'
            report = json.loads((artifact / 'ci-reports' / group / 'results.json').read_text())
            identity = (runner, group)
            if identity in seen or report['revision'] != revision or report['platform'] != system:
                raise ValueError(f'wrong/duplicate report: {identity}')
            if report['group'] != group or report['catalog_sha256'] != catalog_sha256:
                raise ValueError(f'wrong gate catalog: {identity}')
            if not report['passed'] or [g['id'] for g in report['gates']] != [g['id'] for g in expected]:
                raise ValueError(f'failed, missing or duplicated gates: {identity}')
            if any(gate['status'] != 0 for gate in report['gates']):
                raise ValueError(f'failed gate in {identity}')
            if system in compilers and compilers[system] != report['compiler_sha256']:
                raise ValueError(f'groups used different compilers on {system}')
            compilers[system] = report['compiler_sha256']
            seen.add(identity)
            results.append(report)
            for file in artifact.rglob('*'):
                if not file.is_file():
                    continue
                target = root / file.relative_to(artifact)
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists() and target.read_bytes() != file.read_bytes():
                    raise ValueError(f'evidence collision: {target}')
                shutil.copyfile(file, target)
    report = {'revision': revision, 'groups': len(seen), 'gates': sum(len(r['gates']) for r in results),
              'compilers': compilers, 'passed': True}
    (destination / 'summary.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('incoming', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    collect(args.incoming, args.destination)
