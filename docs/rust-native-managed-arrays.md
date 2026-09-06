# Rust native managed primitive arrays

This dependent Rust-only slice now starts at the current-main PR 143
composition `0b14d9455cc6935bf31e7feba5bf5dbd224a96d5`, whose base is main
`9598f10e2a6e63031b06c404a8c1e352715b3e7a`. It therefore retains the native
pointer identity, character alias, string lifetime, and helper-hygiene fixes
while reusing main's byte-backed `SnString`. It adds no second string
representation and performs no text conversion.

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

`tests/rust-native/native_array_same_place.sn` preserves the independent
reviewer's source and sidecar byte-for-byte. It passes one `int[]` place twice
to a native function. Rust evaluates both argument places in source order,
uses raw pointers only to identify the source places, and redirects both C
arguments to one staged `SnArray`. The first occurrence performs the single
copy-back. This preserves C address identity and mutations without constructing
overlapping Rust mutable references. Distinct source arrays retain independent
staging and ordered copy-back.

The review correction is verified against the pinned tag after smoke
`/tmp/sindarin-s2-tag-smoke-gp25ev`. The unchanged review source and the
distinct-place control produce their committed output at O0/O1/O2 in
`/tmp/sindarin-s2-tag-managed-identity-okY8tL` (six tag pairs). The corrected
Rust review source also passes O0/O1/O2 in
`/tmp/sindarin-s2-managed-alias-fix-RnlCry`; the focused native suite exercises
both fixtures in its full ABI matrix.

The authoritative tag control is peeled `v0.0.83` commit
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`. The composition control smoke is
`/tmp/sindarin-s2-tag-smoke-Ute943`. The strict differential matrix is
`/tmp/sindarin-s2-managed-array-main9598-matrix-Ce0RZs`: all nine
default/checked/unchecked by O0/O1/O2 pairs compile and run on both the tagged
C compiler and this Rust branch, match the committed oracle, and match each
other byte-for-byte. The source, sidecar, and output hashes are recorded inside
that directory. An exploratory native parameter-rebinding source is preserved at
`/tmp/sindarin-s2-managed-array-oracle-NeCNew`: it compiles under the tag but
the tagged executable exits through `SIGSEGV` after generated C frees the
caller's array handle and leaves it dangling. It is neither admitted as defined
parity nor treated as a Rust exception.

The retained PR 143 behavior was rechecked after the template composition in
`/tmp/sindarin-s2-managed-pr143-matrix-FsrGmw`: all 36 tag/Rust pairs and all
72 runs pass with exact stdout and stderr across the same modes. Focused gates
pass with no skips: 8 tagged native fixtures, 21 native extra fixtures, 4
native diagnostics, 1 imported-origin fixture, all 12 Rust toolchain and
artifact-lifecycle cases, 36 promoted closure cases plus the closure
diagnostic, and 63 compiled raw-byte string executions. Composing PR 144's
strict fixture-count workflow on this dependent slice requires counts of 8
tagged and 21 extra fixtures.

Managed string/nested/struct/function arrays, native structs and struct
results, callbacks, SDK packages that require those representations, and
escaping borrowed buffers remain required native parity work.
