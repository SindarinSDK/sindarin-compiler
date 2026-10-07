# Compiler CI

`.github/workflows/ci.yml` is the single C/Rust compiler validation pipeline.
Release packaging remains in the release workflow.

Every push and manual run has a concurrency group containing its unique run ID,
so later pushes cannot replace either running or pending main validation. Pull
request revisions share a group by PR number and cancel superseded checks.
This lets development continue while earlier main revisions finish validation.
Final acceptance requires complete green CI on the final integrated revision.

Three build jobs produce one compiler bundle for each of Linux, macOS and
Windows. The bundle contains the staged `bin` tree and installed project
libraries. A tar archive preserves executable modes; a file manifest binds the
bundle to the checkout revision, platform and compiler checksum. Test jobs
restore and verify that bundle rather than compiling the compiler again.

The C, Rust, core, native, array/record, closure, callback and numeric/I/O groups
run independently on every platform. Two additional Linux groups run lifetime
and callback sanitizer checks. There are 26 test jobs. Each group executes its
gates sequentially, records their exit codes and elapsed times, and continues
after a failure so remaining diagnostics are retained. Differential cases use
four workers with independent executable paths and deterministic report order.

`scripts/ci/gates.json` retains all 98 gates from the previous Rust runtime
workflow and the five compiler-suite gates, plus structural-interface and
interface metadata lifetime validation and repeated native nil-result compiler
checks. Linux executes all 106 gates;
macOS and Windows each execute 82, retaining the original Linux-only sanitizer
scope. The original Windows compiler-suite Rust checks keep their default ABI;
the Rust/parity groups retain the pinned `x86_64-pc-windows-gnullvm` ABI.

The evidence job rejects missing groups, missing or duplicated gates, failures,
different compiler binaries within a platform, different revisions and different
gate catalogs. It assembles the existing `rust-runtime-diagnostics-*` artifacts.
Validated compiler artifacts retain their previous names. The final
`Compiler CI` check fails if builds, tests, evidence or artifact publication
failed, were cancelled or were skipped.

Local CI helper checks:

```sh
python3 -m unittest discover -s tests/ci -v
python3 scripts/ci/build_bundle.py pack .sn/compiler-bundle.tar.gz
python3 scripts/ci/run_gates.py --group c
SN_PARITY_WORKERS=4 python3 scripts/ci/run_gates.py --group rust
```

The normal developer `make test` and Rust-specific Make targets retain their
existing meanings. CI scheduling does not change source fixtures, expected
outputs, test exclusions or compiler language behavior.
