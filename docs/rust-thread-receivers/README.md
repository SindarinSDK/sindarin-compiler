# Thread-private receiver and aggregate ownership

Dependent on preserved PR133 head
`824195cdb54e8ab91386427996aa5a02fc47917a`. This branch does not change PR127
or PR133 and has not integrated current main. Root supplies the corrected
foundation for a later ordered restack. The mutable lane's supplied checkpoints
`132f0e4f957` and `dd7504` use invocation-local snapshots; no shared-owner API
was assumed or imported from those branches.

## Private interface for composition

Only structures selected by a thread receiver or aggregate argument receive the
field-storage projection. Their fields use a hygienically named `Field<T>`
containing `Arc<Mutex<T>>`. `read()` takes an owned rvalue snapshot and releases
the lock; `set()` replaces a field; `share()` retains the same field owner.
Checked scalar mutations retain the existing checked arithmetic lowering and
hold a field guard only for the read/modify/write. Operands are evaluated before
the mutation guard. No guard spans a method call, callback, spawn or join.

A projected value struct's ordinary Clone snapshots fields. Its private share
method retains field owners for a receiver/reference transfer. A tagged
reference-counted struct's Clone retains those owners as well, preserving alias
identity. Spawn captures the receiver before arguments. Ordinary value-struct
thread arguments retain their tagged copy behavior; supported reference-counted
arguments retain identity. The representation is safe Rust without unsafe
Send/Sync, raw-pointer lifetime assertions, or new source restrictions.

This is a thread-private interface, not an API added to the C model, AST, runtime
or frontend. It is separate from the mutable lane's non-thread pointer-backed
capture work. Later composition must explicitly choose snapshot versus share at
an ownership boundary; deep cloning cannot substitute for a retained reference.

## Scope and evidence

`verification.json` records exact compiler/source identities and raw evidence
paths. Every new differential harness first ran the mandatory fresh smoke using
`/tmp/sindarin-tagged-control-reference.json`, exact tagged cwd/compiler, default
C, `--no-install -O0`, and the unchanged negative-integer stdout oracle. Each
feature compiler/run has its own TMPDIR/output and captured stdout, stderr and
status; execution requires a fresh executable. Rust uses a nonexistent C
compiler. No output normalization is used.

The review run has ten raw O0 matches: three unchanged tagged sources
(`test_thread_spawn_self_method`, `test_thread_struct_param`,
`test_pass_self_to_function`) and seven new deterministic probes. They cover
visible receiver writes with method calls in the RHS, checked mutations,
reference-counted aggregate writes and aliases, caller rebinding before join,
value snapshots, aggregate returns, and nested self-method spawn/join. No sleeps
or changed tagged expectations are involved. These three catalog paths now have
focused parity evidence; this is not a new full-corpus measurement.

The existing ownership preservation run has 20/20 raw O0 matches, including
scalar reference forwarding/aliasing, synchronized reference storage, closure
captures and pending-handle array aliases. Full Rust checks retain 237 generation
passes/16 known failures, 163 negative passes/eight known failures/five inherited
promotions, 23 positive and one negative closure passes, and six toolchain passes.
Final generation checks and the corrected-worktree unit binary (1608 passes)
are recorded separately with their actual provenance. Earlier suite evidence is
not relabeled as a rerun after later edits.

All changed production files are under Rust target/templates. C/shared sources,
tagged fixtures, harness/config and runtime are unchanged from the parent.

## Required remaining work and observations

Full concurrency parity is incomplete. General nested aggregate/array mutation
places, pointer-backed field aliases, global aggregate receiver places,
reference-return lifetimes, aggregate closure transfer and native composition
remain required; this bounded scalar-field/owned-string slice does not establish
them. `remaining.json` retains the 15 other previously observed catalog compile
blockers, without claiming a fresh inventory or treating rejections as parity.
The earlier worker-error fixture evidence does not establish every runtime error.

Raw unsuccessful C attempts are retained in the evidence directory and their
exact sources under `tests/rust-thread-receivers/observations`: stack value-struct
reference spawn was rejected; replacing a string in the value-copy probe aborted
at cleanup; nested reference-counted method spawns returned the expected worker
result but the subsequent caller read did not match; direct spawning on a local
value receiver failed C compilation. These are unresolved observations, not
positive parity, C-defect claims, or reasons to alter C/frontend semantics.
The accepted nested case uses the tagged-valid `self` receiver form. All those
attempts had a passing fresh control first.
