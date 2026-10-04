#!/usr/bin/env python3
"""Independent nil array state, copies, defaults and native transport."""
import hashlib
import json
import os
from pathlib import Path
import sys


ORACLES = {
    'tests/integration/test_nil_array.sn': b'true\nfalse\n3\n',
    'tests/rgen/nil_array_transitions.sn': b'true\nfalse\n0\nfalse\ntrue\n2\ntrue\ntrue\ntrue\ntrue\n',
    'tests/rust-native/scalar_nil_array_native.sn': b'true\ntrue\nfalse\n0\nfalse\n',
    'tests/rgen/nil_array_constructions.sn': b'true\n2\n2\nfalse\n3\nfalse\n3\n0\nfalse\n4\n5\nfalse\n2\nword:b\nfalse\n',
    'tests/rgen/nil_array_concat_copy.sn': b'true\nfalse\nfalse\n4\n9\n[]\n[]\n1\n8\ntrue\ntrue\nfalse\n',
    'tests/rgen/nil_array_float_state.sn': b'true\nfalse\nfalse\ntrue\ntrue\ntrue\nfalse\nfalse\ntrue\ntrue\nfalse\n',
    'tests/rust-native/scalar_nil_array_implicit.sn': b'false\nfalse\ntrue\n',
    'tests/rgen/nil_array_default.sn': b'false\nfalse\ntrue\n',
    'tests/rgen/nil_array_fields_calls.sn': b'true\nfalse\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\nfalse\n',
    'tests/rgen/nil_array_hygiene.sn': b'6\n7\n7\ntrue\n',
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 90 successful nil array differential cases")
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
        "tests/rust-native/nil-next-native-array.sn.c"
    ).read_bytes()).hexdigest()
    report["oracle_scope"] = (
        "Nil array state is distinct from allocated empty arrays through defaults, "
        "assignment, literal returns, copies, concatenation, native NULL results, "
        "field/free/instance/static call contexts, array constructors and nested "
        "owned copies. Float comparisons retain C object-byte equality and "
        "private names avoid user identifiers. Borrowed array returns, qualified "
        "array parameters and general alias identity remain separate gaps."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 90 independent nil array C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
