# Native variadic calls in the Rust backend

Resolved direct native calls with `...` use generated fixed-signature C adapters.
Each distinct declaration and argument-type shape gets a hygienic adapter; calls
with the same shape share it. Imported aliases and source dependencies retain
their resolved C symbols and origins.

For example, a call with a float tail becomes a typed Rust call to a C adapter
whose corresponding parameter is `float`. The adapter calls the original
variadic function. C promotes that float to double, using the actual target ABI.
The same rule handles bool, byte and char promotion to int. Integer widths and
pointer types remain the ones selected by the established C backend.

The normal native ABI wrappers handle supported fixed parameters and results.
Reference parameters preserve pointer identity, including duplicate scalar,
char and managed-record arguments. Owned string, byte-array and supported record
results use their existing adoption paths. Owned `as val` parameters transfer
to the real callee; the forwarding adapter must not clean up the transferred
record again. Borrowed C string pointers may be read while their source owner is
live; the controls do not establish validity after that owner is replaced or
freed.

Native C bodies retain their original variadic calls. Their generated definitions
include the ellipsis from their declarations. Private C initialization and
callable bodies still link when the executable needs no public Rust ABI wrapper.

This work does not establish arbitrary Rust callback transport through foreign
C function-pointer APIs or indirect variadic callable values. Remaining ABI,
SDK, array and lifetime families stay on the full parity checklist.

`make test-rust-parity-native-variadics` runs six unchanged sources at O0/O1/O2
and default/checked/unchecked arithmetic, then checks complete coverage, frozen
raw output and helper hashes independently. Hosted runtime CI requires all 54
comparisons on Linux, macOS and Windows.

See the [current parity ledger](rust-parity-progress.md) and
[local validation](rust-parity-evidence/native-variadics-validation.json).
