#!/usr/bin/env python3
"""Exact C/Rust text-stream checks for printing, native flushes, and diagnostics."""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
EXE_SUFFIX = ".exe" if os.name == "nt" else ""
NEWLINE = b"\r\n" if os.name == "nt" else b"\n"

NATIVE_FLUSH_SOURCE = ROOT / "tests/rust-native/scalar_initializer_timing.sn"
NATIVE_FLUSH_EXPECTED = NEWLINE.join(
    [b"initializer", b"body-before-native", b"42", b"43", b""]
)
DIAGNOSTIC_SOURCE = ROOT / "tests/rgen/int_checked_overflow.sn"
DIAGNOSTIC_STDERR = b"Runtime error: integer overflow in addition" + NEWLINE
FIXTURE_HASHES = {
    NATIVE_FLUSH_SOURCE: "a0a0a69df8bc02392a56363bb00340d279ea157646e5ddd4015aced17caa1a5a",
    ROOT / "tests/rust-native/scalar_initializer_timing.expected":
        "17af02e5b7cffb2526f9f17cb22efe9c4c08dff2a636493abc8cf6319644f3c4",
    DIAGNOSTIC_SOURCE: "c2cdf6192628c189c74e593d35ffb65f57a100b1251f8a87eb0a267d8983a8f7",
}


def invoke(command):
    argv = [os.fspath(item) for item in command]
    return subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=120), argv


def require_status(label, result, argv, expected):
    if result.returncode != expected:
        raise AssertionError(
            f"{label}: status={result.returncode}, expected={expected}\n"
            f"argv={argv!r}\nstdout_hex={result.stdout.hex()}\n"
            f"stderr_hex={result.stderr.hex()}")


def compile_target(compiler, source, target, output, extra=()):
    result, argv = invoke([
        compiler, source.relative_to(ROOT), "--target", target, "-O0",
        *extra, "--no-install", "-l", "1", "-o", output,
    ])
    require_status(f"{source.name} {target} compile", result, argv, 0)
    if not output.is_file():
        raise AssertionError(f"{source.name} {target}: missing executable {output}")


def main():
    for source, expected_hash in FIXTURE_HASHES.items():
        if not source.is_file():
            raise FileNotFoundError(f"required transport fixture is missing: {source}")
        actual_hash = hashlib.sha256(source.read_bytes()).hexdigest()
        if actual_hash != expected_hash:
            raise AssertionError(
                f"transport fixture hash mismatch: {source.relative_to(ROOT)}: "
                f"{actual_hash} != {expected_hash}")

    compiler = ROOT / f"bin/sn{EXE_SUFFIX}"
    if not compiler.is_file():
        raise FileNotFoundError(f"compiler is missing: {compiler}")

    with tempfile.TemporaryDirectory(prefix="sn-text-transport-") as directory:
        temp = Path(directory)
        native_outputs = {}
        for target in ("c", "rust"):
            executable = temp / f"native-flush-{target}{EXE_SUFFIX}"
            compile_target(compiler, NATIVE_FLUSH_SOURCE, target, executable)
            result, argv = invoke([executable])
            require_status(f"native flush {target} run", result, argv, 0)
            if result.stderr:
                raise AssertionError(
                    f"native flush {target}: unexpected stderr {result.stderr.hex()}")
            if result.stdout != NATIVE_FLUSH_EXPECTED:
                raise AssertionError(
                    f"native flush {target}: stdout={result.stdout.hex()}, "
                    f"expected={NATIVE_FLUSH_EXPECTED.hex()}")
            native_outputs[target] = result.stdout
        if native_outputs["c"] != native_outputs["rust"]:
            raise AssertionError("native flush: exact C/Rust stdout mismatch")

        diagnostic_streams = {}
        for target in ("c", "rust"):
            executable = temp / f"diagnostic-{target}{EXE_SUFFIX}"
            compile_target(compiler, DIAGNOSTIC_SOURCE, target, executable,
                           extra=("--checked",))
            result, argv = invoke([executable])
            require_status(f"checked diagnostic {target} run", result, argv, 1)
            if result.stdout:
                raise AssertionError(
                    f"checked diagnostic {target}: unexpected stdout {result.stdout.hex()}")
            if result.stderr != DIAGNOSTIC_STDERR:
                raise AssertionError(
                    f"checked diagnostic {target}: stderr={result.stderr.hex()}, "
                    f"expected={DIAGNOSTIC_STDERR.hex()}")
            diagnostic_streams[target] = result.stderr
        if diagnostic_streams["c"] != diagnostic_streams["rust"]:
            raise AssertionError("checked diagnostic: exact C/Rust stderr mismatch")

    print("PASS native initializer/body/return printing and C flush: 2 target executions")
    print("PASS checked diagnostic stderr/status: 2 target executions")


if __name__ == "__main__":
    main()
