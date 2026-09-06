# Rust native pointer unwrap and owned slices

This dependent slice starts at the reviewed raw-pointer bridge
`643d9d6af8a553352a4789dda6f519f4552d6797` and composes the prospective
byte-backed `SnString` implementation at `a15932511b0bd6fdc9debc54059a8a840be88cd2`.
The raw-pointer branch remains unchanged.

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
ABI representations agree. Source `char` uses a short-lived `c_char` cell and
copies the final byte back into the Rust character after the call. This keeps
the established 0xff character contract while avoiding a four-byte Rust
`char *` at the C boundary.

The unchanged tag-79c20b fixtures `test_pointer_unwrap.sn` and
`test_interop_pointers.sn` are compiled and run through both C and Rust without
source changes. A separate binary fixture returns `A ff B`, a nonempty byte
array, and mutated scalar references to cover data transfer that the unchanged
fixtures' nil paths do not observe.

This slice does not admit managed array arguments, arrays with managed or
non-byte elements, native structs/results, callbacks, or borrowed native
buffers that escape a call. `test_interop_edge_cases.sn` and the broader native
struct, callback, SDK, and buffer/slice families remain required follow-ups.
