# Rust backend completion ledger

Goal: verified feature and behavioral parity with C on integrated main, preserving
the established language contract and C as the default target. Rejections are
remaining implementation work, not parity. This ledger supersedes historical
head counts, not the language specification or the historical evidence itself.

## Baseline: 2026-10-02

Remote main verified as `4dba66c151133f83bf4e11a5b0c12c414384bb6a`.
Isolated branch: `integration/rust-parity-completion`. The older conflicted main
checkout is untouched. Linux aarch64, GCC 13.3.0, rustc 1.93.1; Release CMake
build with Ninja, eight build workers; test runners use their default 20 workers.
Existing project-local libs copied into `.sn`; no host software changes.

Commands: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
-DCMAKE_C_COMPILER=gcc`, `cmake --build build -j 8`,
`python3 scripts/run_rust_tests.py all --no-cleanup --no-color --verbose`,
`python3 scripts/run_tests.py all --no-cleanup --verbose`, and
`python3 tests/rgen/byte_string_compare.py`.

| Suite | Pass | Fail | Skip |
|---|---:|---:|---:|
| Rust generation | 295 | 16 | 0 |
| Rust negatives | 164 | 13 | 5 |
| Native tagged / extra / origin / negative | 6 / 12 / 1 / 2 | 0 | 0 |
| Closures positive / negative | 36 / 1 | 0 | 0 |
| Rust toolchain | 12 | 0 | 0 |
| C unit / cgen / model | 1608 / 107 / 79 | 0 | 0 |
| C integration / negative | 1141 / 58 | 0 | 0 |
| C exploratory | 223 | 1 | 0 |
| C exploratory negative | 11 | 0 | 0 |

Raw-byte comparisons: 63 executions pass, including ten C/Rust paired fixtures
at O0/O1/O2 and three Rust-only raw argv executions. Raw logs are retained in
`rust-parity-evidence/main-4dba66c-*.log`.

The C failure is `test_limitation_closure_array`: AddressSanitizer reports a
heap-buffer-overflow in concurrent `sn_array_push_safe`, with another thread
reallocating the storage. Do not mask it by rerunning until green or excluding it.
Determine the defined synchronization contract before making a semantic change.

Rust failures include shared-frontend admission failures, stale generated-source
snapshots after byte-string support, and obsolete negative expectations. Each
needs classification and behavior verification; do not bulk bless snapshots.
Five skips are explicit promotions to closure regression tests; trace their
coverage and remove redundant skipped entries when equivalence is verified.

## Completion checklist

- [ ] Current full Rust baseline failures classified and resolved without altering C oracles.
- [ ] Receiver alias forwarding: PR150 (`a53de135`) integrated and reverified.
- [ ] General array identity, indexed/nested ownership, dynamic aliases and rebinding.
- [ ] Remaining closure captures, callable contexts and reference lifetimes.
- [ ] Concurrency/globals/synchronization: reconcile PR127, PR133, PR134, PR142.
- [ ] Native managed transport, pointers, structs, callbacks and SDK: reconcile PR143 and remaining gaps.
- [ ] Remaining string, numeric, matching, reflection, module and type features inventoried and completed.
- [ ] Platform runtime gates: PR144 Windows failures resolved; Linux/macOS/Windows green on final head.
- [ ] Automated differential coverage of behavior, raw streams, status, order, mutation and lifetimes across modes.
- [ ] C baseline sanitizer failure explained/resolved within established language semantics.
- [ ] Full integrated suites pass without unexplained failures or hidden skips.
- [ ] All changes merged into main; accurate architecture/usage/coverage docs and final evidence report.

## Integration queue

Live inspection: PR150 has green three-platform compiler checks and is mergeable.
PR144 has failed Windows compiler/runtime checks. PR127, PR133 and PR143 have
merge conflicts; PR134 and PR142 are mergeable but depend on unmerged foundations.
PR descriptions and historical focused gates are not final-head verification.
Current main CI does not execute the complete Rust suite.

PR150 is being composed locally first. No remote merge has occurred in this goal.

## Local integration results

PR150 head `a53de135e1e0c4a980ae23aff6c093aa9daf3e32` fast-forwarded
into the isolated branch. Rebuild succeeds. Full Rust suite: 300 generation passes,
16 generation failures; all other suite totals unchanged. The 29 failing
case/reason pairs match the baseline (durations are not part of the comparison).
Five receiver fixtures pass in 15 source-identical C/Rust pairs at O0/O1/O2, with
zero compile/run statuses and identical raw stdout/stderr. Evidence:
`pr150-rust.log` and `pr150-receiver-pairs.json`. Remote PR remains open.

Corrected three obsolete statement-match rejection messages to include the already
implemented char subject. No source fixture, runtime oracle or compiler behavior
changed. Focused `rgen-errors --filter statement_match_` passes; see
`statement-match-negatives.log`. This removes three stale test failures, not three
language gaps. The next full negative run is required after further changes.

Raw harness logs deliberately retain terminal progress carriage returns and
trailing spaces; whitespace checks apply to source/docs edits, excluding these
unmodified captured logs. They also retain failure traces and statuses.

## Iterator unchecked-mode repair and differential gate

Added `scripts/check_rust_parity.py` and `make test-rust-parity-core`. The gate
requires an explicit nonempty fixture selection, existing sources/compiler,
successful compilation and zero runtime status on both targets. It compares raw
stdout/stderr and records compiler/source SHA-256 hashes, commands, statuses and
raw streams in JSON. Missing artifacts, timeouts and failed controls fail the
gate; special `.args`/`.exit`/`.panic` contracts are explicitly rejected rather
than silently ignored. This positive gate complements, not replaces, negatives
and independent runtime expectations.

Differential testing exposed Rust rejection of signed iterator-binding postfix
mutation in unchecked O2. Removed the obsolete checked-only validation guard:
these bindings are mutable local copies and existing local lowering applies.
The same unchanged comprehensive iterator source now matches C at O0/O1/O2.
All six neighboring iterator negative tests pass. No C/shared production changed.

Refreshed five stale generated-Rust snapshots only after paired execution checks:
by-value scalar assignment (checked and unchecked fixture), numeric mutations,
iterator mutations and resolved callable methods. Existing sources and runtime
expectations are unchanged. The rgen runner verifies those expectations after
snapshot matching: now 305 passes, 11 failures, no skips. The core differential
gate passes all 30 pairs (60 executions), covering those five fixtures plus the
five receiver-alias fixtures at each optimization level. Evidence is
`core-after-iterator.json` and `rgen-after-snapshot-repair.log`.

The separate `resolved_calls.sn` historical Rust fixture fails C compilation at
all three optimization levels: heap-owning operator arguments are emitted as
values where C expects pointers. Rust executes it successfully. Do not claim it
as C parity evidence or change C semantics merely to satisfy this Rust fixture.
Its snapshot remains unresolved separately from the real iterator gap.

## Obsolete rejection fixtures promoted

Four former negatives now compile and execute identically with C and Rust at
O0/O1/O2: opaque declaration alongside generic identity, string-to-int admission,
string append as a value-match result, and append as an expression prefix.
Moved their byte-identical sources into rgen, recorded C stdout as the runtime
oracle and generated Rust snapshots. The conversion fixture only checks
admission (its result is unused), not the runtime conversion value.
All 12 pairs pass; `promoted-positive-pairs.json` retains the original paths and
source hashes. Full Rust run after promotion: rgen 309 pass/11 fail/0 skip;
negatives 167 pass/6 fail/5 explicit promotion skips. Native, closure and toolchain
counts remain passing and unchanged. Remaining negative failures are three old
messages and three reference-argument cases requiring C-admission classification.

The C exploratory sanitizer failure is consistent with the documented contract:
`docs/threading.md` requires locks when threads mutate shared arrays. Its current
source spawns three closures that push into the same array without locks. It
cannot provide a defined C behavior oracle; final regression-gate treatment
still requires an explicit, documented resolution rather than hiding the failure.

## Working strategy (user update)

Verified existing PRs may be merged; new changes may be committed and pushed
directly to main after required local tests pass. Refresh and verify the combined
revision before each update. Monitor every resulting CI run and prioritize any
failure immediately, pausing unrelated integration until green. Preserve useful
work before closing superseded PRs. No routine approval is required.

PR150 merged remotely as `6605f87f502382b74e100b0280d2bcf59c991ab0`.
Post-merge Compiler run: `37033394816`; Ubuntu passed at the latest observation,
macOS and Windows still running. No local follow-up commits have been pushed.

## Pointer and opaque sizeof

C's `helper_c_sizeof_min` emits `sizeof(void *)` for pointers and opaque types.
Rust now emits `std::mem::size_of::<*mut std::ffi::c_void>() as i64`, resolving
size on the compilation target rather than hard-coding the host layout. Existing
scalar/managed-handle rendering is unchanged. Promoted two unchanged former
negative sources, and added an observable type-size fixture covering primitive,
opaque, nested-char and string pointees. The five sizeof rgen tests and six
remaining sizeof negatives pass. Nine source-identical C/Rust pairs at O0/O1/O2
pass; evidence: `sizeof-pointer-pairs.json`. A draft pointer-expression test was
rejected by both shared frontends because ordinary functions cannot declare
pointer variables/returns; it was replaced by legal type-form coverage, not
counted as a backend gap or parity result.

## Reconciled local baseline

All eight formerly positive admission failures were independently compiled through
C and Rust emission: identical shared-frontend errors. Their sources now execute
as explicit `shared_frontend_*` rejection tests, not passing feature claims.
Historical snapshots/output sidecars are retained under
`docs/restoration/post-tag-fixtures/tests/rgen`. Seven source files are byte
identical; the import fixture changes only its relative fixture path to account
for relocation. `shared-frontend-rejections.json` records original identities
and both target diagnostics.

Two default-mode overflow fixtures contain unused arithmetic eliminated by the
shared optimizer. C and Rust both exit zero with empty streams. Archived their
obsolete panic/snapshot expectations and refreshed the active snapshots without
changing sources or optimizer settings (`default-dce.json`). Other checked
overflow fixtures remain active. The Rust-only `resolved_calls` snapshot was
refreshed only after its original runtime oracle passed at all three levels;
it is still not C parity evidence. Two obsolete diagnostic messages now match
the remaining native-aggregate and array-contains boundaries.

Three as-ref negatives exposed missing Rust validation of explicit references
to literals/computed values and fields rooted in temporary structs. C either
fails native compilation or dereferences an invalid pointer for these sources;
the shared call checker already states that explicit as-ref arguments require
a variable/field. Added the missing target-local storage check to ordinary and
static calls, preserving explicit temporary-borrow metadata for operators.
Sources/expected rejection texts are unchanged. Raw controls are recorded in
`reference-argument-controls.json`; invalid C executions are not parity oracles.

The five skipped historical closure negatives are byte-identical to active
positive closure tests. Archived the duplicate rejection copies and removed the
harness skip list. `closure-promotion-identity.json` maps every archived source
to its executed positive counterpart and SHA-256. No feature was excluded.

Replaced the racy C exploratory fixture with the existing synchronized historical
regression and its PASS oracle. It explicitly locks shared closure-array mutation
and asserts distinct returned lengths 4,5,6. The original source remains verbatim
in `unsynchronized-closure-array.sn.txt`; the initial sanitizer trace remains in
the baseline log. This repairs a test violating documented synchronization rules,
not compiler/runtime semantics, and is an explicit change to the restored corpus.

Fresh local gates: Rust generation 315; negatives 179; native tagged/extra/origin/
negative 6/12/1/2; closure positive/negative 36/1; toolchain 12. All pass, zero
failures and zero skips. C unit/cgen/model/integration/integration-negative/
exploratory/exploratory-negative: 1608/107/79/1141/58/224/11, all pass with no
skips. Formatting passes. Logs: `reconciled-rust.log`, `reconciled-c.log`.
