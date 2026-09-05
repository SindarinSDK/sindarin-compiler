#!/usr/bin/env python3
"""Focused closure comparisons; run alone after make build, from repository root.

The standard harness remains at its default 20 workers. This extra comparison
runs one compiler/program at a time and does not alter harness configuration.
Known C defects have separate source probes; they are never called C passes.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ROOT / "tests/rgen"
RUST_ONLY = {"closure_values_direct_ownership", "closure_values_order",
             "closure_values_computed_calls", "closure_values_shadow_calls",
             "closure_values_mutable_alias", "closure_values_owned_mutation",
             "closure_values_owned_string_callback_order",
             "closure_values_owned_string_reentrant",
             "closure_values_owned_string_transitive",
             "closure_values_owned_string_visibility"}
MODE_SENSITIVE = {"closure_values_snapshot_unsigned_boundaries"}
EXPECTED_PANICS = {"closure_values_snapshot_unsigned_zero_division",
                   "closure_values_snapshot_unsigned_zero_modulo"}


def checked(command):
    result = subprocess.run([str(x) for x in command], cwd=ROOT,
                            capture_output=True, timeout=120)
    if result.returncode:
        raise RuntimeError(f"{command}: exit {result.returncode}\n"
                           + result.stderr.decode(errors="replace"))
    return result.stdout


def main():
    positive = negative = panics = 0
    with tempfile.TemporaryDirectory(prefix="sn-closure-values-") as directory:
        temp = Path(directory)
        compiler = ROOT / "bin/sn"
        for source in sorted(SOURCES.glob("closure_values_*.sn")):
            if source.stem in MODE_SENSITIVE or source.stem in EXPECTED_PANICS:
                continue
            expected = source.with_suffix(".expected").read_bytes()
            targets = ["rust"] if source.stem in RUST_ONLY else ["rust", "c", "default"]
            for opt in range(3):
                for target in targets:
                    exe = temp / f"{source.stem}-{target}-{opt}"
                    flags = [] if target == "default" else ["--target", target]
                    checked([compiler, source.relative_to(ROOT), *flags, f"-O{opt}",
                             "--no-install", "-l", "1", "-o", exe])
                    actual = checked([exe])
                    if actual != expected:
                        raise AssertionError(f"{source.name} {target} O{opt}: "
                                             f"{actual!r} != {expected!r}")
                    positive += 1
            print(f"PASS {source.stem}: {', '.join(targets)} O0/O1/O2", flush=True)
        for stem in sorted(MODE_SENSITIVE):
            source = SOURCES / f"{stem}.sn"
            expected = source.with_suffix(".expected").read_bytes()
            for opt in range(3):
                for target in ["rust", "c", "default"]:
                    exe = temp / f"{stem}-{target}-unchecked-{opt}"
                    flags = [] if target == "default" else ["--target", target]
                    checked([compiler, source.relative_to(ROOT), *flags, "--unchecked",
                             f"-O{opt}", "--no-install", "-l", "1", "-o", exe])
                    actual = checked([exe])
                    if actual != expected:
                        raise AssertionError(f"{source.name} {target} unchecked O{opt}: "
                                             f"{actual!r} != {expected!r}")
                    positive += 1
            print(f"PASS {stem}: rust, c, default unchecked O0/O1/O2", flush=True)
            for opt in range(3):
                arithmetic = [] if opt < 2 else ["--checked"]
                for target in ["rust", "c", "default"]:
                    exe = temp / f"{stem}-{target}-checked-{opt}"
                    flags = [] if target == "default" else ["--target", target]
                    checked([compiler, source.relative_to(ROOT), *flags, *arithmetic,
                             f"-O{opt}", "--no-install", "-l", "1", "-o", exe])
                    result = subprocess.run([exe], cwd=ROOT, capture_output=True, timeout=120)
                    if result.returncode == 0:
                        raise AssertionError(f"{source.name} {target} checked O{opt}: expected failure")
                    if target == "rust" and b"checked arithmetic failed" not in result.stderr:
                        raise AssertionError(f"{source.name} Rust checked O{opt}: "
                                             f"stderr {result.stderr!r}")
                    panics += 1
            print(f"PASS {stem}: rust, c, default checked failure O0/O1/O2", flush=True)
        for stem in sorted(EXPECTED_PANICS):
            source = SOURCES / f"{stem}.sn"
            for opt in range(3):
                exe = temp / f"{stem}-rust-checked-{opt}"
                checked([compiler, source.relative_to(ROOT), "--target", "rust",
                         "--checked", f"-O{opt}", "--no-install", "-l", "1", "-o", exe])
                result = subprocess.run([exe], cwd=ROOT, capture_output=True, timeout=120)
                if result.returncode == 0 or b"checked arithmetic failed" not in result.stderr:
                    raise AssertionError(f"{source.name} checked O{opt}: "
                                         f"exit {result.returncode}, stderr {result.stderr!r}")
                panics += 1
            print(f"PASS {stem}: Rust checked panic O0/O1/O2", flush=True)
        for source in sorted((SOURCES / "errors").glob("closure_values_*.sn")):
            expected = source.with_suffix(".expected").read_text().strip()
            for opt in range(3):
                output = temp / "rejected.rs"
                output.unlink(missing_ok=True)
                result = subprocess.run([str(compiler), str(source.relative_to(ROOT)),
                    "--emit-rust", "--no-install", "-l", "1", f"-O{opt}", "-o", str(output)],
                    cwd=ROOT, capture_output=True, timeout=120)
                diagnostic = result.stderr.decode()
                first_error = diagnostic[diagnostic.find("Error:"):].splitlines()[0]
                if result.returncode <= 0 or first_error != expected or output.exists():
                    raise AssertionError(f"{source.name} O{opt}: {diagnostic}")
                negative += 1
        print(f"PASS: {positive} compiled executions; {panics} expected checked panics; "
              f"{negative} ordered rejections with no Rust artifact")


if __name__ == "__main__":
    main()
