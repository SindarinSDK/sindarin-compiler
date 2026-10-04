#!/usr/bin/env python3
"""Independent nil string state, copies, defaults and native transport."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_nil_string.sn': b'true\nfalse\nhello\n',
    'tests/rgen/nil_string_transitions.sn': b'true\nfalse\n0\nfalse\ntrue\ntrue\ntrue\n',
    'tests/rgen/nil_string_print_concat.sn': b'[]\n\nfalse\na\nfalse\n0\nfalse\nb\n',
    'tests/rgen/nil_string_fields.sn': b'true\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\n',
    'tests/rust-native/scalar_nil_string_native.sn': b'true\ntrue\nfalse\nfalse\n0\ntrue\n',
    'tests/rust-native/scalar_nil_string_implicit.sn': b'true\n',
    'tests/rust-native/scalar_nil_string_default.sn': b'true\n',
    'tests/rust-native/scalar_nil_string_calls.sn': b'true\ntrue\ntrue\ntrue\ntrue\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 72 successful nil string differential cases")
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
    report["native_reference_sha256"] = hashlib.sha256(Path(
        "tests/rust-native/nil-next-native-string.sn.c"
    ).read_bytes()).hexdigest()
    report["oracle_scope"] = (
        "Nil remains distinct from allocated empty strings through assignment, "
        "field/array-element copies, literal returns, null-safe printing and "
        "concatenation; native NULL results, default string arguments and "
        "free/instance/static/native call contexts preserve null state. "
        "Nil arrays and C-undefined borrowed nil returns remain separate."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 72 independent nil string C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
