#!/usr/bin/env python3
"""Exact raw-byte C/Rust string parity probes across O0/O1/O2."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROBES = ROOT / "tests/rgen/byte_string_probes"
PARITY_CASES = {
    PROBES / "split_empty_utf8.sn": bytes.fromhex(
        "6469726563743d3cc37ca93e0a"
        "6a6f696e65643d3cc3a93e0a"
        "636f756e74733d322f312f312f320a"
        "696e64657865643d3cc37ca93e0a"
        "66756e6374696f6e3d3cc3a93e0a"
        "61727261793d3cc37ca93e0a"
    ),
    PROBES / "byte_array_to_string.sn": bytes.fromhex(
        "61667465724e756c3d3c413e2f310a"
        "6265666f72654e756c3d3c41ff3e2f320a"
        "6e6f4e756c3d3c41ff3e2f320a"
        "636f6e6361743d3c41ff213e2f330a"
    ),
    PROBES / "string_apis.sn": bytes.fromhex(
        "617070656e643d3c6162c3a93e2f340a"
        "62797465733d342f39372f39382f3139352f3136390a"
        "696e74733d2d34322f393232333337323033363835343737353830370a"
        "646f75626c65733d2d332e35303030302f3130302e30303030300a"
    ),
    PROBES / "array_text_utf8.sn": bytes.fromhex("5b22c3a9225d0a"),
    PROBES / "struct_text_bytes.sn": bytes.fromhex(
        "426f78207b2076616c75653a2022af22207d0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_literal.sn": bytes.fromhex("310a"),
    ROOT / "tests/rgen/invalid_utf8_concat.sn": bytes.fromhex("76616c6964af0a"),
    ROOT / "tests/rgen/invalid_utf8_interpolation.sn": bytes.fromhex(
        "696e76616c69643a20af0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_pattern.sn": bytes.fromhex(
        "66616c6c6261636b0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_result.sn": bytes.fromhex("af0a"),
}

ARGV_SOURCE = PROBES / "raw_argv.sn"
ARGV = [bytes([0xaf]), bytes.fromhex("c3a9"), b"plain"]
ARGV_EXPECTED = bytes.fromhex("af0ac3a90a706c61696e0a")


def checked(command):
    argv = [item if isinstance(item, bytes) else os.fsencode(item) for item in command]
    result = subprocess.run(argv, cwd=ROOT,
                            capture_output=True, timeout=120)
    if result.returncode:
        raise RuntimeError(f"{command}: exit {result.returncode}\n"
                           + result.stderr.decode(errors="backslashreplace"))
    return result.stdout


def main():
    compiler = ROOT / "bin/sn"
    executions = 0
    with tempfile.TemporaryDirectory(prefix="sn-byte-strings-") as directory:
        temp = Path(directory)
        for source, expected in sorted(PARITY_CASES.items()):
            for opt in range(3):
                outputs = {}
                for target in ("c", "rust"):
                    executable = temp / f"{source.stem}-{target}-O{opt}"
                    checked([compiler, source.relative_to(ROOT), "--target", target,
                             f"-O{opt}", "--no-install", "-l", "1", "-o", executable])
                    outputs[target] = checked([executable])
                    if outputs[target] != expected:
                        raise AssertionError(
                            f"{source.name} {target} O{opt}: "
                            f"{outputs[target].hex()} != {expected.hex()}")
                    executions += 1
                if outputs["c"] != outputs["rust"]:
                    raise AssertionError(f"{source.name} O{opt}: raw target mismatch")
            print(f"PASS {source.stem}: C/Rust O0/O1/O2 raw bytes", flush=True)
        for opt in range(3):
            executable = temp / f"raw-argv-rust-O{opt}"
            checked([compiler, ARGV_SOURCE.relative_to(ROOT), "--target", "rust",
                     f"-O{opt}", "--no-install", "-l", "1", "-o", executable])
            output = checked([executable, *ARGV])
            if output != ARGV_EXPECTED:
                raise AssertionError(
                    f"raw_argv.sn rust O{opt}: {output.hex()} != {ARGV_EXPECTED.hex()}")
            executions += 1
        print("PASS raw_argv: Rust O0/O1/O2 raw OS argument bytes", flush=True)
    print(f"PASS: {executions} compiled raw-byte executions")


if __name__ == "__main__":
    main()
