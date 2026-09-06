# Rust native reference-struct results

This dependent Rust-only slice starts at managed-array correction
`9bfe534a4b3906cc198b767ccbfed609537ce938`, which retains the reviewed native
pointer/string bridge and its same-place array identity correction. It does not
change the C target, shared frontend, runtime headers, tagged fixtures, or their
oracles.

The admitted native aggregate contract is a `native struct ... as ref` whose
fields use C-compatible scalar layouts other than `char`, and whose methods are
aliased native instance methods with no explicit parameters and scalar or void
results. A native function may return such a struct as an owned result or take
it as a by-value handle argument. Native methods receive the same allocation
pointer.

Rust emits a private `repr(C)` layout containing the tagged C reference count
followed by the declared scalar fields. The source-visible handle owns a raw
pointer to that layout. Clone increments the embedded count; drop decrements it,
invokes the declared dispose alias at zero, and releases the C allocation with
`free`. Dereference borrows the allocation only for the handle's lifetime, and
`PartialEq` compares allocation identity. Native function results enter through
`from_owned`; borrowed handle arguments are cloned before the wrapper takes
ownership, while owned temporaries transfer their existing credit. This lets
ordinary Rust value structs and arrays retain handles using their normal
`Clone` and `Drop` behavior.

The original tagged sources remain byte-for-byte unchanged. Their SHA-256
values and the pinned v0.0.83 O0/O1/O2 compile/run streams are in
`/tmp/sindarin-s2-tag-aggregate-o60c3z` (15 successful controls across the five
measured sources). Four share the implemented contract:

- `test_refcount_chain_in_struct_literal.sn`
- `test_struct_return_array_leak.sn`
- `test_struct_rvalue_member_leak.sn`
- `test_struct_rvalue_member_leak_contexts.sn`

The Rust O0/O1/O2 executions for those four are in
`/tmp/sindarin-s2-rust-native-ref-originals-8M8uN9`. All 12 compile and run
successfully, match their committed oracle, and match the corresponding fresh
tag output byte-for-byte. The focused `scalar_native_ref_struct_result.sn`
checks clone identity through direct handles, value-struct fields, normal
arrays, native arguments, a native method, and final zero-live-count disposal.
Its pinned tag O0/O1/O2 evidence is
`/tmp/sindarin-s2-tag-native-ref-focused-WnlTc3`, preceded by smoke
`/tmp/sindarin-s2-tag-smoke-qEEbRn`; Rust O0 evidence is
`/tmp/sindarin-s2-native-ref-focused-vuz3n6`. Debug/ASAN runs of the focused
control and all four unchanged sources pass with empty sanitizer streams in
`/tmp/sindarin-s2-native-ref-asan-2qKl2j`.

`test_array_literal_arg_leak.sn` additionally requires a native managed array
whose elements are reference-struct handles. That ABI must retain each staged
pointer, preserve array ownership callbacks, and release/copy back without
inventing ownership for raw pointers. It remains a required dependent slice;
its precise current boundary is preserved in
`/tmp/sindarin-s2-native-ref-deferred-WWosFK`. Same-type native reference
results that may alias an input remain fail-closed through the existing
`borrow_inferred_call` diagnostic until the wrapper can make the tagged retain
decision. Native char fields, non-scalar fields, parameterized native methods,
native value/packed structs, callbacks, struct-by-value ABI, and cross-thread
handles also remain required work.

The focused repository gates contain 12 unchanged tagged native fixtures, 23
native extras, 7 native diagnostics, and 1 imported-origin fixture. Composing
the strict native fixture-count workflow requires those exact counts. The only
call-validation change is the declaration-level admission for the annotated
native reference method family; resolved native method-call forms remain
rejected and are not claimed by this slice.

## Disposal output ordering correction

The independent `native_ref_dispose_order.sn` probe is preserved byte-for-byte
at SHA-256
`f4a3a6d1453431a8fab32a4836293ae1da594bb510dea512958eca9a2c4075ce`;
its sidecar is
`909135975fab2378d69310ad78b7fd6d7433998643ef13caba1dac04f6f9921c`.
Rust already dropped the last handle at the tagged lexical endpoint, but the C
`dispose` callback's `printf` remained buffered while Rust writes went directly
to stdout. The zero-count drop path now flushes C streams immediately after the
dispose callback. Fresh pinned-tag O0/O1/O2 evidence is in
`/tmp/sindarin-s2-tag-dispose-order-KIA94u`, preceded by the required smoke in
`/tmp/sindarin-s2-tag-smoke-Ypkmeb`. The committed C/Rust O0/O1/O2 and Rust
debug matrix expects `inside`, `dispose`, then `after` without changing the
source or oracle order.
