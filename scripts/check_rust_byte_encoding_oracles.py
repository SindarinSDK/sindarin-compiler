#!/usr/bin/env python3
"""Verify raw byte encodings independently of either compiler backend."""

import base64
import hashlib
import json
import os
from pathlib import Path
import sys

ORIGINAL = "tests/integration/test_byte_encoding.sn"
ORIGINAL_OUTPUT = (
    "Hex of Hello: 48656c6c6f\ntoString of Hello: Hello\n"
    "Base64 of Hello: SGVsbG8=\nBase64 of Man: TWFu\n"
    "Base64 of Ma: TWE=\nBase64 of M: TQ==\nHex of 0xAB: ab\n"
    "Hex of empty: ''\nBase64 of empty: ''\nLatin1 string: café\n"
    "PNG header hex: 89504e47\nAll byte encoding tests completed!\n"
).encode()
PREFIX = "tests/rgen/byte_encoding_"
NATIVE = "tests/rust-native/byte_encoding_object_view.sn"
JOIN = "tests/rgen/array_join_hygiene.sn"
ORACLES = {
    ORIGINAL: ORIGINAL_OUTPUT,
    PREFIX + "domain.sn": bytes(range(256)).hex().encode() + b"\n"
    + base64.b64encode(bytes(range(256))) + b"\n",
    PREFIX + "latin1_domain.sn": bytes(range(1, 256)).decode("latin1").encode(),
    PREFIX + "latin1_nul.sn": "0\nA\n4\néÿ\n0\n".encode(),
    PREFIX + "receivers.sn": b"17\n23\n4d616e\n1\nTWFu\n2\nMan\n3\nTWFu\n",
    PREFIX + "parameters.sn": "0102ff\nAQL/\nAé\n4d616e\n".encode(),
    PREFIX + "sync_capture_state.sn": b"4d616e\n4d616e\n42616e\n42616e\n43616e\n43616e\n42616e\n",
    JOIN: (
        "1-2-3\n4\n11\n21\n0,42,9223372036854775807,-1\n"
        "[0, 42, 9223372036854775807, -1]\n1-2-3\n[1, 2, 3]\n"
        "1-2-3\n1\n[1, 2, 3]\n41\n4/5\n1\n42\nA\n1\n"
    ).encode(),
}
# The language int is int64_t. C permits unsigned-char reads of its object
# representation. Check native C, the borrowed parameter, and returned storage
# against Python's independent host-byte-order encoding, rather than narrowing.
view = (513).to_bytes(8, sys.byteorder)[:2]
hex_view = view.hex().encode()
base64_view = base64.b64encode(view)
ORACLES[NATIVE] = b"\n".join([
    hex_view, hex_view, base64_view, hex_view, base64_view,
    *(f"0x{byte:02x}".encode() for byte in view),
]) + b"\n"


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 81 successful differential cases")
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
        if source.startswith(PREFIX) or source == JOIN:
            if Path(source).with_suffix(".expected").read_bytes() != expected:
                raise ValueError(f"stale fixture oracle: {source}")
        if os.name == "nt":
            expected = expected.replace(b"\n", b"\r\n")
        for target in ("c", "rust"):
            result = case["targets"][target]
            if result["compile"]["status"] != 0 or result["run"]["status"] != 0:
                raise ValueError(f"unsuccessful execution: {identity} {target}")
            if result["run"]["stdout_hex"] != expected.hex() or result["run"]["stderr_hex"]:
                raise ValueError(f"independent output mismatch: {identity} {target}")
    if seen != required:
        raise ValueError("incomplete optimization/arithmetic coverage")
    report["independent_oracle_cases"] = len(seen)
    report["native_reference_scope"] = (
        "Native C and Python host-byte-order int64 object prefixes; borrowed "
        "encoder parameters and returned byte arrays with wider scalar storage."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 81 independent C/Rust byte-encoding oracles, including native object views")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
