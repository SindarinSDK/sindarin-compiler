#!/usr/bin/env python3
"""Compare unchanged positive fixtures through both executable targets.

This is a differential gate, not a replacement for negative tests or independent
output oracles. Both targets must compile and exit successfully, and their raw
stdout/stderr must agree. Never counts two failed compilations as parity.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def run(command, timeout):
    try:
        result = subprocess.run(command, capture_output=True, timeout=timeout)
        return {"command": command, "status": result.returncode,
                "stdout_hex": result.stdout.hex(), "stderr_hex": result.stderr.hex()}
    except subprocess.TimeoutExpired as error:
        return {"command": command, "status": None, "timeout": timeout,
                "stdout_hex": (error.stdout or b"").hex(),
                "stderr_hex": (error.stderr or b"").hex()}
    except OSError as error:
        return {"command": command, "status": None, "error": str(error)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixtures", nargs="+", type=Path)
    parser.add_argument("--compiler", type=Path, default=Path("bin/sn"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compile-timeout", type=int, default=120)
    parser.add_argument("--run-timeout", type=int, default=30)
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    if not compiler.is_file():
        parser.error(f"compiler does not exist: {compiler}")
    for fixture in args.fixtures:
        if not fixture.is_file() or fixture.suffix != ".sn":
            parser.error(f"expected an existing .sn fixture: {fixture}")
        # Fixtures with custom invocation or expected failure belong to their
        # dedicated harness until this gate can enforce their complete contract.
        for suffix in (".args", ".exit", ".panic"):
            if fixture.with_suffix(suffix).exists():
                parser.error(f"unsupported execution sidecar: {fixture.with_suffix(suffix)}")
    report = {"compiler": str(compiler),
              "compiler_sha256": hashlib.sha256(compiler.read_bytes()).hexdigest(),
              "cases": []}
    with tempfile.TemporaryDirectory(prefix="sn-differential-") as directory:
        for index, fixture in enumerate(args.fixtures):
            for optimization in ("-O0", "-O1", "-O2"):
                case = {"source": str(fixture), "source_sha256": hashlib.sha256(
                    fixture.read_bytes()).hexdigest(), "optimization": optimization,
                    "targets": {}}
                for target in ("c", "rust"):
                    executable = Path(directory) / f"case-{index}-{optimization}-{target}.exe"
                    build = run([str(compiler), str(fixture), "--target", target,
                                 optimization, "--no-install", "-o", str(executable)],
                                args.compile_timeout)
                    result = {"compile": build}
                    if build["status"] == 0 and executable.is_file():
                        result["run"] = run([str(executable)], args.run_timeout)
                    case["targets"][target] = result
                c = case["targets"]["c"].get("run")
                rust = case["targets"]["rust"].get("run")
                case["passed"] = bool(c and rust and c["status"] == rust["status"] == 0
                                      and c["stdout_hex"] == rust["stdout_hex"]
                                      and c["stderr_hex"] == rust["stderr_hex"])
                report["cases"].append(case)
                print(f'{"PASS" if case["passed"] else "FAIL"} {fixture} {optimization}',
                      flush=True)
    report["passed"] = all(case["passed"] for case in report["cases"])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
