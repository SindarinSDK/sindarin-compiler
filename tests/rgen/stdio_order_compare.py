#!/usr/bin/env python3
"""Require original C diagnostic statuses, streams and redirected stream order."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
NEWLINE = b'\r\n' if os.name == 'nt' else b'\n'
SOURCES = {
    'assert_heap_message_order': ('e212a9041417fe1a8183b1ae4660c29f4b6d0a90c3e8e8226e3a86f651675567', b'message\ncondition\n', b'bad\n', 1),
    'stdio_helper_hygiene': ('5d71b5d3d8abe955bc8b7aac4a92bf1997eecc77686555585a05c8f2d8502176', b'55\n', b'', 0),
    'test_assert': ('9b3f568e34fe5c8b55ebc9de11ef38e85693c851d7983ac05ac9914493790b54',
                    b'Testing assert\nFirst assert passed\nSecond assert passed\n',
                    b'Assertion failed: expected condition to be true\n', 1),
    'test_thread_panic_propagate':
        ('bd30fcea8645c43374581e59501a53a475c00fe184523f245df2c8bcce657cc4',
         b'Testing thread panic propagation\n', b'panic: Division by zero\n', 1),
}


def run(command, merged=False):
    try:
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT if merged else subprocess.PIPE,
                                timeout=120)
        return {'command': [str(x) for x in command], 'status': result.returncode,
                'stdout_hex': result.stdout.hex(), 'stderr_hex': (result.stderr or b'').hex()}
    except subprocess.TimeoutExpired as error:
        return {'command': [str(x) for x in command], 'status': None, 'timeout': 120,
                'stdout_hex': (error.stdout or b'').hex(),
                'stderr_hex': (error.stderr or b'').hex()}


def main():
    compiler = ROOT / ('bin/sn.exe' if os.name == 'nt' else 'bin/sn')
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(), 'cases': []}
    with tempfile.TemporaryDirectory(prefix='sn-stdio-order-') as directory:
        for name, (digest, out, err, status) in SOURCES.items():
            source = ROOT / ('tests/rgen' if name in ('stdio_helper_hygiene', 'assert_heap_message_order') else
                             'tests/integration') / (name + '.sn')
            assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
            out, err = out.replace(b'\n', NEWLINE), err.replace(b'\n', NEWLINE)
            for mode in (('checked',) if name == 'test_thread_panic_propagate'
                         else ('default', 'checked', 'unchecked')):
                for optimization in ('-O0', '-O1', '-O2'):
                    case = {'source': str(source.relative_to(ROOT)), 'source_sha256': digest,
                            'mode': mode, 'optimization': optimization, 'targets': {}}
                    for target in ('c', 'rust'):
                        executable = Path(directory) / (name + mode + optimization + target + '.exe')
                        command = [compiler, source.relative_to(ROOT), '--target', target,
                                   optimization, *([] if mode == 'default' else ['--' + mode]),
                                   '--no-install', '-l', '1', '-o', executable]
                        data = {'compile': run(command)}
                        if data['compile']['status'] == 0 and executable.is_file():
                            data['separate'] = run([executable])
                            data['merged'] = run([executable], merged=True)
                        case['targets'][target] = data
                    case['passed'] = all(
                        data.get('separate', {}).get('status') == status and
                        data['separate']['stdout_hex'] == out.hex() and
                        data['separate']['stderr_hex'] == err.hex() and
                        data.get('merged', {}).get('status') == status and
                        data['merged']['stdout_hex'] == (err + out).hex()
                        for data in case['targets'].values())
                    report['cases'].append(case)
                    print(('PASS' if case['passed'] else 'FAIL'), name, mode, optimization, flush=True)
    report['passed'] = len(report['cases']) == 30 and all(c['passed'] for c in report['cases'])
    output = ROOT / '.sn/rust-parity-stdio-order.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
