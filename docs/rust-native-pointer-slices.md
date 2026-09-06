# Rust native pointer unwrap and owned slices

This dependent slice starts at the reviewed raw-pointer bridge
`643d9d6af8a553352a4789dda6f519f4552d6797`. Its current composition base is
main `9598f10e2a6e63031b06c404a8c1e352715b3e7a`, which contains the final
byte-backed `SnString` implementation from `915986e16f14819c436942bbbac6bdaf71110020`.
The preserved raw-pointer implementation commit remains unchanged.

Native bodies still compile as C. The Rust wrapper now admits the tagged forms
needed to observe their results: owned `str` results, owned `byte[]` results,
borrowed `str` arguments, and scalar `as ref` arguments. A string argument is
held in a temporary NUL-terminated byte vector for the full C call. A returned
C string is copied directly into `SnString(Vec<u8>)` and then freed, without
UTF-8 decoding or replacement. A returned `SnArray<byte>` is layout-checked,
copied into `Vec<u8>`, and its C data and header allocations are freed exactly
once. Generated native-body pointer slices therefore retain tagged behavior:
they copy the requested byte range, and a nil pointer with a positive range is
zero-filled by `sn_array_from_ptr` before crossing the ABI.

The scalar reference bridge passes Rust scalar places directly where their C
ABI representations agree. Source `char` uses short-lived `c_char` storage
keyed by source-place address and copies the final byte back after the call.
Two arguments that name the same character therefore give C the same address
and observe each other's writes. This keeps the established 0xff character
contract while avoiding a four-byte Rust `char *` at the C boundary.

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

The main composition smoke is `/tmp/sindarin-s2-tag-smoke-dm13OQ`. The strict
four-review-fixture composition matrix is
`/tmp/sindarin-s2-pr143-main9598-matrix-NRnYRi`: all 36 tag and 36 Rust
compiles and all 72 runs pass under default, checked, and unchecked modes at
O0, O1, and O2. Expected output, backend stdout, and backend stderr compare
exactly without normalization. The four source hashes remain the values
recorded in `rust-native-pointers.md`.

Focused composition gates pass with no skips: 8 tagged native C/Rust fixtures,
17 native extra fixtures (including O0/O1/O2/debug managed transfer and the
four review corrections), 4 precise native rejection fixtures, 1
imported-origin fixture, all 12 Rust toolchain and artifact lifecycle cases,
36 closure positives plus 1 closure rejection, and 63 compiled raw-byte string
executions. Main supplies 6 tagged and 12 extra fixtures; this slice adds two
tagged pointer programs and five native fixtures. When the strict platform
workflow from PR 144 is composed, its required fixture counts must therefore
be updated deliberately from 6/12 to 8/17 rather than bypassed or weakened.

This slice does not admit managed array arguments, arrays with managed or
non-byte elements, native structs/results, callbacks, or borrowed native
buffers that escape a call. `test_interop_edge_cases.sn` and the broader native
buffer/slice probes remain measured in
`/tmp/sindarin-s2-pointer-slice-deferred-XPNBXS`; they fail at those precise
boundaries and receive no parity credit.
Native struct, callback, SDK, and buffer/slice families remain required
follow-ups.
