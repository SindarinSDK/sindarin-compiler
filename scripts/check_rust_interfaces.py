#!/usr/bin/env python3
"""Run the complete structural-interface matrix and its frozen contracts."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

from check_rust_interface_oracles import ORACLES, PROBE_SHA256, verify, verify_sources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('.sn/rust-parity-interfaces.json'))
    args = parser.parse_args()
    verify_sources()
    fixtures = [*ORACLES, *PROBE_SHA256]
    output = args.output
    command = [sys.executable, 'scripts/check_rust_parity.py', *fixtures,
               '--require-count', str(len(fixtures)), '--output', str(output),
               '--parallel', os.environ.get('SN_PARITY_WORKERS', '4')]
    for mode in ('default', 'checked', 'unchecked'):
        command.extend(['--arithmetic-mode', mode])
    result = subprocess.run(command)
    if result.returncode:
        return result.returncode
    verify(output)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
