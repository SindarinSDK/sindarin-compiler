#!/usr/bin/env python3
"""Independent C sizeof and borrowed reference-call contracts."""
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import sys


class Callbacks(c.Structure):
    _fields_ = [("first", c.c_char), ("callback", c.c_void_p), ("last", c.c_bool)]


def sizes(*types):
    return "".join(f"{c.sizeof(t)}\n" for t in types).encode("ascii")


ORACLES = {
    **{f"tests/rgen/sizeof_{kind}.sn": b"" for kind in ("function", "interface", "nil", "void")},
    # The canonical C emitter uses long long for standalone callable/nil/void
    # queries; callable fields and interfaces use pointers. These are different
    # contracts, independent of Rust's runtime callable representation.
    "tests/rgen/sizeof_c_callables.sn": sizes(c.c_longlong, c.c_longlong, c.c_longlong) + b"0\n41\n",
    "tests/rgen/sizeof_c_nonvalues.sn": sizes(c.c_longlong, c.c_longlong),
    "tests/rgen/sizeof_c_interfaces.sn": sizes(c.c_void_p),
    "tests/rgen/sizeof_c_callback_fields.sn": sizes(Callbacks),
    "tests/rgen/sizeof_c_lambda_operand.sn": sizes(c.c_longlong) + b"0\n",
    "tests/rust-native/scalar_sizeof_native_lambda.sn": sizes(c.c_longlong),
    "tests/integration/test_as_ref_on_ref_struct_param.sn": (
        b"compute: PASS\nmutate: PASS\nbumpSelf: PASS\nthreadWork: PASS\n"
        b"All as-ref-on-ref-struct tests passed!\n"
    ),
    "tests/rgen/reference_call_borrowed_handles.sn": b"32\n16\n16\n36\n18\n18\n18\n",
    "tests/rgen/reference_call_static_handles.sn": b"20\n10\n12\n12\n34\n17\n17\n",
    "tests/rgen/reference_call_member_handles.sn": b"6\n6\n7\n7\n7\n7\n",
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 126 successful sizeof/reference differential cases")
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
        "C fallback and pointer sizeof contracts, unevaluated lambda captures "
        "and native lambda queries; borrowed shared reference handles through "
        "free, instance and static calls, repeated aliases, returned handles, "
        "member/index arguments and read-only reentry. Reference structs in "
        "these controls use the existing thread field owner representation; "
        "general reference, interface and callable ABI/identity remains separate."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 126 independent sizeof/reference C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
