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

# Windows keeps the C target's stdout in text mode, so its line terminators are
# CRLF. The Rust target writes SnString bytes directly and retains LF. Keep
# exact target-specific raw oracles instead of normalizing either byte stream.
WINDOWS_C_TEXT_STDOUT = {
    PROBES / "split_empty_utf8.sn": bytes.fromhex(
        "6469726563743d3cc37ca93e0d0a"
        "6a6f696e65643d3cc3a93e0d0a"
        "636f756e74733d322f312f312f320d0a"
        "696e64657865643d3cc37ca93e0d0a"
        "66756e6374696f6e3d3cc3a93e0d0a"
        "61727261793d3cc37ca93e0d0a"
    ),
    PROBES / "byte_array_to_string.sn": bytes.fromhex(
        "61667465724e756c3d3c413e2f310d0a"
        "6265666f72654e756c3d3c41ff3e2f320d0a"
        "6e6f4e756c3d3c41ff3e2f320d0a"
        "636f6e6361743d3c41ff213e2f330d0a"
    ),
    PROBES / "string_apis.sn": bytes.fromhex(
        "617070656e643d3c6162c3a93e2f340d0a"
        "62797465733d342f39372f39382f3139352f3136390d0a"
        "696e74733d2d34322f393232333337323033363835343737353830370d0a"
        "646f75626c65733d2d332e35303030302f3130302e30303030300d0a"
    ),
    PROBES / "array_text_utf8.sn": bytes.fromhex("5b22c3a9225d0d0a"),
    PROBES / "struct_text_bytes.sn": bytes.fromhex(
        "426f78207b2076616c75653a2022af22207d0d0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_literal.sn": bytes.fromhex("310d0a"),
    ROOT / "tests/rgen/invalid_utf8_concat.sn": bytes.fromhex("76616c6964af0d0a"),
    ROOT / "tests/rgen/invalid_utf8_interpolation.sn": bytes.fromhex(
        "696e76616c69643a20af0d0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_pattern.sn": bytes.fromhex(
        "66616c6c6261636b0d0a"
    ),
    ROOT / "tests/rgen/invalid_utf8_result.sn": bytes.fromhex("af0d0a"),
}

ARGV_SOURCE = PROBES / "raw_argv.sn"
UNIX_ARGV = [bytes([0xaf]), bytes.fromhex("c3a9"), b"plain"]
UNIX_ARGV_EXPECTED = bytes.fromhex("af0ac3a90a706c61696e0a")
# Windows starts from a UTF-16 command line. An unpaired low surrogate is
# transported by the Rust boundary as its explicit WTF-8 byte sequence.
WINDOWS_ARGV = ["\udcaf", "é", "plain"]
WINDOWS_ARGV_EXPECTED = bytes.fromhex("edb2af0ac3a90a706c61696e0a")
EXE_SUFFIX = ".exe" if os.name == "nt" else ""


def completed(command):
    if os.name == "nt":
        argv = [os.fsdecode(item) if isinstance(item, bytes) else os.fspath(item)
                for item in command]
    else:
        argv = [item if isinstance(item, bytes) else os.fsencode(item) for item in command]
    result = subprocess.run(argv, cwd=ROOT,
                            capture_output=True, timeout=120)
    return result, argv


def checked(command):
    result, argv = completed(command)
    if result.returncode:
        raise RuntimeError(
            f"command={argv!r}\n"
            f"status={result.returncode}\n"
            f"stdout_hex={result.stdout.hex()}\n"
            f"stderr_hex={result.stderr.hex()}\n"
            f"stderr_display={result.stderr.decode(errors='backslashreplace')}")
    return result, argv


def mismatch_diagnostic(source, target, opt, compiler, executable,
                        compile_result, compile_argv, run_result, run_argv):
    config = {
        key: os.environ.get(key, "<unset>")
        for key in ("SN_CC", "SN_CFLAGS", "SN_RELEASE_CFLAGS", "SN_LDFLAGS",
                    "SN_LDLIBS", "SN_RUSTC", "SN_RUSTFLAGS")
    }
    config_path = ROOT / "etc" / ("sn.windows.cfg" if os.name == "nt" else
                                  "sn.darwin.cfg" if os.uname().sysname == "Darwin" else
                                  "sn.linux.cfg")
    dlls = sorted(str(path) for parent in (compiler.parent, executable.parent)
                  for path in parent.glob("*.dll")) if os.name == "nt" else []
    return "\n".join([
        f"source={source.relative_to(ROOT)} target={target} opt=O{opt}",
        f"compiler={compiler} exists={compiler.is_file()}",
        f"executable={executable} exists={executable.is_file()} "
        f"size={executable.stat().st_size if executable.is_file() else '<missing>'}",
        f"compile_argv={compile_argv!r}",
        f"compile_status={compile_result.returncode}",
        f"compile_stdout_hex={compile_result.stdout.hex()}",
        f"compile_stderr_hex={compile_result.stderr.hex()}",
        f"run_argv={run_argv!r}",
        f"run_status={run_result.returncode}",
        f"run_stdout_hex={run_result.stdout.hex()}",
        f"run_stderr_hex={run_result.stderr.hex()}",
        f"config_env={config!r}",
        f"config_path={config_path} exists={config_path.is_file()}",
        f"runtime_dlls={dlls!r}",
        f"PATH={os.environ.get('PATH', '<unset>')}",
    ])


def main():
    if len(PARITY_CASES) != 10 or set(PARITY_CASES) != set(WINDOWS_C_TEXT_STDOUT):
        raise AssertionError("byte-string target oracle inventory must contain the same 10 cases")
    for source in [*PARITY_CASES, ARGV_SOURCE]:
        if not source.is_file():
            raise FileNotFoundError(f"required byte-string fixture is missing: {source}")

    compiler = ROOT / f"bin/sn{EXE_SUFFIX}"
    target_executions = 0
    pairs = 0
    with tempfile.TemporaryDirectory(prefix="sn-byte-strings-") as directory:
        temp = Path(directory)
        for source, expected in sorted(PARITY_CASES.items()):
            for opt in range(3):
                outputs = {}
                for target in ("c", "rust"):
                    executable = temp / f"{source.stem}-{target}-O{opt}{EXE_SUFFIX}"
                    compile_result, compile_argv = checked(
                        [compiler, source.relative_to(ROOT), "--target", target,
                         f"-O{opt}", "--no-install", "-l", "1", "-o", executable])
                    run_result, run_argv = checked([executable])
                    outputs[target] = run_result.stdout
                    target_expected = (WINDOWS_C_TEXT_STDOUT[source]
                                       if os.name == "nt" and target == "c" else expected)
                    if outputs[target] != target_expected:
                        raise AssertionError(
                            f"{source.name} {target} O{opt}: "
                            f"{outputs[target].hex()} != {target_expected.hex()}\n" +
                            mismatch_diagnostic(
                                source, target, opt, compiler, executable,
                                compile_result, compile_argv, run_result, run_argv))
                    target_executions += 1
                pairs += 1
                if os.name != "nt" and outputs["c"] != outputs["rust"]:
                    raise AssertionError(f"{source.name} O{opt}: raw target mismatch")
            if os.name == "nt":
                print(f"PASS {source.stem}: Windows C CRLF/Rust LF exact raw oracles "
                      "O0/O1/O2", flush=True)
            else:
                print(f"PASS {source.stem}: Unix C/Rust O0/O1/O2 raw bytes", flush=True)

        argv_values = WINDOWS_ARGV if os.name == "nt" else UNIX_ARGV
        argv_expected = WINDOWS_ARGV_EXPECTED if os.name == "nt" else UNIX_ARGV_EXPECTED
        for opt in range(3):
            executable = temp / f"raw-argv-rust-O{opt}{EXE_SUFFIX}"
            checked([compiler, ARGV_SOURCE.relative_to(ROOT), "--target", "rust",
                     f"-O{opt}", "--no-install", "-l", "1", "-o", executable])
            output = checked([executable, *argv_values])[0].stdout
            if output != argv_expected:
                raise AssertionError(
                    f"raw_argv.sn rust O{opt}: {output.hex()} != {argv_expected.hex()}")
        if os.name == "nt":
            print("PASS raw_argv: Windows Rust O0/O1/O2 UTF-16/WTF-8 argument transport",
                  flush=True)
            print(f"PASS: {pairs} platform-specific C/Rust output pairs "
                  f"({target_executions} target executions) + 3 Rust-only Windows "
                  "UTF-16/WTF-8 argv executions")
        else:
            print("PASS raw_argv: Unix Rust O0/O1/O2 raw OS argument bytes", flush=True)
            print(f"PASS: {pairs} C/Rust raw-output pairs "
                  f"({target_executions} target executions) + 3 Rust-only Unix "
                  "raw-byte argv executions")


if __name__ == "__main__":
    main()
