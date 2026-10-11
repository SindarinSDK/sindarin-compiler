#!/usr/bin/env python3
"""Run the accepted SDK's native checks with this platform's compiler bundle."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CHECKS = {'time-date': 'check_time_date_native.py', 'dtls': 'check_dtls_lifecycle.py', 'random': 'check_random_native.py', 'uuid': 'check_uuid_native.py', 'stringbuilder': 'check_stringbuilder_native.py'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', choices=CHECKS)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--compiler', type=Path,
                        default=ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn'))
    parser.add_argument('--sdk', type=Path, default=ROOT / '.sn/sdk-native-integration')
    args = parser.parse_args()
    command = [sys.executable, str(args.sdk.resolve() / 'scripts' / CHECKS[args.module]),
               '--compiler', str(args.compiler.resolve())]
    if args.sanitize:
        command.append('--sanitize')
    return subprocess.call(command, cwd=ROOT)


if __name__ == '__main__':
    raise SystemExit(main())
