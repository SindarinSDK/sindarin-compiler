#!/usr/bin/env python3
"""Require exact CRT exit statuses, buffered streams and callback order."""

import ctypes
import hashlib
import json
import os
from pathlib import Path
import tempfile

from stdio_order_compare import ROOT, NEWLINE, run

ORIGINAL = "tests/integration/test_exit.sn"
ORIGINAL_SHA256 = "1585eb3539b56ba5efa350008248b7658f046f53e6b6eadfd0a30f7e93043aa3"
STATIC_IMPORT = "tests/integration/test_static_import.sn"
STATIC_IMPORT_SHA256 = "3719288ac31bc3b519d1f24519cf16f0fee83d8e11b4a5145a869f85af15e2ae"
PLAIN = {
    "argument": (1, b"argument\n1\n"),
    "closure": (0, b"closure\n"),
    "helper_hygiene": (0, b"28\n"),
    "method": (0, b"method\n"),
    "plain": (0, b"plain\n"),
    "static": (0, b"static\n"),
    "value_match": (0, b"match\n"),
    "worker": (0, b"main\nworker\n"),
}
HOOK = "tests/rust-native/builtin_exit_hooks.sn"
INITIALIZER = "tests/rust-native/builtin_exit_initializer.sn"
REFERENCE = "tests/rust-native/builtin_exit_hooks.c"
MODES = ("default", "checked", "unchecked")
OPTIMIZATIONS = ("-O0", "-O1", "-O2")


def specifications():
    yield ORIGINAL, [], 1, b"Before exit\n", b""
    yield STATIC_IMPORT, [], 0, b"otherValueTest::other value <-\nmain::This is my value\nmain::mutated\n", b""
    for name, (status, output) in PLAIN.items():
        yield f"tests/rgen/builtin_exit_{name}.sn", [], status, output, b""
    # Native C independently prints the narrowed int result. Python's C ABI
    # conversion supplies an additional oracle; both targets must match it.
    assert ctypes.sizeof(ctypes.c_int) == 4
    for value in (0, 1, 255, 256, -1, 4294967297):
        code = ctypes.c_int(value).value
        status = code & (0xffffffff if os.name == "nt" else 0xff)
        output = (b"native-before\n" + f"{code}\n".encode()
                  + b"body\n1\ncallback-B\ncallback-A\n")
        yield HOOK, [str(value)], status, output, b"stderr-B\nstderr-A\n"
    yield INITIALIZER, [], 1, (
        b"native-before\ninitializer\nbody\n42\ncallback-B\ncallback-A\n"
    ), b"stderr-B\nstderr-A\n"


def verify_case(case, status, output, error):
    for data in case["targets"].values():
        if data["compile"]["status"] != 0:
            return False
        separate, merged = data.get("separate", {}), data.get("merged", {})
        if (separate.get("status") != status or merged.get("status") != status
                or separate.get("stdout_hex") != output.hex()
                or separate.get("stderr_hex") != error.hex()
                or merged.get("stdout_hex") != (error + output).hex()
                or merged.get("stderr_hex") != ""):
            return False
    return len(case["targets"]) == 2


def main():
    compiler = ROOT / ("bin/sn.exe" if os.name == "nt" else "bin/sn")
    assert hashlib.sha256((ROOT / ORIGINAL).read_bytes()).hexdigest() == ORIGINAL_SHA256
    assert hashlib.sha256((ROOT / STATIC_IMPORT).read_bytes()).hexdigest() == STATIC_IMPORT_SHA256
    specs = list(specifications())
    assert len(specs) == 17 and len({source for source, *_ in specs}) == 12
    for name, (status, output) in PLAIN.items():
        base = ROOT / f"tests/rgen/builtin_exit_{name}"
        assert base.with_suffix(".expected").read_bytes() == output
        if status:
            assert int(base.with_suffix(".exit").read_text()) == status
    report = {
        "compiler_sha256": hashlib.sha256(compiler.read_bytes()).hexdigest(),
        "native_reference_sha256": hashlib.sha256((ROOT / REFERENCE).read_bytes()).hexdigest(),
        "process_exit_cases": 153,
        "executions_per_case": 4,
        "cases": [],
    }
    with tempfile.TemporaryDirectory(prefix="sn-builtin-exit-") as directory:
        for index, (source, arguments, status, output, error) in enumerate(specs):
            output, error = output.replace(b"\n", NEWLINE), error.replace(b"\n", NEWLINE)
            digest = hashlib.sha256((ROOT / source).read_bytes()).hexdigest()
            for mode in MODES:
                for optimization in OPTIMIZATIONS:
                    case = {"source": source, "source_sha256": digest,
                            "arguments": arguments, "arithmetic_mode": mode,
                            "optimization": optimization, "expected_status": status,
                            "expected_stdout_hex": output.hex(),
                            "expected_stderr_hex": error.hex(), "targets": {}}
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
                    case["passed"] = verify_case(case, status, output, error)
                    report["cases"].append(case)
                    print("PASS" if case["passed"] else "FAIL", source, arguments,
                          mode, optimization, flush=True)
    report["passed"] = len(report["cases"]) == 153 and all(c["passed"] for c in report["cases"])
    report["independent_oracle_cases"] = sum(c["passed"] for c in report["cases"])
    path = ROOT / ".sn/rust-parity-exit.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n")
    if report["passed"]:
        print("PASS: 153 process-exit oracles, 612 executions; exact status, streams, order and CRT lifecycle")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
