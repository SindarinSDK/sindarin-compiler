# Rust native managed primitive arrays

This dependent Rust-only slice starts at the reviewed native pointer and
managed byte transport head `b3a3cd1b3c40ae2d9c7efcb42a665876d30fd0b2`
and includes its whitespace correction at
`788d7b840b6bfff9e262c82955afb5c5ca407b67`. It reuses the byte-backed
`SnString` representation already composed into that parent; it adds no second
string representation and performs no text conversion.

The native bridge accepts default and `as val` primitive array parameters and
owned primitive array results for `int`, `long`, `int32`, `uint`, `uint32`,
`byte`, `bool`, `char`, `float`, and `double`. The tagged compiler passes native
array parameters as shared `SnArray *` handles even when the declaration spells
`as val`, so both forms preserve caller-visible element and length mutation.
Rust projects both forms as mutable array borrows. The call lowering retains
its existing place stabilization and left-to-right argument evaluation.

The wrapper copies a Rust array into storage obtained from the C `malloc`
contract before entering native code. Native `push` and other operations can
therefore use C `realloc` without touching Rust allocator storage. After the
call, the wrapper validates the scalar element layout, copies the resulting
length and elements back to the caller's `Vec`, and frees the temporary C
buffer. `char` arrays explicitly convert between Sindarin's byte-valued Rust
`char` representation and C `char`. Owned array results are copied into a new
Rust `Vec`, then their C data and header allocations are freed once. Raw pointer
parameters and results retain the borrowed/opaque behavior from the parent and
do not enter this ownership path.

`tests/rust-native/scalar_primitive_array_bridge.sn` and its C sidecar cover all
ten primitive element types, caller-visible element and length growth, the
tagged native `as val` behavior, empty arguments/results, owned results, field
and indexed places, and once-only source-order evaluation. The hygiene fixture
forces source collisions with the projected array type, `malloc`, `free`, and
array-argument temporary names.

The authoritative tag control is peeled `v0.0.83` commit
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`. The final control smoke is
`/tmp/sindarin-s2-tag-smoke-03DB0W`. The strict differential matrix is
`/tmp/sindarin-s2-managed-array-final-matrix-wzu9jE`: all nine
default/checked/unchecked by O0/O1/O2 pairs compile and run on both the tagged
C compiler and this Rust branch, match the committed oracle, and match each
other byte-for-byte. The source, sidecar, and output hashes are recorded inside
that directory. An exploratory native parameter-rebinding source is preserved at
`/tmp/sindarin-s2-managed-array-oracle-NeCNew`: it compiles under the tag but
the tagged executable exits through `SIGSEGV` after generated C frees the
caller's array handle and leaves it dangling. It is neither admitted as defined
parity nor treated as a Rust exception.

Focused final gates pass with no skips: 8 tagged native cases, 15 native extra
cases, 4 native diagnostics, 1 imported-origin case, all 12 Rust toolchain and
artifact-lifecycle cases, 36 promoted closure cases, and the closure diagnostic
case.

Managed string/nested/struct/function arrays, native structs and struct
results, callbacks, SDK packages that require those representations, and
escaping borrowed buffers remain required native parity work.
