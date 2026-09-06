# Rust concurrency foundation

Base: restored main `fcdb872925fac6469d1a5a19c9b4d2de45e5ebc4`.
Oracle: v0.0.83, peeled `79c20bdb8314aff3c778471ceab20bb8f9ca8d62`, annotated
`a896688aefaf68a0eb502b9baeebe8b0d6185e37`. This is a partial backend foundation,
not concurrency parity or permission to change the tagged language.

## Private interface and tagged contract

`rust_concurrency.c` runs last, after private closure/call/arithmetic lowering.
It annotates the private JSON projection only. A collision-free prefix names
runtime types, global storage, operand temporaries and companion handles.

* A direct-call spawn evaluates arguments on the caller, captures owned values,
  then starts `std::thread`. Arguments retain private call-site metadata.
* A pending named result has its tagged zero value and a separate optional join
  handle. Joining replaces the result. Repeated joining and a conditional branch
  that never spawned retain the current result. Sync lists join in source order.
* Ordinary scope exit joins outstanding named handles. Explicit detach and
  discarded spawn statements detach. Replacing a companion does not add a join;
  the shared tagged checker still rejects reassignment of a known pending handle.
* Globals use hygienically named `LazyLock<Cell<T>>` storage, forced in declaration
  order before the source main body. A cell has a value mutex and a separate lock
  gate, reflecting the tagged atomic storage / explicit mutex distinction.
  Reads release the value guard before returning. Scalar assignments, string
  append, direct array mutations, indexed assignments and direct field assignments
  update storage. Lock guards unwind on ordinary return, break and continue.
* Synchronized postfix operations hold the gate while mutating, release it, then
  read the result, as the tagged template does. This does not establish arithmetic
  parity for every numeric type or mode.

The support code uses Rust's checked thread-safety bounds; there is no unsafe
Send/Sync implementation, C bridge, or new source-language restriction. Programs
requiring an unimplemented representation can still fail backend generation or
rustc. Those failures are required work, never counted as parity.

## Ownership dependencies / required remaining work

The current base closure handle contains `Rc<dyn Fn>`. Threading reference
arguments or closures requires a coordinated shared owner/capture representation
with the mutable/owned closure lane. Copying an `as ref` argument into a worker
would lose visible writes; this foundation does not claim that support. Capturing
borrowed `self` also requires lifetime/receiver lowering rather than moving a
borrow into an unscoped worker. The generated Join and Cell support lives in new
private partials; closure support is unchanged.

Arrays containing thread handles still need private slot/companion lowering;
ordinary arrays returned by an individual thread work. Heap-bearing/reference
struct parameter forms remain dependent on owned/reference lowering. Native
source/include forms belong to the native lane. `assert` and worker failure
messages/exit behavior depend on diagnostics lowering; current Rust panics are
not evidence of the tagged worker-error contract. Char mutations and nested
numeric-array formatting remain numeric-lane dependencies. Nested mutable
aggregate places, alias visibility and all scheduling/interleaving behavior are
not established by the focused execution results.

## Verification scope

Evidence is preserved on spark1 at `/tmp/sindarin-rust-concurrency-evidence`.
`comparison-final.json` records all 74 paths, source hashes, raw logs and status:
52 thread-rejection catalog paths + 20 global-rejection paths + two local sync/lock
controls. At **-O0 only**, 47 compile and run on Rust; 46 have byte-identical output
and exit status to the unchanged tagged C compiler. One differs in pre-existing
nested numeric-array formatting; 27 fail compilation. `backlog.json` records every
remaining path and observed diagnostic. Existing tagged probes include timing and
native cases; these are observed executions, not proof of all interleavings.

`scripts/test_rust_concurrency.py` runs 15 deterministic positive sources and one
negative source through both compilers without changing source or normalizing
output. All pass. The Rust leg sets SN_CC to a nonexistent executable. The negative
requires identical raw compiler diagnostics and is reported separately from
execution parity. The positive sources exercise argument order, repeated joining,
scope-exit joining, owned argument lifetime through join, concurrent visibility,
early-return unlocking, global initialization and mutation, and name hygiene.
Seven former Rust rejection fixtures are preserved byte-for-byte under
`tests/rgen/concurrency-promoted`; their original paths, SHA256s and exact old
rejection text are retained in the manifest, and the differential script executes
them explicitly. They are not silently excluded from verification.

The AArch64 tagged float-postfix case initially fails linking
`__atomic_feraiseexcept`. The successful supplemental deterministic run explicitly
uses `--tag-ldlibs '-lm -pthread -latomic'`, recorded in its results. This uses the
existing toolchain library; no compiler, C config, source or host change was made.
The 74 tagged catalog comparisons use the original tagged configuration.

Existing Rust suites: generation 235 pass / 16 pre-existing failures; negative
168 pass / 8 pre-existing post-tag semantic expectation failures, after the seven
executed promotions; toolchain 6/6 pass. Compiler unit binary: 1608/1608 pass.
`verification.json` records executable hashes and protected C/shared file identity.
No full corpus or all-mode parity claim is made.

## Unresolved lifetime observation

`observations/reassign_before_join.sn` preserves the initial new probe that
reassigned borrowed string/array arguments before joining. Tagged C compiled but
exited with signal 11; Rust returned `kept: 42`. The original evidence remains in
`deterministic-1`. This is not classified as a C defect, a defined-behavior parity
oracle, or a completed case. The positive lifetime test keeps arguments alive
through the join before reassigning them.

## Reproduce

From this worktree, using existing project dependencies:

```
python3 scripts/test_rust_concurrency.py \
  --tag-compiler /path/to/unchanged-v0.0.83/bin/sn \
  --output /unique/new/evidence-directory \
  --tag-ldlibs '-lm -pthread -latomic'
python3 scripts/run_rust_tests.py all --compiler ./bin/sn
```

The differential script refuses to reuse an output directory and gives each
compiler/case its own temporary directory and executable timeouts. The optional
link flags are supplemental evidence and should be omitted when not needed by
the supplied tagged toolchain. Root owns review, CI coordination and integration.
