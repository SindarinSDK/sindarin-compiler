#!/usr/bin/env python3
"""Verify frozen void reflection and non-evaluation against independent oracles."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import tempfile

from check_rust_parity import compare_case

ROOT = Path(__file__).resolve().parents[1]
CONTROL = '''fn touch(count: int as ref): void =>
  count += 1
  println("unexpected touch")

fn describe(count: int as ref): TypeInfo =>
  return typeOf(touch(count))

fn main(): void =>
  var count: int = 0
  var first: TypeInfo = typeOf(touch(count))
  var second: TypeInfo = describe(count)
  var third: TypeInfo = typeOf(println("unexpected print"))
  println($"{count}:{first.name}:{first.fieldCount}:{first.typeId}")
  println($"{second.name}:{second.typeId == first.typeId}:{third.name}:{third.typeId == first.typeId}")
  println(typeOf(exit(83)).name)
'''


def digest(path):
    return hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    original = ROOT / 'tests/rgen/errors/typeof_void.sn'
    marker = json.loads(original.with_suffix('.parity.json').read_text())
    if (digest(original) != marker['source_sha256'] or
            digest(original.with_suffix('.expected')) != marker['legacy_oracle_sha256'] or
            marker['stdout_hex'] != b'void\n'.hex() or marker['exit'] != 0):
        raise ValueError('frozen void source or original diagnostic changed')
    if args.sanitize:
        os.environ.update(SN_CFLAGS=os.environ.get('SN_CFLAGS', '') +
                          ' -fsanitize=address,undefined -fno-omit-frame-pointer',
                          ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                          UBSAN_OPTIONS='halt_on_error=1')
    compiler = ROOT / 'bin' / ('sn.exe' if os.name == 'nt' else 'sn')
    metadata = ROOT / 'tests/rgen/typeof_metadata.sn'
    with tempfile.TemporaryDirectory(prefix='sn-void-reflection-') as folder:
        control = Path(folder) / 'void-effects.sn'
        control.write_text(CONTROL)
        fixtures = [(original, 'tests/rgen/errors/typeof_void.sn', b'void\n'),
                    (metadata, 'tests/rgen/typeof_metadata.sn',
                     metadata.with_suffix('.expected').read_bytes().replace(b'\r\n', b'\n')),
                    (control, 'control:void-effects',
                     b'0:void:0:1219850847\nvoid:true:void:true\nvoid\n')]
        jobs = [(source, label, expected, mode, opt)
                for source, label, expected in fixtures
                for mode in ('default', 'checked', 'unchecked')
                for opt in ('-O0', '-O1', '-O2')]
        settings = argparse.Namespace(c_ldlibs=None, compile_timeout=120, run_timeout=30)

        def verify(job):
            index, (source, label, expected, mode, opt) = job
            case = compare_case(compiler, source, index,
                                mode, opt, folder, settings)
            case['source'] = label
            if os.name == 'nt':expected = expected.replace(b'\n', b'\r\n')
            case['independent_oracle'] = expected.hex()
            case['passed'] = case['passed'] and all(
                target['compile']['status'] == 0 and
                target['run']['status'] == 0 and
                target['run']['stdout_hex'] == expected.hex() and
                target['run']['stderr_hex'] == ''
                for target in case['targets'].values())
            print(('PASS ' if case['passed'] else 'FAIL ') + label + ' ' + mode + ' ' + opt, flush=True)
            return case

        with ThreadPoolExecutor(max_workers=int(os.environ.get('SN_PARITY_WORKERS', '2'))) as workers:
            cases = list(workers.map(verify, enumerate(jobs)))
    report = {'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
              'oracle_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'sanitizers': args.sanitize,
              'environment': {name: os.environ.get(name) for name in
                              ('SN_CC', 'SN_CFLAGS', 'SN_RELEASE_CFLAGS', 'SN_RUSTFLAGS',
                               'ASAN_OPTIONS', 'UBSAN_OPTIONS')},
              'control_source': CONTROL, 'historical_sources_preserved': True,
              'original_diagnostic_sha256': digest(original.with_suffix('.expected')),
              'cases': cases, 'independent_oracle_cases': len(cases),
              'passed': len(cases) == 27 and all(case['passed'] for case in cases)}
    output = ROOT / '.sn' / ('rust-parity-void-reflection-sanitizers.json' if args.sanitize
                             else 'rust-parity-void-reflection.json')
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if not report['passed']:raise SystemExit(1)
    print('PASS: 27 independent void/metadata reflection mode cases')


if __name__ == '__main__':
    main()
