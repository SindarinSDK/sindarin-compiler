# Rust native scalar fixtures

These fixtures exercise executable builds with `--target rust`; they must not
be routed through `--emit-rust`, because native programs require a generated C
header/source bundle. `scalar_bridge.sn` covers every admitted scalar ABI and a
Sindarin body rendered as C. `imported_alias.sn` covers imported source origins
and a Rust callable whose name differs from its external C symbol. The error
fixtures pin the first-slice bool and closure boundaries.

The unified test runner compiles each positive fixture with both the default C
backend and `--target rust`, executes both binaries under a timeout, and
requires byte-for-byte equivalent output. Error fixtures pin Rust validation
at the deliberately bounded scalar ABI boundary.
