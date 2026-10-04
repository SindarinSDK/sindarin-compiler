#!/usr/bin/env python3
"""Independent indexed string copies, lifetimes and evaluation order."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    "tests/integration/test_generics_functions.sn": (
        b"Test 1 passed: identity<int>\nTest 2 passed: identity<str>\n"
        b"Test 3 passed: first<int>\nTest 4 passed: first<str>\n"
        b"Test 5 passed: map<int, int>\nTest 6 passed: map<str, str>\n"
        b"Test 7 passed: filter<int>\nAll generic function tests passed!\n"
    ),
    "tests/rgen/indexed_string_return_generic.sn": b"original:alpha\nalpha!\noriginal:alpha\nalpha!\nchanged\n2\n",
    "tests/rgen/indexed_string_return_call_kinds.sn": b"alpha!\noriginal:alpha\nalpha\nbeta\nalpha\noriginal:changed\n2\n",
    "tests/rgen/indexed_string_return_computed.sn": b"beta\n2\n",
    "tests/rgen/indexed_string_return_order.sn": b"beta\n12\n",
    "tests/rgen/indexed_string_initializer_hygiene.sn": b"35\nbeta\n1\n",
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 54 successful indexed string differential cases")
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
        "One unchanged generic-function original; independent returned string "
        "mutation, free/instance/static calls, computed owner lifetime and "
        "single evaluation, owner-before-index order and local initialization "
        "with helper-name collisions. General aggregate/array/reference "
        "identity, aliases and native ABI remain separate obligations."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 54 independent indexed string C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
