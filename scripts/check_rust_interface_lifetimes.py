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


def main():
    compiler = Path('bin/sn.exe' if os.name == 'nt' else 'bin/sn').resolve()
    rustc = os.environ.get('SN_RUSTC', 'rustc')
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'cases': []}
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
                newline = b'\r\n' if os.name == 'nt' else b'\n'
                passed = (run.returncode == 0 and run.stdout == b'true' + newline
                          and run.stderr == b'metadata_entries:0' + newline)
                report['cases'].append({'source': source, 'source_sha256': source_hash,
                                        'optimization': optimization, 'status': run.returncode,
                                        'stdout_hex': run.stdout.hex(), 'stderr_hex': run.stderr.hex(),
                                        'passed': passed})
                print(('PASS' if passed else 'FAIL') + f' {source} {optimization}', flush=True)
                if not passed:
                    raise ValueError('metadata outlived source ownership')
    report['passed'] = True
    output = Path('.sn/rust-interface-lifetimes.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 12 interface lifetime audits')


if __name__ == '__main__':
    main()
