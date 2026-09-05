# Thread ownership successor

This dependent branch starts at PR127's integration commit
`be6dca14d3055314986965f140cd6052bbbbe17b` (main
`80c689c2b18b9506c987cfc639ad60ac501756ca` merged once into PR127).
Dependency merge `5f945bf9` incorporates the supplied owned-array checkpoint
`7d7786e1390d8bd8ccdd67326614aa9a71a99a4c`, including closure successor
`1270495d3802da44b0d09b041f000141906584aa`. Root owns integration of those
separate PRs. Capture classification and owned-array mutation lowering are reused,
not reimplemented.

## Implemented private contracts

Thread-bearing models use Arc-owned callable identity, Send+Sync trait objects,
and Arc-owned capture cells backed by a mutex. Non-thread models retain the
existing Rc/Cell/RefCell output. The bounds express the private representation;
there is no frontend Send/Sync restriction or unsafe Send implementation.
Recursive callable slots use OnceLock and Weak to retain the existing weak-cycle
scheme safely across threads.

Captures still follow the dependency's value-versus-shared classification.
Shared owners are cloned by identity; their contents are not copied to make a
thread argument. Reads clone the existing rvalue under a lock and release the
guard before evaluating the next operand. Assignment, replacement and mutation
update the same shared owner. The capture API is `get`, `set`, `replace`,
`borrow_mut`, plus the storage lock and explicit synchronization gate. Existing
owned array/string rvalue copying remains the dependency's behavior.

The caller captures a closure callee before its arguments, preserving its own
handle for later calls. Rebound function arguments no longer retain the
named-function conversion marker. The focused tests exercise a closure whose
creator has returned, recursive calls, caller-visible scalar and array changes,
and reference aliasing.

Scalar reference functions reached by a thread use shared owner parameters.
A fixed-point private pass propagates the representation through direct calls,
including ordinary calls and forwarding. Two parameters may reference the same
owner; no aliased `&mut` references or mutex guards are sent to a worker.
Promotion uses lexical variable declarations. Aggregate/global places and other
unimplemented reference signatures remain required backend work and are rejected
rather than copied. Local synchronized storage retains its separate lock gate.

Handle arrays have ordinary value storage and a private vector of shared pending-join slots.
Push, indexed sync (including index normalization), whole-array sync, empty arrays
and reassignment preserve the tagged fixture behavior. Struct results use the
existing zero-value projection. Dropping discarded array handles detaches rather
than introducing an implicit join. Local declaration/assignment aliases share pending join identity, including
intermediate aliases and resetting the original before synchronizing the copy.
The join-slot mutex is released before waiting. Function/closure transfer,
repeated synchronization through different aliases, and arbitrary mutations of
pending handle arrays remain required, unestablished cases.

A thread-only explicit operand type resolves Rust's ambiguous literal receiver
for checked operations. The integrated signed diagnostic helpers are retained
exactly. The tagged worker division-by-zero fixture now matches both output
streams and exit status 1. Other worker failures, bounds failures and assert
handling are not thereby declared complete.

The remaining globals case `using` renders the tagged explicit dispose call after
its body. Early return skips that call, exactly as the tagged template does;
ordinary Rust field cleanup handles owned storage. No new RAII dispose-on-return
semantics were introduced.

## Evidence

All fresh C controls use `/tmp/sindarin-tagged-control-reference.json` and the
specified cwd/compiler at `review-rust-diagnostic-tag79c20b`, tag
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`. Before every new tagged harness,
`test_int_negative.sn` is freshly compiled and executed and its stdout compared
with the unchanged expected file. Failure is HARNESSISSUE. The tag command selects
C by default, uses `--no-install -O0`, and never receives `--target`.

`scripts/test_rust_thread_ownership.py` gives each case/compiler fresh temporary
and output directories, captures stdout/stderr/status separately, checks the
executable before running, and compares fresh output streams without normalization.
The Rust leg sets SN_CC to a nonexistent compiler. The harness is Spark1-specific
because its control reference is explicitly pinned by the user.

At **O0 only**, `using/results.json` has 18/18 raw differential matches: nine
unchanged tagged sources and nine new deterministic probes, including one runtime
failure with exit status 1. Positive tagged fixture stdout expectations also
match unchanged. The smoke is separate and is not counted as feature parity.
There are no sleeps in the new positive probes.

The earlier Rust-only inventory (`inventory-review`) compiled 53/72 catalog
sources: 52 exited 0 and the worker-failure fixture exited 1. It is **not parity
evidence**. The subsequently fixed `using` case has separate fresh differential
evidence. `remaining.json` records the remaining 18 compilation blockers and their
observed diagnostics; this is not a new full-corpus or all-mode measurement.
Numeric-array formatting and additional unmeasured lifetime/alias cases remain.

Existing suites after the dependency integration and implementation retain 237
passing generation cases and 16 known failures; negatives have 163 passes,
8 known failures and five explicitly promoted cases from the closure dependency.
The closure-value suites pass 23 positive and one negative case; toolchain passes
6/6; compiler unit binary passes 1608/1608. Final rerun logs and binary identities
are recorded in verification.json. Source C/frontend/runtime/templates are
unchanged from PR127. The dependency's Makefile change only adds a Rust closure
suite target; no C recipe or configuration changes.

Raw evidence remains under `/tmp/sindarin-rust-thread-ownership-evidence`.
The two initial new probes in `tests/rust-thread-ownership/observations` are
preserved but not counted as passed or defined-behavior oracles: duplicate mutable
closure aliases printed matching values then tagged C aborted during cleanup;
a capture referenced only inside a lock produced an undeclared capture in tagged
C. The latter is not repaired in the frontend. Both had passing fresh controls.
An initial Rust capture-helper rebuild used a stale newly-added template because
CMake's configured glob omitted the new dependency; project reconfiguration fixed
staging, and source/staged Rust templates were compared. Initial harness launch
syntax errors were preserved separately and produced no feature interpretation.

## Remaining ownership coordination

Root has been asked to obtain sn-s2-mutable's stable struct-capture owner interface.
Receiver and aggregate-reference lowering must carry that owner across spawn,
with field projections; holding a receiver mutex through a method that joins a
worker can deadlock, and copying a receiver loses alias-visible writes. This
branch does not duplicate the struct-capture implementation. The remaining native,
assert/diagnostic and char/numeric cases belong to the corresponding coordinated
lanes. Full tagged-language concurrency parity remains required.

## Pending-array alias follow-up

The fresh `array-alias-before` probe established a concrete gap: tagged C
returned 42 through an array copy while Rust returned its zero placeholder.
`array-alias-final` now records five raw O0 differential matches: two unchanged
tagged array fixtures, the existing shared-capture array probe, and two new
copy/assignment/reset probes. The mandatory fresh smoke passed first. This is
a focused extension; the earlier 18-case and suite evidence retains its own
compiler identity and is not relabeled as a full rerun. Exact fresh provenance
and results are in `array-alias-verification.json`. The Rust leg again used a
nonexistent C compiler. No C source, fixture, harness or oracle was changed.
