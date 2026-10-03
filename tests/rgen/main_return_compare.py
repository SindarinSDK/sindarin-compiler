#!/usr/bin/env python3
"""Require normal main return to preserve CRT callbacks and buffered streams."""

import ctypes
import hashlib
import json
import os
from pathlib import Path
import tempfile

from builtin_exit_compare import MODES, OPTIMIZATIONS, REFERENCE
from stdio_order_compare import ROOT, NEWLINE, run

PLAIN = {
    "fallthrough": ([], b"body\n"),
    "early": ([], b"1\n"),
    "initializer": ([], b"initializer\nbody\n42\n"),
    "args": (["argument"], b"2\n"),
    "hygiene": ([], b"28\n"),
    "integer_fallthrough": ([], b"integer fallthrough\n"),
}


def specifications():
    for name, (arguments, body) in PLAIN.items():
        yield f"tests/rust-native/main_return_{name}.sn", arguments, 0, (
            b"native-before\n" + body + b"callback-B\ncallback-A\n"
        ), b"stderr-B\nstderr-A\n"
    assert ctypes.sizeof(ctypes.c_int) == 4
    for value in (0, 1, 255, 256, -1, 4294967297):
        code = ctypes.c_int(value).value
        status = code & (0xffffffff if os.name == "nt" else 0xff)
        yield "tests/rust-native/main_return_integer.sn", [str(value)], status, (
            b"native-before\n" + f"{code}\n".encode()
            + b"body\n1\ncallback-B\ncallback-A\n"
        ), b"stderr-B\nstderr-A\n"


def merged_output(source, output, error):
    # The authoritative C entrypoint flushes stdout only at fallthrough.
    # Explicit returns bypass that footer and leave the stream for CRT exit.
    fallthrough = ("fallthrough", "args", "hygiene", "integer_fallthrough")
    if any(source.endswith("_" + name + ".sn") for name in fallthrough):
        callbacks = b"callback-B\ncallback-A\n".replace(b"\n", NEWLINE)
        assert output.endswith(callbacks)
        return output[:-len(callbacks)] + error + callbacks
    return error + output


def verify_case(case, status, output, error, merged):
    for data in case["targets"].values():
        if data["compile"]["status"] != 0:
            return False
        separate, combined = data.get("separate", {}), data.get("merged", {})
        if (separate.get("status") != status or combined.get("status") != status
                or separate.get("stdout_hex") != output.hex()
                or separate.get("stderr_hex") != error.hex()
                or combined.get("stdout_hex") != merged.hex()
                or combined.get("stderr_hex") != ""):
            return False
    return len(case["targets"]) == 2


def main():
    compiler = ROOT / ("bin/sn.exe" if os.name == "nt" else "bin/sn")
    specs = list(specifications())
    assert len(specs) == 12 and len({source for source, *_ in specs}) == 7
    report = {
        "compiler_sha256": hashlib.sha256(compiler.read_bytes()).hexdigest(),
        "native_reference_sha256": hashlib.sha256((ROOT / REFERENCE).read_bytes()).hexdigest(),
        "main_return_cases": 108,
        "executions_per_case": 4,
        "cases": [],
    }
    with tempfile.TemporaryDirectory(prefix="sn-main-return-") as directory:
        for index, (source, arguments, status, output, error) in enumerate(specs):
            output, error = output.replace(b"\n", NEWLINE), error.replace(b"\n", NEWLINE)
            merged = merged_output(source, output, error)
            digest = hashlib.sha256((ROOT / source).read_bytes()).hexdigest()
            for mode in MODES:
                for optimization in OPTIMIZATIONS:
                    case = {"source": source, "source_sha256": digest,
                            "arguments": arguments, "arithmetic_mode": mode,
                            "optimization": optimization, "expected_status": status,
                            "expected_stdout_hex": output.hex(),
                            "expected_stderr_hex": error.hex(),
                            "expected_merged_stdout_hex": merged.hex(), "targets": {}}
                    for target in ("c", "rust"):
                        executable = Path(directory) / f"{index}-{mode}-{optimization}-{target}.exe"
                        data = {"compile": run([
                            compiler, source, "--target", target, optimization,
                            *([] if mode == "default" else ["--" + mode]),
                            "--no-install", "-l", "1", "-o", executable,
                        ])}
                        if data["compile"]["status"] == 0 and executable.is_file():
                            data["separate"] = run([executable, *arguments])
                            data["merged"] = run([executable, *arguments], merged=True)
                        case["targets"][target] = data
                    case["passed"] = verify_case(case, status, output, error, merged)
                    report["cases"].append(case)
                    print("PASS" if case["passed"] else "FAIL", source, arguments,
                          mode, optimization, flush=True)
    report["passed"] = len(report["cases"]) == 108 and all(c["passed"] for c in report["cases"])
    report["independent_oracle_cases"] = sum(c["passed"] for c in report["cases"])
    path = ROOT / ".sn/rust-parity-main-return.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n")
    if report["passed"]:
        print("PASS: 108 normal-return oracles, 432 executions; exact status, streams and CRT callback order")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
