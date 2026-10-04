#!/usr/bin/env python3
"""Independent execution oracles for C-only native helper partitioning."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    "tests/integration/test_struct_native_c_interop.sn": (
        b"=== Native Struct C Interop Test ===\n\n"
        b"Test 1: Native struct passed via 'as ref'\n"
        b"  PASS: Struct modified by C function via as ref\n"
        b"Test 2: Multiple C function calls modifying struct\n"
        b"  PASS: Multiple modifications work correctly\n"
        b"Test 3: Reading struct field via C function\n"
        b"  PASS: C function can read struct fields\n"
        b"Test 4: C function computing struct field\n"
        b"  PASS: Computed field set correctly\n"
        b"Test 5: Struct passed by value to C function\n"
        b"  PASS: Struct passed by value works\n"
        b"Test 6: Original struct unchanged after pass-by-value\n"
        b"  PASS: Original struct preserved after pass-by-value\n"
        b"\n=== All Native Struct C Interop Tests PASSED! ===\n"
    ),
    "tests/rust-native/scalar_c_only_helpers.sn": b"13\n5\n7\n",
    "tests/integration/test_native_struct_ref_array_pass.sn": (
        b"sindarin count: 3\nsingle widget: id=1, value=10\nsingle pass: 1\n"
        b"native count: 3\nwidget[0]: id=1, value=10\n"
        b"widget[1]: id=2, value=20\nwidget[2]: id=3, value=30\narray pass: 1\n"
    ),
    "tests/exploratory/test_struct_zlib_style.sn": (
        b"=== zlib-style Streaming Struct Tests ===\n\n"
        b"Test 1: Basic streaming pattern (zlib-style)\n"
        b"  PASS: Basic streaming works\n"
        b"Test 2: Multi-chunk streaming\n  PASS: Multi-chunk streaming works\n"
        b"Test 3: Output buffer exhaustion handling\n"
        b"  PASS: Output exhaustion handled correctly\n"
        b"Test 4: Resume streaming after buffer refill\n"
        b"  PASS: Resume streaming works\n\n=== All zlib-style Tests PASSED! ===\n"
    ),
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 36 successful C-only native helper differential cases")
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
        if (Path(source).with_suffix(".expected").is_file() and
                Path(source).with_suffix(".expected").read_bytes() != expected):
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
    report["native_reference_sha256"] = hashlib.sha256(Path(
        "tests/integration/test_native_struct_ref_array_pass_helper.c"
    ).read_bytes()).hexdigest()
    report["oracle_scope"] = (
        "Three unchanged native interop originals execute struct by-value and "
        "as-ref/array operations and streaming mutation wholly in C. Helpers remain in "
        "the sidecar; a shared scalar helper retains its Rust bridge for "
        "direct and lambda calls. Public aggregate ABI remains separate."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 36 independent C-only native helper C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
