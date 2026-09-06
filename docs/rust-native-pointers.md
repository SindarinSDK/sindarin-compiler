# Rust native raw pointer bridge

## Scope

This branch starts from `eea3d09be759ee7945a92f07ff268f497e2f88c4` and
extends only the Rust target's native bridge. Native functions may return and
receive raw pointers whose pointee is another pointer, `void`, an opaque type,
or a scalar already supported by the native bridge. Raw pointers are copied as
addresses. The Rust side does not dereference them, acquire them, release them,
or attach a borrowed lifetime, so ownership remains with the C side.

Nil pointer expressions lower to `std::ptr::null_mut()` and inherit their
pointer type from the call, comparison, declaration, assignment, or return
context. Opaque declarations require no Rust declaration. The ABI still
preserves the tag's representation: an opaque value is `void *`, so a source
`*OpaqueHandle` is `void **` in C and `*mut *mut std::ffi::c_void` in Rust.
Source `*char` uses `*mut std::ffi::c_char`, because the native pointee is one
byte even though a Rust `char` is four bytes. Other scalar pointees use their
existing fixed-width Rust ABI types.

Every Rust wrapper evaluates the native call once, stores the result, flushes
C output, and then returns or converts that stored result. This preserves
source order when C native bodies and Rust functions both print. The generated
flush binding uses the private-name allocator across module symbols and every
function parameter, so a tagged-valid parameter such as
`__sn_native_fflush_0` cannot shadow the generated extern binding.

The former `*int` signature rejection is covered as a compiled and executed
positive. Its negative fixture now records the narrower managed `*str`
boundary. `scalar_pointer_bridge.sn` exercises non-null and null pointers,
same and distinct identities, copied/pass-through identities, pointer results
used directly as arguments, two effectful pointer-result arguments evaluated
once each, primitive, character, void, and opaque pointees, and the tag's
`void **` opaque ABI. Its C and Rust executions match under O0, O1, O2, and
debug builds.

## Tagged evidence

The oracle is v0.0.83 commit
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`, using the verified compiler and
worktree in `/tmp/sindarin-tagged-control-reference.md`. An initial unchanged
six-fixture tagged O0 run is retained at
`/tmp/sindarin-s2-pointer-tag-o0-G1TFHr`.

The required final smoke passed at
`/tmp/sindarin-s2-pointer-final-smoke-gM7u0T`. The strict final matrix is
`/tmp/sindarin-s2-pointer-matrix-bkz108`. It runs the byte-identical tagged
sources `test_interop_opaque.sn`, `test_opaque_types.sn`, and
`test_inline_pointer_passing.sn` under default, checked, and unchecked modes
at O0, O1, and O2. All 27 tag compiles and all 27 Rust compiles returned zero
and produced executables. All 54 executions returned zero. Every execution
matched its unchanged expected output, and all 54 direct stdout/stderr
tag-to-Rust comparisons were byte-identical. Each compile, executable, run,
and comparison status is stored separately. No output, diagnostic, path,
ANSI, or newline normalization was applied.

The three source SHA-256 values recorded by the matrix are:

- `test_interop_opaque.sn`:
  `0cb0084c0221ef7f0e0fb899ae483a368d8289b3a3eb89d236092bd306fb9bab`
- `test_opaque_types.sn`:
  `eab8e72db6e1ff1b2bc7865f33ada0812519ddc1649b0778363639613b64b6b5`
- `test_inline_pointer_passing.sn`:
  `9158155543ff08e270794e0f96c68a7a97e6e681f8a68601eee6d5c2fe4ce1a4`

Generated-source inspection is retained at
`/tmp/sindarin-s2-pointer-generated-JyjWOH`; it records the opaque `void **`
shape and the per-call flush without changing the source. The focused O0 run
that first established corrected mixed C/Rust output order is
`/tmp/sindarin-s2-pointer-flush-kSRbRf`.

The branch was composed with main
`90a0cdd7ca3c493f1110ae0ef7bd2fdf20046d3d` before the parameter-hygiene
correction. The reviewer's byte-identical collision probe is committed as
`native_flush_parameter_collision.sn`. Its generated-source evidence is in
`/tmp/sindarin-s2-pointer-hygiene-generated-VHB1vw`: the source parameter
retains `__sn_native_fflush_0` and the extern/call use
`__sn_native_fflush_1`. The strict C/Rust matrix in
`/tmp/sindarin-s2-pointer-hygiene-matrix-Bvvmye` covers this probe and the
three unchanged tagged raw-pointer fixtures under default, checked, and
unchecked arithmetic at O0, O1, and O2. All 36 paired cases compiled, ran,
matched their oracles, and matched each other without output normalization.

Focused repository validation after the main composition passed with zero
skips: 6 unchanged tagged native C/Rust tests, 12 Rust-native extra tests
(including the pointer and parameter-hygiene ABI matrices at O0/O1/O2/debug),
2 native rejection tests, 1 imported-origin test, and all 12 Rust
toolchain/artifact lifecycle cases. Before this bounded correction, the literal
`make build && make test` gate passed 1608 unit, 107 C-generation, 79
model-generation, 1141 integration, 58 integration-error, 224 exploratory,
and 11 exploratory-error tests.

`make test-rgen` on the required `eea3d09b` parent reported 238 passed, 23
failed, and 0 skipped. The failures are the numeric, resolved-call, and
`as ref` frontend/golden changes that landed after this fixed parent; none is
in a pointer/native fixture or a file changed here. This branch does not
rewrite those independently owned snapshots or frontend cases.

## Required follow-ups

The other measured unchanged tagged files remain owned pointer work, with
their current dependencies explicit:

- `test_interop_edge_cases.sn` first reaches a managed string result and later
  requires native callbacks with pointer parameters. Its SHA-256 is
  `fbae43e68034b6963e35331443f7087202620293d836a34bde0ddc02b04e3541`.
- `test_interop_pointers.sn` requires managed string and array results,
  pointer slicing, pointer unwrapping, and native `as ref` scalar parameters.
  Its SHA-256 is
  `c0e70a78ffbcbc2dfbf87f0aa46e84c8d2c2b472b96a379d4f43650d6eb46526`.
- `test_pointer_unwrap.sn` requires a managed string parameter/result plus
  pointer slicing and array results. Its SHA-256 is
  `10720fa705f88b71933abd9c6b5c67170e627f7b8222ef20384a0d0e5faf68cd`.
- `test_buffer_unwrap.sn`, `test_nil_pointer_slice.sn`, and exploratory
  `test_pointer_slice_bounds.sn` belong to the dependent pointer
  unwrap/slice/buffer slice.

No callback, managed string/array ABI, pointer dereference, pointer slice,
native struct result, or raw-pointer ownership behavior is admitted by this
branch. These are required parity work rather than exceptions.
