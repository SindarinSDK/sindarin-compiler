# Array value parameters in the Rust backend

Ordinary functions and instance/static methods now accept array parameters
qualified `as val`. Both targets make an independent copy at callable entry,
after argument evaluation. Scalar buffers copy their elements; strings, nested
arrays and owned value records use their existing deep-copy operations. Reference
records and function elements retain their identity while the array buffer is
independent. Returning the local copy transfers its ownership.

This also repairs the C backend's parameter-sharing bug: the unchanged original
`test_as_val_array_copy.sn` now leaves the caller's `[1, 2, 3]` unchanged. The
old C output mutated that array despite the documented `as val` contract, so it
is not the oracle. Ordinary C closure-array slots now retain copied/borrowed
handles and release replaced or destroyed slots. Comparisons clean up owned
array results, including comparisons with two temporary operands.

Rust coalesces duplicate incoming borrows while making a distinct copy for each
value parameter. Mixed default/value parameters continue to share the original
through the default parameter. Methods participate in the existing thread-owner
transport graph. Borrowed field inputs remain owned by their source, and a value
parameter is never replaced with an alias to the receiver's field. Timing
controls exercise scalar argument effects before entry copies for ordinary,
static and instance calls, including a threaded array parameter receiving a
record field. Nested array push evaluates its value before its indexed receiver,
matching the defined C macro order.

`make test-rust-parity-array-values` compares five frozen sources at O0/O1/O2
with default, checked and unchecked arithmetic. All 45 comparisons independently
check source identity, expected raw output, successful compilation/execution and
empty runtime stderr. Two sources are unchanged original corpus programs; three
new controls cover element types, copying, duplicate and mixed inputs, returns,
forwarding, nil/empty identity, methods, fields, joined workers, escaping value
snapshot captures and argument timing. The platform workflow requires the gate
on Linux, macOS and Windows.

All 90 sanitizer-configured executions pass. An actual command/object audit
confirms ASAN/UBSAN instrumentation in all 45 generated C main objects. The C
runtime archive is prebuilt and recorded without claiming its instrumentation.
The 45 Rust executions are pure Rust; their generated code is not instrumented.
These checks do not establish sanitizer coverage of all language memory behavior.

Full parity still requires mutation of ordinary closure array parameters,
qualified closure parameter compositions, broader effectful/indexed alias
combinations, heap-owning record reference parameters, SDK/native array
transport, foreign callbacks, native disposal and global/thread lifetime
composition. Existing value-capture snapshots in the C backend are preserved;
this increment does not establish a general closure mutation/capture contract.

See the [completion ledger](rust-parity-progress.md) and
[validation evidence](rust-parity-evidence/array-values-validation.json).
