#!/usr/bin/env python3
"""Require interface provenance to be empty after all source owners are gone."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from check_rust_interface_oracles import SOURCE_SHA256

SOURCES = [f'tests/rust-native/native_interface_metadata_{name}.sn'
           for name in ('scope', 'growth', 'capture')]
SOURCES.append('tests/rust-native/native_interface_returned_array_scope.sn')


def audit_expectations(windows):
    # Sindarin stdout follows the CRT contract; Rust eprintln! uses LF.
    return (b'true\r\n' if windows else b'true\n', b'metadata_entries:0\n')


def audit_passed(run, windows):
    expected_stdout, expected_stderr = audit_expectations(windows)
    return (run.returncode == 0 and run.stdout == expected_stdout
            and run.stderr == expected_stderr)


def main():
    compiler = Path('bin/sn.exe' if os.name == 'nt' else 'bin/sn').resolve()
    rustc = os.environ.get('SN_RUSTC', 'rustc')
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'cases': []}
    output = Path('.sn/rust-interface-lifetimes.json')
    output.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sn-interface-lifetimes-') as folder:
        for index, source in enumerate(SOURCES):
            source_hash = hashlib.sha256(Path(source).read_bytes()).hexdigest()
            if source_hash != SOURCE_SHA256[source]:
                raise ValueError(f'source changed: {source}')
            for optimization, level in (('-O0', '0'), ('-O1', '1'), ('-O2', '2')):
                directory = Path(folder)
                emitted = directory / f'audit_{index}_{level}.rs'
                build = subprocess.run([str(compiler), '--no-install', '--emit-rust',
                                        optimization, source, '-o', str(emitted)],
                                       capture_output=True, timeout=120)
                if build.returncode:
                    raise ValueError(build.stderr.decode(errors='replace'))
                code = emitted.read_text()
                registry = re.search(r'^struct (\w*interface_registry\w*);', code, re.M)
                if not registry or code.count('fn main()') != 1:
                    raise ValueError('missing provenance registry or entry point')
                code = code.replace('fn main()', 'fn __sn_lifetime_audit_main()', 1)
                code += ('\nfn main() { __sn_lifetime_audit_main(); '
                         'eprintln!("metadata_entries:{}", ' + registry.group(1) +
                         '::links().lock().unwrap().len()); }\n')
                emitted.write_text(code)
                executable = directory / f'audit_{index}_{level}.exe'
                build = subprocess.run([rustc, '--edition=2021', '-C', f'opt-level={level}',
                                        str(emitted), '-o', str(executable)],
                                       capture_output=True, timeout=120)
                if build.returncode:
                    raise ValueError(build.stderr.decode(errors='replace'))
                run = subprocess.run([str(executable)], capture_output=True, timeout=10)
                expected_stdout, expected_stderr = audit_expectations(os.name == 'nt')
                passed = audit_passed(run, os.name == 'nt')
                report['cases'].append({'source': source, 'source_sha256': source_hash,
                                        'optimization': optimization, 'status': run.returncode,
                                        'stdout_hex': run.stdout.hex(), 'stderr_hex': run.stderr.hex(),
                                        'expected_stdout_hex': expected_stdout.hex(),
                                        'expected_stderr_hex': expected_stderr.hex(),
                                        'passed': passed})
                report['passed'] = all(case['passed'] for case in report['cases'])
                output.write_text(json.dumps(report, indent=2) + '\n')
                print(('PASS' if passed else 'FAIL') + f' {source} {optimization}', flush=True)
                if not passed:
                    raise ValueError(f'interface lifetime audit failed: status={run.returncode}, '
                                     f'stdout={run.stdout!r}, stderr={run.stderr!r}')
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 12 interface lifetime audits')


if __name__ == '__main__':
    main()
