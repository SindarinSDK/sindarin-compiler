# Rust native pointer unwrap and owned slices

This dependent slice starts at the reviewed raw-pointer bridge
`643d9d6af8a553352a4789dda6f519f4552d6797` and composes the prospective
byte-backed `SnString` implementation at `a15932511b0bd6fdc9debc54059a8a840be88cd2`.
It then incorporates the reviewed parameter-hygiene correction at
`47094f6a2bb16a79a0905ab343a540a1d797d11e`. The preserved raw-pointer
implementation commit remains unchanged.

Native bodies still compile as C. The Rust wrapper now admits the tagged forms
needed to observe their results: owned `str` results, owned `byte[]` results,
borrowed `str` arguments, and scalar `as ref` arguments. A string argument is
copied into a NUL-terminated allocation retained until the calling thread exits.
This preserves native code retaining the passed pointer across later calls, as
observed by `native_string_escape.sn`; cross-thread escape and shutdown lifetimes
still require broader validation. A returned
C string is copied directly into `SnString(Vec<u8>)` and then freed, without
UTF-8 decoding or replacement. A returned `SnArray<byte>` is layout-checked,
copied into `Vec<u8>`, and its C data and header allocations are freed exactly
once. Generated native-body pointer slices therefore retain tagged behavior:
they copy the requested byte range, and a nil pointer with a positive range is
zero-filled by `sn_array_from_ptr` before crossing the ABI.

The scalar reference bridge passes Rust scalar places directly where their C
ABI representations agree. Source `char` uses a short-lived `c_char` cell and
copies the final byte back into the Rust character after the call. This keeps
the established 0xff character contract while avoiding a four-byte Rust
`char *` at the C boundary.

The unchanged tag-79c20b fixtures `test_pointer_unwrap.sn` and
`test_interop_pointers.sn` are compiled and run through both C and Rust without
source changes. A separate binary fixture returns `A ff B`, a nonempty byte
array, and mutated scalar references to cover data transfer that the unchanged
fixtures' nil paths do not observe.

The final tagged-control smoke is `/tmp/sindarin-s2-tag-smoke-uvRbS9`.
The unchanged two-fixture matrix is
`/tmp/sindarin-s2-pointer-slice-matrix-73mL4Z`: both source hashes match the
tag checkout and all 18 C plus 18 Rust compile/run cases pass under default,
checked, and unchecked modes at O0, O1, and O2, with exact expected output and
direct backend comparisons. The binary managed-transfer matrix is
`/tmp/sindarin-s2-managed-pointer-matrix-bAdRQ5`; its nine mode/optimizer pairs
also pass exactly, including the raw `0xff` string byte. The committed source,
C sidecar, and binary oracle SHA-256 values are respectively
`352dfb3bc4029894311b1a215c18a0147d894ab984e0574a89a47c37d03bb5c7`,
`f528ad1b60340013047cd34128ac8345b626bb298c43dbcbc07c045b397a3845`,
and `cc698c3968653530157c0e73547410ceb8e01098af2e7fc13901d82cef6d163a`.

Focused repository gates pass with no skips: 8 tagged native C/Rust cases,
17 native extra cases (including O0/O1/O2/debug managed transfer), 4 precise
native rejection cases, 1 imported-origin case, and all 12 Rust toolchain and
artifact lifecycle cases.

This slice does not admit managed array arguments, arrays with managed or
non-byte elements, native structs/results, callbacks, or general borrowed native
buffers and their ownership/lifetime rules. `test_interop_edge_cases.sn` and the broader native
buffer/slice probes remain measured in
`/tmp/sindarin-s2-pointer-slice-deferred-XPNBXS`; they fail at those precise
boundaries and receive no parity credit.
Native struct, callback, SDK, and buffer/slice families remain required
follow-ups.

## Current integration verification

The integration composes this original PR onto verified main `432983b3`.
Existing Rust generation snapshots are unchanged. The native-receiver negative
still rejects native structs; its message now accurately lists the remaining
closure, thread and struct boundaries after array expressions were admitted.
It receives no parity credit.

`make test-rust-parity-native-managed` compares seven unchanged sources at
O0/O1/O2 in default, checked and unchecked arithmetic modes: 63 pairs with
compiler/source hashes, commands, statuses and raw stdout/stderr in
`.sn/rust-parity-native-managed.json`. The binary managed fixture has separate
LF and Windows CRLF oracles. Both targets must satisfy the platform oracle and
agree byte for byte; no binary output normalization is performed.

The eight tagged and seventeen extra fixtures are required by hosted runtime
CI, alongside the imported-origin fixture and the complete Rust suite. Final
hosted evidence will be recorded after this composition is refreshed against
the receiver changes in PR134. Historical temporary paths above describe the
original slice and are not substitutes for current integration evidence.
