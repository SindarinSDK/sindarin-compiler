# Rust native scalar fixtures

These fixtures exercise executable builds with `--target rust`; they must not
be routed through `--emit-rust`, because native programs require a generated C
header/source bundle. `scalar_bridge.sn` covers every admitted scalar ABI and a
Sindarin body rendered as C. Its duplicate source spelling and distinct
same-basename source objects depend on post-tag sidecar behavior and are
classified as Rust-only extra coverage, not tagged C/Rust parity evidence.
`scalar_helper_dependency.sn` proves that a native body retains complete,
transitive ordinary-function dependencies in its private C projection.
`scalar_helper_struct_dependency.sn` pins a helper's value-struct definition;
`scalar_helper_type_dependency.sn` extends that closure through nested value
struct definitions and a helper global's transitive initializer and lifecycle.
`scalar_bool_char.sn` verifies C `_Bool` normalization and byte-preserving C
`char` calls/results through both backends at O0, O1, O2, and debug O0.
`scalar_tagged_char_control.sn` is the separately classified source-equivalent
extraction of tag-`79c20b`'s `native_identity_char`; the unchanged full tagged
fixture remains outside the parity denominator because unrelated ordinary Rust
constructs are not supported yet.
`imported_alias.sn` likewise covers post-tag imported source origins and a Rust
callable whose name differs from its external C symbol. The error fixtures pin
the remaining pointer and closure boundaries.

Tagged parity is measured separately by `rust-native-tagged`, which compiles
and executes the unchanged tag-`79c20b` fixtures
`tests/cgen/pragma_source.sn`, `tests/integration/test_native_math.sn`, and
`tests/integration/test_native_with_body.sn` through both C and Rust. The
toolchain suite additionally compiles and runs the unchanged native-body case
with `-g`, pinning configured sanitizer-runtime linkage through the C driver.
post-tag `scalar_bridge.sn` and `imported_alias.sn` suites require their Rust
outputs to match explicit oracles; these cases remain outside the tagged parity
denominator. Error fixtures pin Rust validation at the deliberately bounded
scalar ABI boundary.

The next native ABI slices still need explicit representation and lifecycle
work for composite values (including native structs and managed values), raw
pointers, callbacks/closures, and package/SDK entry points. None of those
families is admitted by the bool/char scalar slice.
