# Rust closure-value regressions

This Rust-private corpus promotes closure cases whose tagged files record an
older Rust rejection. The tagged sources and expectations under `tests/rgen`
remain unchanged for restoration identity. Run these cases through
`scripts/run_rust_tests.py rust-closure-values` and
`rust-closure-values-errors`.

The positive cases cover shared scalar cells, recursive and reentrant callable
identity, mutable value snapshots, and shared mutable owned strings. The error
case preserves the Rust diagnostic for floating-point `%=`. Plain `.expected`
files are runtime output oracles; `.expected.rs` files are exact Rust emission
oracles. This directory is not part of the C/default test runner.

Owned array cases preserve the tagged distinction between independent mutable
`array_copy` snapshots and `is_ref` array reassignment slots. Direct captured
index assignment remains in the tagged rejection corpus because its unchanged
C control does not produce an executable.
