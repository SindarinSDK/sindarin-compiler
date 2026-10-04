#!/usr/bin/env python3
"""Independent deep copies and concatenation of owned value structs and nested arrays."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_struct_array_concat.sn': b'PASS\n',
    'tests/rgen/owned_array_concat_values.sn': b'2 a 2 10\nb 3 20\nchanged 9 other 8\n',
    'tests/rgen/owned_array_copy_values.sn': b'source 2 2\noriginal 9 copy 8\n',
    'tests/rgen/owned_array_concat_nested.sn': b'2 1 2 3 4\n9 8\n',
    'tests/rgen/owned_array_null_state.sn': b'true true false false\ntrue true false false\n1 true true true\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 45 successful owned-array differential cases")
    seen = set()
    for case in report["cases"]:
        source = case["source"].replace("\\", "/")
        identity = (source, case["optimization"], case["arithmetic_mode"])
        if not case["passed"] or identity in seen or identity not in required:
            raise ValueError(f"unexpected or duplicate case: {identity}")
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != case["source_sha256"]:
            raise ValueError(f"source changed: {source}")
        expected = ORACLES[source]
        if Path(source).with_suffix(".expected").read_bytes() != expected:
            raise ValueError(f"stale fixture oracle: {source}")
        if os.name == "nt":
            expected = expected.replace(b"\n", b"\r\n")
        if set(case["targets"]) != {"c", "rust"}:
            raise ValueError(f"missing target: {identity}")
        for target in ("c", "rust"):
            result = case["targets"][target]
            if result["compile"]["status"] != 0 or result["run"]["status"] != 0:
                raise ValueError(f"unsuccessful execution: {identity} {target}")
            if result["run"]["stdout_hex"] != expected.hex() or result["run"]["stderr_hex"]:
                raise ValueError(f"independent output mismatch: {identity} {target}")
    if seen != required:
        raise ValueError("incomplete optimization/arithmetic coverage")
    report["independent_oracle_cases"] = len(seen)
    report["oracle_scope"] = (
        "Array concatenation and copyOf recursively copy supported auto-copy "
        "value structs with owned strings, arrays and nested structs, and "
        "concatenation supports nested arrays. Results survive source replacement "
        "and cleanup, and nullable fields/arrays preserve nil versus allocated "
        "empty state. User-defined copy hooks, borrowed returns and general shared "
        "array/reference identity remain separate obligations."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 45 independent owned-array C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
