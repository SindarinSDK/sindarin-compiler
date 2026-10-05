# Byte-pointer slices in the Rust backend

Ordinary Rust functions now evaluate byte-pointer slices and no-op `valueOf`
expressions. The generated helper copies byte offsets into owned Rust storage.
It follows `sn_array_from_ptr`: a reversed or empty range produces an empty
array, a positive range from a nil pointer is zero-filled, and negative offsets
are relative to the pointer without array-length clamping. Non-null accesses
must remain inside the source allocation. The result survives mutation and
disposal of that allocation. Empty and nil-source results retain non-null array
identity where the source program observes it.

Bounds convert to the C runtime's long-long offset type. A generated typed C
adapter promotes char bounds using the actual C compiler's char signedness,
including explicit `-fsigned-char` and `-funsigned-char`. Runtime subtraction
wraps independently of the source arithmetic mode. Helper names avoid source
symbols. Controls cover globals, ordinary returns, record fields and joined
workers that create their native pointer within the worker.

Every pointer and bound expression is evaluated once. C leaves independent
function argument order unspecified; the controls check individual call counts
without declaring a portable C evaluation order. Invalid memory ranges and
allocation-failure behavior are not established by these controls.

`make test-rust-parity-pointer-slices` compares five frozen sources across
O0/O1/O2 and default/checked/unchecked arithmetic, then independently checks all
45 source identities, output bytes, helper hashes and execution statuses. Three
sources are unchanged original corpus programs. The gate is mandatory on
Linux, macOS and Windows. On the published parent, all 45 C cases pass and all
45 Rust cases fail compilation. Full local C/Rust suites and all previous
42-report/3639-case gates pass with this implementation.

All 90 sanitizer executions satisfy their frozen oracles. Actual compilation
commands instrument 117 C objects with ASAN/UBSAN; all have ASAN sites and 81
have applicable UBSAN sites. Rust execution logs identify their exact generated
C objects; the pure Rust nil-pointer original has none. This checks the C
boundary. The generated Rust pointer-copy code is not sanitizer-instrumented.

Other pointer element types remain required parity work. The existing C
runtime creates byte-width, byte-tagged storage even for wider or managed source
element types. Replacing that storage with a typed Rust vector would change
observable behavior. Separate C-valid char/bool/int/float/double probes still
fail Rust compilation and receive no parity credit. Foreign callbacks, SDK
array/record transport and broader native ownership/lifetime compositions also
remain on the full goal.

See the [current ledger](rust-parity-progress.md) and
[validation evidence](rust-parity-evidence/pointer-slices-validation.json).
