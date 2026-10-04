#!/usr/bin/env python3
"""Verify match control flow, byte characters and local atomic postfix effects."""

import hashlib
import json
import os
from pathlib import Path
import sys

# Fixed original-corpus outputs and separately calculated control oracles.
# Byte domains cover all 256 character values. The place trace checks C's
# negative-index length read, and captures retain a private array copy.
ORACLES = {
    'tests/integration/test_match_return_arms.sn': b'one\ntwo\nthree\nother\n',
    'tests/exploratory/test_match_primitives.sn': b'=== Match Primitives Tests ===\n\ntest_match_int: PASS\ntest_match_int_expr: PASS\ntest_match_long: PASS\ntest_match_long_large: PASS\ntest_match_double: PASS\ntest_match_str: PASS\ntest_match_str_interpolated: PASS\ntest_match_char: PASS\ntest_match_bool: PASS\ntest_match_bool_false: PASS\ntest_match_byte: PASS\ntest_match_int_multivalue: PASS\ntest_match_else_only: PASS\ntest_match_statement_context: PASS\ntest_match_nested: PASS\n\nAll match primitives tests passed!\n',
    'tests/exploratory/test_match_native.sn': b'=== Match Native Tests ===\n\ntest_match_native_int: PASS\ntest_match_native_long: PASS\ntest_match_native_double: PASS\ntest_match_native_char: PASS\ntest_match_native_struct_field: PASS\ntest_match_native_struct_sizes: PASS\ntest_match_in_struct_with_native: PASS\ntest_match_native_multivalue: PASS\ntest_match_native_as_expression: PASS\n\nAll match native tests passed!\n',
    'tests/exploratory/test_match_memory.sn': b'=== Match Memory Context Tests ===\n\ntest_shared_match_expression: PASS\ntest_shared_match_struct_return: PASS\ntest_shared_match_statement: PASS\ntest_shared_match_multiline: PASS\ntest_private_match: PASS\ntest_local_scope_with_match: PASS\ntest_match_in_loop_shared: PASS\n\nAll match memory context tests passed!\n',
    'tests/rgen/value_match_char_result.sn': b'a\n',
    'tests/rgen/value_match_multiline_body.sn': b'10\n',
    'tests/rgen/value_match_bool_multiline_body.sn': b'true\n',
    'tests/rgen/value_match_string_multiline_body.sn': b'one\n',
    'tests/rgen/value_match_float_multiline_body.sn': b'9.00000\n',
    'tests/rgen/value_match_break_prefix.sn': b'',
    'tests/rgen/value_match_continue_prefix.sn': b'',
    'tests/rgen/match_control_char_bytes.sn': bytes(range(256)),
    'tests/rgen/match_control_char_subject.sn': bytes(
        ord({0: "0", 97: "1", 90: "2"}.get(i, "3")) for i in range(256)),
    'tests/rgen/match_control_probes/char_subject.sn.raw': bytes(
        ord({0: "0", 97: "1", 255: "2"}.get(i, "3")) for i in range(256)),
    'tests/rgen/match_control_callable_returns.sn': b'11\n22\nvoid-one\n31\nvoid-other\n32\n',
    'tests/rgen/match_control_bool_returns.sn': b'17\n23\n',
    'tests/rgen/match_control_arm_scopes.sn': b'43\n4\n9\n6\n',
    'tests/rgen/match_control_local_atomic_lock.sn': b'7\n8\n7\n0xFF\n0x00\n0xFF\ntrue\ntrue\ntrue\n',
    'tests/rgen/match_control_char_places.sn': b'D\n3\nE\nA\n2\n@\ntrue\n1\ntrue\ntrue\nZ\n[\n',
    'tests/rgen/match_control_captured_char_array.sn': b'A\nB\nA\nD\nC\nD\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 180 successful match-control differential cases")
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
        "Four unchanged original corpus sources, seven byte-preserved promoted "
        "Rust negative sources, character subject/result byte domains, "
        "callable return scopes, arm-local bindings/loops, explicit local "
        "atomic locks, computed character places and by-value array captures."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 180 independent match-control C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
