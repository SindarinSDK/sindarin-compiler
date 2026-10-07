#!/usr/bin/env python3
"""Share the staged compiler and project dependencies without losing modes."""
import argparse
import hashlib
import json
import platform
from pathlib import Path, PurePosixPath
import subprocess
import tarfile


MANIFEST = Path('.sn/compiler-build-manifest.json')
ROOTS = ('bin', '.sn/sindarin-pkg-libs', 'sn.lock')


def digest(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def included(member):
    return None if '.git' in PurePosixPath(member.name).parts else member


def pack(destination):
    files = {}
    for root in ROOTS:
        path = Path(root)
        if not path.exists():
            continue
        paths = [path] if path.is_file() else path.rglob('*')
        for file in paths:
            if file.is_file() and '.git' not in file.parts:
                files[file.as_posix()] = digest(file)
    executable = 'bin/sn.exe' if platform.system() == 'Windows' else 'bin/sn'
    if executable not in files:
        raise ValueError('compiler is missing from the build bundle')
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    manifest = {'revision': revision, 'platform': platform.system(),
                'compiler': executable, 'compiler_sha256': files[executable],
                'files': dict(sorted(files.items()))}
    MANIFEST.parent.mkdir(exist_ok=True)
    MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(destination, 'w:gz', compresslevel=3, dereference=True) as archive:
        for root in ROOTS:
            if Path(root).exists():
                archive.add(root, filter=included)
        archive.add(MANIFEST)
    print(f'Packaged {len(files)} files from {revision}: {destination}')


def verify():
    manifest = json.loads(MANIFEST.read_text())
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    if revision != manifest['revision'] or platform.system() != manifest['platform']:
        raise ValueError('build bundle does not match the checkout/platform')
    for name, expected in manifest['files'].items():
        if not Path(name).is_file() or digest(Path(name)) != expected:
            raise ValueError(f'build artifact changed: {name}')
    print(f"Verified {len(manifest['files'])} build files and compiler {manifest['compiler_sha256']}")
    return manifest


def restore(source):
    with tarfile.open(source) as archive:
        archive.extractall('.', filter='data')
    verify()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('pack', 'restore', 'verify'))
    parser.add_argument('archive', nargs='?', type=Path)
    args = parser.parse_args()
    if args.operation == 'verify':
        verify()
    elif args.archive is None:
        parser.error('pack/restore require an archive path')
    elif args.operation == 'pack':
        pack(args.archive)
    else:
        restore(args.archive)
