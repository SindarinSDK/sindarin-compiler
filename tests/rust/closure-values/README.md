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

Owned struct cases cover immutable visibility and lexical shadowing, independent
per-invocation mutation snapshots shared by callable aliases, whole-value
reassignment, ordered reentrant callbacks, transitive nested captures, and an
escaping read-only struct with owned string and array fields. Mutable plain
value structs use an invocation-local clone; the O2 boundary case verifies
native-width unsigned wrapping and postfix sequencing in that clone. The same
invocation-local snapshot now supports scalar fields in heap-owning structs,
including nested fields, escaping closures, sibling identity, and reentry.
Mutation through pointer-backed struct fields remains a separate owner-path
gap rather than being projected through a deep clone.

Nested array receiver cases keep the cell on the captured outer array, evaluate
the receiver index and pushed value once, and release each mutable borrow before
the closure can be called again. Captured `pop` uses the same callable-owned
storage for direct and nested receivers.

Expression-bodied `void` closures discard their body value. The mutation-tail
case covers both compound assignment and postfix increment of a captured cell.
