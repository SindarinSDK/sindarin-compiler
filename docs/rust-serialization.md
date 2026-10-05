# Serialization in the Rust backend

Rust emits the generated `@serializable` methods and calls the existing C
Encoder/Decoder vtables through typed wrappers. Native factories and concrete
format implementations retain their C interfaces. C remains the default target.

The Rust projection supplies the built-in opaque Encoder and Decoder types,
which have no source struct declaration or C reference-count field. Their Rust
owners share one C allocation, release it once and keep known child contexts'
parents alive. Generated Encoder `end` calls invalidate the Rust owner because
the concrete callback can free the child allocation. `result` uses the C runtime's
existing finalization cache, preserving repeated-result behavior. No Send claim
is made for these opaque serialization handles.

Generated traversal preserves source field order, aliases, scalar/string
operations, nested serializable records and supported scalar/string/record
arrays. Decode locals use private names so source fields such as `d` cannot
shadow the decoder. Threaded record fields use their lowered storage wrappers;
serialization reads values before calling native code. Mixed programs preserve
ordinary native-handle arrays without inventing C retain callbacks for Encoder
or Decoder. Serialized records retain the previous native wire-ABI exclusion.

`make test-rust-parity-serialization` requires 13 unchanged sources at O0/O1/O2
and default/checked/unchecked arithmetic. Its 117 comparisons independently
check frozen sources, C helpers, output bytes, statuses and empty runtime stderr.
The gate is mandatory on Linux, macOS and Windows. Eight sources are existing
corpus programs; five controls cover vtable operations, generated names,
primitive arrays, nil handles, cleanup counts, ordinary returns and borrows,
mixed native arrays and joined record threads.

All 234 instrumented target executions pass. The actual 603 C object compile
commands include ASAN/UBSAN; every object has ASAN sites and 585 have applicable
UBSAN sites. Each Rust execution is associated with its exact generated C
objects. This validates the instrumented C boundary; Rust memory instrumentation
is not claimed.

Full parity still requires foreign callbacks, broader SDK and serialized-record/
array transport, serialization handles crossing threads, implicit global cleanup,
C-side disposal/alias ownership and additional lifetime compositions. The local
checks do not establish these families. Full exact-revision hosted acceptance
also remains required.

See the [parity ledger](rust-parity-progress.md) and
[validation evidence](rust-parity-evidence/serialization-validation.json).
