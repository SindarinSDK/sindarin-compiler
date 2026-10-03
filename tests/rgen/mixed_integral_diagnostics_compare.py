#!/usr/bin/env python3
"""Verify C mixed-integer checked diagnostics, status and buffered stream order."""
from pathlib import Path
import hashlib
import json
import os
import tempfile

from stdio_order_compare import ROOT, NEWLINE, run

SOURCES = {
    'mixed_integral_checked_overflow': ('fb0e5195575bd025c992a0763642c4289024656838fa9c33613afee582906cdd', b'Runtime error: integer overflow in addition\n'),
    'mixed_integral_checked_div_zero': ('dab2baa188e26d47e4102eb01c3e88f659565860051d842c3b7403e00f786807', b'panic: Division by zero\n'),
    'mixed_integral_checked_mod_zero': ('be60f5d6feb4262c9f11a4d3ebc94be6ddb765b57e7ab7669459e403e1b89acf', b'panic: Modulo by zero\n'),
}


def main():
    compiler = ROOT / ('bin/sn.exe' if os.name == 'nt' else 'bin/sn')
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(), 'cases': []}
    with tempfile.TemporaryDirectory(prefix='sn-mixed-integral-diagnostics-') as directory:
        for name, (digest, message) in SOURCES.items():
            source = Path('tests/rgen') / (name + '.sn')
            assert hashlib.sha256((ROOT / source).read_bytes()).hexdigest() == digest
            out, err = b'before' + NEWLINE, message.replace(b'\n', NEWLINE)
            for optimization in ('-O0', '-O1', '-O2'):
                case = {'source': str(source), 'source_sha256': digest,
                        'mode': 'checked', 'optimization': optimization, 'targets': {}}
                for target in ('c', 'rust'):
                    executable = Path(directory) / (name + optimization + target + '.exe')
                    data = {'compile': run([compiler, source, '--target', target, optimization,
                                            '--checked', '--no-install', '-l', '1', '-o', executable])}
                    if data['compile']['status'] == 0 and executable.is_file():
                        data['separate'] = run([executable])
                        data['merged'] = run([executable], merged=True)
                    case['targets'][target] = data
                case['passed'] = all(
                    data.get('separate', {}).get('status') == 1 and
                    data['separate']['stdout_hex'] == out.hex() and
                    data['separate']['stderr_hex'] == err.hex() and
                    data.get('merged', {}).get('status') == 1 and
                    data['merged']['stdout_hex'] == (err + out).hex()
                    for data in case['targets'].values())
                report['cases'].append(case)
                print(('PASS' if case['passed'] else 'FAIL'), name, optimization, flush=True)
    report['passed'] = len(report['cases']) == 9 and all(c['passed'] for c in report['cases'])
    output = ROOT / '.sn/rust-parity-mixed-integral-diagnostics.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
