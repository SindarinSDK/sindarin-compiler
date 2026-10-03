#!/usr/bin/env python3
"""Require independent output oracles for the character/CRT differential gate."""

import hashlib
import json
import os
from pathlib import Path
import sys

ORIGINAL = "tests/integration/test_char_methods.sn"
ORIGINAL_OUTPUT = (
    "toString 'a': a\ntoUpper 'x': X\ntoLower 'Z': z\ntoInt 'A': 65\n"
    "isDigit '5': true\nisDigit 'a': false\nisAlpha 'x': true\nisAlpha '9': false\n"
    "isWhitespace ' ': true\nisWhitespace 'z': false\nisAlnum 'A': true\n"
    "isAlnum '3': true\nisAlnum '@': false\n'a'.toUpper().toString(): A\n"
).encode()
COMPREHENSIVE = "tests/exploratory/test_str_comprehensive.sn"
COMPREHENSIVE_OUTPUT = (
    "=== str Comprehensive Type Test ===\n\n"
    "1. Literals and variables\n   PASS\n2. String interpolation\n   PASS\n"
    "3. Comparison operations\n   PASS\n4. Concatenation\n   PASS\n"
    "5. Arrays\n   PASS\n6. Array equality\n   PASS\n7. 2D arrays\n   PASS\n"
    "11. Lambdas\n   PASS\n12. Functions\n   PASS\n13. Control flow\n   PASS\n"
    "14. Edge cases\n   PASS\n15. String methods\n   PASS\n"
    "\n=== All str tests passed! ===\n"
).encode()
ORIGINAL_ORACLES = {ORIGINAL: ORIGINAL_OUTPUT, COMPREHENSIVE: COMPREHENSIVE_OUTPUT}
SOURCES = {
    COMPREHENSIVE,
    ORIGINAL,
    "tests/rgen/character_methods_basic.sn",
    "tests/rgen/character_methods_receivers.sn",
    "tests/rgen/character_methods_predicates.sn",
    "tests/rgen/string_ctype_bytes.sn",
    "tests/rgen/string_operations.sn",
    "tests/rust-native/character_ctype_locale.sn",
}


def verify(path):
    report = json.loads(path.read_text())
    if not report["passed"] or len(report["cases"]) != 72:
        raise ValueError("expected 72 successful differential cases")
    seen = set()
    for case in report["cases"]:
        source = case["source"].replace("\\", "/")
        identity = (source, case["optimization"], case["arithmetic_mode"])
        if not case["passed"] or identity in seen or source not in SOURCES:
            raise ValueError(f"unexpected or duplicate case: {identity}")
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != case["source_sha256"]:
            raise ValueError(f"source changed: {source}")
        expected = ORIGINAL_ORACLES[source] if source in ORIGINAL_ORACLES else Path(source).with_suffix(".expected").read_bytes()
        if os.name == "nt":
            expected = expected.replace(b"\n", b"\r\n")
        for target in ("c", "rust"):
            result = case["targets"][target]
            if result["compile"]["status"] != 0 or result["run"]["status"] != 0:
                raise ValueError(f"unsuccessful execution: {identity} {target}")
            if result["run"]["stdout_hex"] != expected.hex() or result["run"]["stderr_hex"]:
                raise ValueError(f"independent output mismatch: {identity} {target}")
    required = {(source, optimization, mode) for source in SOURCES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if seen != required:
        raise ValueError("incomplete optimization/arithmetic coverage")
    report["independent_oracle_cases"] = len(seen)
    report["native_reference_scope"] = "All 256 byte values, C locale and environment locale; both locale selections must succeed."
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 72 independent C/Rust output oracles, including native CRT and char ABI references")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
