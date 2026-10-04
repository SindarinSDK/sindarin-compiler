#!/usr/bin/env python3
"""Independent nested array stores, deep copies, negative indices and string order."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_nested_array_copy.sn': b'1 2 3\n4 5 6\n999 2 3\n4 5 6\n',
    'tests/rgen/nested_store_stable_indices.sn': b'1 2 3 4\n77 2 3 90 0 1\n',
    'tests/rgen/nested_store_string_indices.sn': b'a b c d\nupdated b c tail 1\n',
    'tests/rgen/nested_store_nullable_string.sn': b'true\ntrue\na b c d\nupdated b c tail 1\n',
    'tests/rgen/nested_store_deep_fields.sn': b'91 8\n4 99\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 45 successful nested store differential cases")
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
        "Stable nested array stores resolve indices before borrowing the final "
        "destination; deep copies retain independent mutation. Variable and literal "
        "negative indices, three dimensions, field owners and private-name "
        "collisions are covered. String destinations capture their indices before "
        "replacement calls mutate the index. Shared array identity and borrowed "
        "returns remain separate gaps."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 45 independent nested store C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
