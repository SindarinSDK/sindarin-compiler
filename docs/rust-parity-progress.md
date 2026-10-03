# Rust backend completion ledger

Goal: verified feature and behavioral parity with C on integrated main, preserving
the established language contract and C as the default target. Rejections are
remaining implementation work, not parity. This ledger supersedes historical
head counts, not the language specification or the historical evidence itself.

## Current verified status: 2026-10-03

Main `bfc0cb99` passes Linux/macOS/Windows Compiler and Rust Runtime CI,
including 594 exact C/Rust positive pairs and 39 diagnostic/order cases. All
existing PR work is reconciled; the open queue is empty. C remains the default
target. Sized-array reflection/defaults, buffered output and mixed integral
conversions are integrated. Full parity is still incomplete: the broader
unchanged C-positive corpus has 96 Rust admission/compilation gaps, and remaining
Rust-negative, ownership, native/SDK and language families need implementation
and full mode/platform validation.

The next local increment implements float/double conversion at scalar storage,
call and return boundaries, and mixed floating compound mutation. The unchanged
`test_interop_types.sn` now passes all nine C/Rust controls. Full local suites
pass; the expanded gate requires 117 new pairs, including observable synchronized
cell updates. The unchanged broader corpus now has 95 compilation gaps. This
increment still requires publication and hosted verification; the main status
above remains the last verified hosted checkpoint. Details follow below.

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

- [x] Current full Rust baseline failures classified and resolved without altering C oracles.
- [x] Receiver alias forwarding: PR150 (`a53de135`) integrated and reverified.
- [ ] General array identity, indexed/nested ownership, dynamic aliases and rebinding.
- [ ] Remaining closure captures, callable contexts and reference lifetimes.
- [x] Existing concurrency PR127, PR133, PR134 and PR142 integrated and verified.
- [ ] Remaining concurrency behavior, diagnostics and ownership parity.
- [ ] Native managed transport, pointers, structs, callbacks and SDK: reconcile PR143 and remaining gaps.
- [ ] Remaining string, numeric, matching, reflection, module and type features inventoried and completed.
- [x] Platform runtime gates: PR144 Windows failures resolved; Linux/macOS/Windows green on integrated main.
- [ ] Automated differential coverage of behavior, raw streams, status, order, mutation and lifetimes across modes.
- [x] C baseline sanitizer failure explained/resolved within established language semantics.
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

## Main baseline published and platform PR repair

Baseline corrections pushed directly to main at
`a03421325a4a5785eec1645633d68ff99fc00126`; Compiler CI run `37034635319`
passes on Linux, macOS and Windows. PR150's merge run `37033394816` also passes
on all three platforms. The synchronized exploratory test and every active
Rust suite pass locally; this is baseline health, not full language parity.

PR144 is composed onto that main in the isolated branch. Its prior Windows
runtime logs identify missing libgcc/libgcc_eh, a missing hashlib import,
host-dependent generated-source mismatches, and a C native-output mismatch.
The repaired workflow selects x86_64-pc-windows-gnullvm with existing LLVM-MinGW:
Rust's official platform documentation specifies UCRT, LLVM tools/libraries,
Windows extern-C calling conventions and LLVM-MinGW compatibility:
https://doc.rust-lang.org/rustc/platform-support/windows-gnullvm.html
Hosted verification is still required; changing the triple alone is not proof.

Windows output support now emits portable Rust cfg(windows) functions/macros.
Generation is host-independent and the actual Rust target selects text behavior.
All 351 active Rust snapshots were regenerated; source fixtures and runtime
oracles remain unchanged. Full local Rust suites pass with the published
baseline counts and zero skips. Full C suites pass with the published counts.
Raw-byte comparisons (63 executions), text/native/diagnostic transport (six
executions), and Windows helper logic pass. The helper test forces only the
platform-independent adapter cfg on Unix; it is not Windows ABI/OS evidence.

The C Windows output discrepancy remains a required gate. Runtime failures now
retain executable, generated C and raw streams under .sn/rust-runtime-diagnostics
and upload those artifacts. An intentional mismatch verified that diagnostic
retention actually works, including Unix byte-valued subprocess argv. No C
output oracle was weakened or normalized to accommodate the discrepancy.

## Windows C control diagnosis and helper namespace repair

At PR144 revision `8af63b45`, Compiler CI `37037991223` passes on all three
platforms; runtime CI `37037990533` passes Linux/macOS but fails Windows.
Retained Windows generated C explains the missing output: the split model routes
the source's main function into an imported translation unit because model paths
contain forward slashes but the Windows invocation uses backslashes. The emitted
entry point has only deferred initialization and fflush. This is a C path-routing
bug, not a passing C behavior oracle. Preserve the failing job log and generated
C/stream metadata under `rust-parity-evidence/pr144-windows-before-path-fix/`.

Normalize the splitter's entry path to the model representation. Also sanitize
drive-letter colons in imported-module filenames; Windows otherwise rejects the
staged raw fixture object path. Two portable model-splitting regressions cover
main routing and external drive paths. Windows native debug executions retain
address sanitization with detect_leaks=0 because LeakSanitizer is unsupported;
non-Windows leak settings are unchanged.

Allocate Windows output helper names against the complete source model, including
previously allocated names, rather than reserving spellings legal in Sindarin.
The new `windows_output_helper_hygiene` source defines all five original helper
names and a suffix local. It matches C at O0/O1/O2. The forced Windows helper
test now checks the actual generated program, including the collision fixture,
exact LF/CRLF conversion, raw bytes and checked-error stderr/status. The argv
adapters are removed only in that Unix simulation; real target argv tests remain.

Local full C suites pass: 1610 unit, 107 cgen, 79 model, 1141 integration,
58 integration-negative, 224 exploratory, 11 exploratory-negative. Full Rust:
316 generation, 179 negative, native 6/12/1/2, closures 36/1, toolchain 12;
all pass with zero skips. Raw-byte 63 executions, native/text/diagnostic six
executions, core 30 C/Rust pairs, helper logic, formatting and source whitespace
checks pass. Logs: `platform-path-c.log`, `platform-path-rust.log`; core results
in `platform-path-core.json` and hygiene pairs in `windows-hygiene-pairs.json`.

The runtime workflow now runs complete Rust suites and the differential core on
every supported platform, and also triggers on pushes to main. Removed the
obsolete macOS exclusion for the repaired synchronized closure-array test. The
existing macOS thread-panic exclusion remains explicit unfinished platform work.
Fresh hosted checks must verify this follow-up before PR144 can merge.

## Expanded hosted gates and portable toolchain checks

At `7bcc1609`, Compiler CI `37039769279` passes all three platforms. Runtime
CI `37039769263` passes Linux. On macOS every runtime, generation and diagnostic
suite passes except the link-driver harness, whose GNU --defsym markers are
invalid for Apple ld. Windows now passes all 30 exact raw-byte C/Rust pairs plus
three UTF-16/WTF-8 argv executions and all six native/text/diagnostic transport
executions. This confirms the C path fix and Windows output behavior on the
actual host, rather than the Unix helper simulation.

Expanded Windows checks expose LLVM 19 AddressSanitizer startup crashes on a
CRT instruction (`44 0f b6 1a`), three Unix-only binary output oracles, the
explicit unfinished Windows C-driver capture case, and the core gate's missing
.exe compiler suffix. Raw failing logs are retained as
`pr144-macos-expanded.log` and `pr144-windows-expanded.log`.

Use portable library-search marker arguments in link-driver checks, assert the
platform's actual configured @link sequence, and execute Windows capture through
a Python-backed .cmd driver whose path has spaces, quotes and ampersands.
Preserve configured target flags, pin exact forwarding/order and run the linked
executable. Wrap Windows sidecar compilation with cmd.exe's outer command quotes.
Forced final-link failure now executes on Windows too; it is no longer silently
omitted inside a passing aggregate case.

Windows binary oracles are explicit immutable `.windows.expected` files, using
the exact bytes already verified against unchanged C at all three modes. Their
CRLF is preserved through Git checkout; runtime bytes remain unnormalized.
The differential script and Make target choose the platform executable suffix.
Captured evidence logs now retain original transport bytes through Git.

Pin LLVM-MinGW 20260616 (LLVM 22.1.8) and its published SHA-256 in both workflows,
extracting it under the checkout's `.sn/toolchains`. Remove the system-LLVM
deletion step; only the project toolchain needs selection. Upstream LLVM commit
ce4618a9c405bd8a9c1e096eb45e9ca83d3891f1 explicitly fixes the observed Windows
11 24H2 interception instruction:
https://github.com/llvm/llvm-project/commit/ce4618a9c405bd8a9c1e096eb45e9ca83d3891f1
Release and digest:
https://github.com/mstorsjo/llvm-mingw/releases/tag/20260616
Hosted debug execution remains required; sanitizers are retained. Windows C
release flags retain -O3 with -fwrapv to preserve the configured wrapping contract.

Remove the last macOS test exclusion and execute thread-panic propagation. Local
focused C test passes, complete Rust suites retain the 316/179/native/closure/
toolchain counts with zero skips, and the differential core passes all 30 pairs.
Logs: `platform-portable-rust.log`, `platform-portable-toolchain.log`; fresh
three-platform CI must verify this repair before integration.

## Bootstrap toolchain override diagnosed

At `2a7aeef4`, Compiler CI `37041533438` passes all platforms, including the
previously excluded macOS thread-panic test. Runtime CI `37041533439` passes
Linux/macOS. Windows passes all 316 generation cases, 179 negatives, native
tagged/origin/negative suites, closures 36/1, C-driver capture, forced native
compile/link failure, all raw-byte/transport probes, and all 30 differential
core pairs. Only debug executions fail in the sanitizer startup instruction.
Preserve the complete Windows log as `pr144-windows-bootstrap-override.log`
and actual hosted raw core results as `pr144-windows-core.json`.

The job prints Clang 22.1.8 before setup but Clang 19.1.6 at build time. The
bootstrap `make setup` invokes `install.ps1`, whose prerequisite installer
downloads LLVM-MinGW 20241217 again and prepends it through GITHUB_PATH. Thus
the previous run did not execute debug programs with the configured new compiler.
Do not claim that upgrading the pin failed to fix LLVM or that new sanitizer
execution passed.

Add an explicit SN_SKIP_PREREQS=1 bootstrap option; normal installations still
install prerequisites. Both Windows jobs already provision the pinned compiler
and build tools, so they opt out of replacing them during bootstrap. After setup,
assert that Get-Command clang.exe resolves to the project toolchain's exact path,
then record its version before building. Hosted verification must establish the
compiler selection and every debug execution on the new revision.

Compile-admission inventory at `2a7aeef4`: all 179 active Rust-negative sources
were independently compiled through C at O0. 132 compile successfully; 47 fail
or reject. This is an investigation list, not 132 proven runtime gaps or parity
results. No program was executed, so runtime validity remains unclassified.
`c-admission-inventory.json` records source/compiler identities, commands, raw
compile streams, statuses and the current Rust rejection oracle.

## Full runtime gates green; synchronized C declaration correction

At `eb916370`, Runtime CI `37043572081` passes all Linux/macOS/Windows jobs,
including complete Rust suites, all debug ABI/sanitizer controls, driver capture,
forced link failures, raw-byte/transport checks and the 30-pair differential core.
Windows records Clang 22.1.8 both after setup and at build. This verifies the
bootstrap selection repair and actual sanitizer execution, not only a tool pin.

Compiler CI `37043572176` passes Linux/macOS but finds one Windows integration
failure under Clang 22: `test_sync_var_array`. C templates emitted `_Atomic char *`
and `_Atomic SnArray *`, qualifying the pointee rather than the pointer handle;
assignment and array runtime arguments then have incompatible pointer types.
Emit `_Atomic(<complete C type>)` in definitions, extern declarations and the
ordinary synchronized local declaration route. Existing scalar spellings change
equivalently from `_Atomic long long` to `_Atomic(long long)`; refreshed only
the two affected C snapshots after checking their generated diff. Fixture sources
and runtime expectations stay unchanged. C remains the default target.

The original synchronized array/string integration source produces exactly
`items: 4` and `message: done` at O0/O1/O2. Evidence:
`atomic-pointer-controls.json`. Full local C suites retain the 1610/107/79/1141/
58/224/11 passing counts; complete Rust suites retain 316/179/native/closures/
toolchain counts, with no skips. Core 30 pairs, raw-byte 63 executions, transport
six executions and formatting pass. Logs: `atomic-pointer-c.log` and
`atomic-pointer-rust.log`. Preserve the new-toolchain Windows runtime success
and C compile failure logs for review; the next hosted revision must verify the
C fix before integration.

## Synchronized C array teardown correction

At `c39d8cc2`, Runtime CI `37044970841` passes all three operating systems.
Compiler CI `37044971073` passes Linux/macOS; Windows now accepts the corrected
atomic pointer declarations but rejects global array teardown in the unchanged
`test_sync_var_array` fixture: `_Atomic(SnArray *) *` cannot be passed to the
ordinary `SnArray **` cleanup helper. Preserve that exact failed job log as
`pr144-windows-atomic-cleanup-failure.log`.

For synchronized global arrays, both C module templates now load the pointer
value and call the existing `sn_array_free` helper; ordinary global cleanup is
unchanged. This avoids aliasing atomic storage through an ordinary pointer.
The original fixture passes strict incompatible-pointer diagnostics and exact
output checks at O0/O1/O2 on the worker's GCC toolchain;
`atomic-cleanup-controls.json` records commands and raw streams. Clang is not
installed on this worker, so the hosted Windows Clang 22 check remains required.

Full local C and Rust suites pass with their unchanged counts and zero skips;
core 30 C/Rust pairs and formatting pass. Logs: `atomic-cleanup-c.log` and
`atomic-cleanup-rust.log`. Hosted results for this fix are still pending.

## Native-pointer unit fixture initialization

At `650faaa9`, Windows compiler CI `37046329604` passes all 1141 integration
tests, verifying synchronized pointer declarations and cleanup on Clang 22.
The unit executable instead asserts in `test_inline_pointer_passing_allowed`: its
stack `Parameter` array initializes only name/type, leaving `mem_qualifier` and
`sync_modifier` indeterminate. The log shows spurious as-ref diagnostics for the
unqualified native pointer argument. Initialize both arrays in that inline
fixture to zero (MEM_DEFAULT/SYNC_NONE), plus the sole remaining uninitialized
stack Parameter array in the AST unit fixture. Existing assertions and production
checker semantics stay unchanged. Preserve the failed job log as
`pr144-windows-uninitialized-parameter-failure.log`.

Local full C/Rust suites and core 30 pairs pass after initialization, with no
skips and unchanged counts; formatting passes. Logs: `parameter-init-c.log`
and `parameter-init-rust.log`. Runtime CI `37046329688` passed all three systems
at the preceding revision; the unit initialization needs fresh hosted checks.

## PR144 integrated and verified; PR127 composed locally

PR144 merged as `9614ecadc37864f33228b77952ea185546e541c5`, after all six
compiler/runtime jobs passed at exact head `9b8a03b0`. Main's Compiler
`37047915370` and Runtime `37047915440` also pass all three platforms.
`pr144-main-ci-green.json` records exact revision, run and job URLs/results.

PR127 head `fa0e708f` is composed with that main locally. Resolve conflicts by
retaining current arithmetic casts, assertion dispatch, opaque type admission,
byte-string and Windows output support, then adding private concurrency lowering.
The 316 existing Rust snapshots stay unchanged and pass. Verify all seven
promoted source SHA-256 hashes before refreshing only the 17 new positive
snapshots; unchanged runtime expectations then pass. Full C counts stay
1610/107/79/1141/58/224/11. Rust: 316 generation, 172 remaining negatives,
native 6/12/1/2, closures 36/1, concurrency 10/7/1, toolchain 12, all passing
with zero skips (`pr127-local-c.log`, `pr127-local-rust.log`).

Default-toolchain differential controls pass 48 of 51 pairs; the floating
postfix source fails C linking at all three levels on this AArch64 GCC worker
(`__atomic_feraiseexcept`). Retain those failed controls separately as
`pr127-default-toolchain-pairs.json`. With the explicit recorded C-only Linux
link supplement `-lpthread -lm -latomic`, all 51 pairs pass. The new
`make test-rust-parity-concurrency` requires exactly 17 sources, records that
flag and raw streams in `pr127-all-mode-pairs.json`, and runs on every runtime
CI platform. Missing fixture-count checks fail before execution. The `all`
Rust runner now includes concurrency suites instead of silently omitting them.
Hosted verification of the composed PR is required before its merge.

C-only triage of the 132 previously compile-admitted Rust negatives now executes
O0/O1/O2 controls. 118 exit zero at O0; 117 at O1/O2. 117 sources exit zero
at all modes, including 46 with no output; silent execution does not establish
observable semantics. Thirteen sources crash on this worker, one has nonzero
normal exit, and one compiles at O0 but fails after optimization. Raw statuses
and streams are in `c-execution-inventory.json`. These are classification
inputs, not completed Rust parity or automatically proven C defects.

Final local core 30 pairs, raw-byte 63 executions, transport six executions,
Windows helper simulation and formatting pass after the composed changes.
Evidence: `pr127-bytes.log`, `pr127-transport.log`, `pr127-windows-helpers.log`.

## PR127 integrated; shared thread ownership and array identity composed

PR127 passes all six hosted checks at `2bf57b91` and merges as
`cbaf22b852292496aa26841b11cdd978808a7032`. Post-merge Compiler
`37049464036` and Runtime `37049464061` must complete before the next
publication. The new `all` concurrency suites and 51-pair gate execute on
main instead of remaining only historical PR evidence.

PR142 head `8ffabb4b` contains current PR133 head `50906e5f` and old PR127
head `fa0e708f`; composing PR142 preserves that work by ancestry. Prepare it
in a separate worktree based on updated foundation `2bf57b91`, with a symlink
to existing project-local libraries and no host installations. Conflict resolution
retains current Windows output and declaration casts, then adds thread owner,
reference and array companion support. All 316 existing Rust generation tests,
172 negatives and closure/native/toolchain suites pass. Seven new concurrency
snapshots change with ownership helper composition; refresh those only after
their original C/Rust controls and runtime oracles pass.

All 11 ownership sources match at O0/O1/O2. The expanded gate requires 23
root-level fixture files (11 ownership plus 12 array-identity files, three of
which are preserved `.sn.raw` originals). It compiles original paths directly,
without staging or formatting, and executes default/checked/unchecked arithmetic
at all three optimization levels: all 207 pairs pass. Raw streams, modes,
commands and source/compiler hashes are retained in `pr142-thread-all-mode-pairs.json`.
All 51 foundation pairs also pass (`pr142-concurrency-pairs.json`), with the
explicit Linux C-only libatomic supplement retained and recorded.

Full C counts remain 1610/107/79/1141/58/224/11; full Rust counts remain
316/172, native 6/12/1/2, closures 36/1, concurrency 10/7/1, toolchain 12.
All pass with zero skips. Core 30 pairs, raw bytes 63 executions, transport
six executions, Windows helper simulation and formatting pass. Logs are
`pr142-local-c.log`, `pr142-local-rust.log`, `pr142-bytes.log`,
`pr142-transport.log` and `pr142-windows-helpers.log`. Hosted checks of the
combined PR142 revision are still required before integration.

Post-PR127 main Compiler `37049464036` and Runtime `37049464061` finish
with all six jobs successful at `cbaf22b8`; exact job/run URLs and results
are retained in `pr127-main-ci-green.json`. Main has the identical source tree
to the tested foundation head `2bf57b91`.

## PR142 Windows synchronized reference ABI correction

At `8382f357`, Compiler CI `37050270047` passes all platforms; Runtime
`37050270094` passes Linux/macOS and fails Windows. The retained hosted
report records 198/207 ownership pairs passing. All nine failures are the
unchanged `sync_reference.sn`: Clang 22 rejects assigning an
`_Atomic(long long) *` address to the thread argument record's ordinary
`long long *` parameter ABI. Rust compiles/runs successfully, but failed C
controls do not count as parity. Preserve the Windows raw log and JSON as
`pr142-windows-atomic-ref-failure.log` and `.json`.

Reproduce C's incompatibility on the GCC worker with
`-Werror=incompatible-pointer-types -Werror=discarded-qualifiers`; preserve
that before-fix log. Both direct and closure thread argument-packaging branches
now convert reference addresses through `void *` to the existing parameter ABI.
Pointer identity, function signatures, source semantics and runtime oracles
remain unchanged; no warning flags are disabled. The source passes all nine
strict-diagnostic C/Rust pairs after the correction. Complete local suites and
207/51/30 gates are rerun; hosted verification is still required.
Unrelated receiver integration stays paused until this revision is green.

The reference-packaging correction passes full local C/Rust suites with
unchanged counts and zero skips, plus all 207 ownership, 51 concurrency and
30 core pairs and formatting. Evidence: `pr142-atomic-ref-c.log`,
`pr142-atomic-ref-rust.log`, `pr142-atomic-ref-all-mode-pairs.json`.

## Ownership integrated and reconciled; PR134 receiver composition

At exact PR142 head `0ae8d944`, Compiler `37051924804` and Runtime
`37051924762` pass all three platforms, verifying the C atomic-reference
ABI correction. PR142 merges as `432983b307b729c0cb4c11db9dfa39a0b5e2c32e`.
Main Compiler `37053036970` and Runtime `37053036854` also pass all six jobs;
exact URLs/statuses are in `pr142-main-ci-green.json`. PR133 targets the old
concurrency branch, so it remains open despite its entire head `50906e5f` being
an ancestor of integrated main. Close it as incorporated through PR142 after
confirming that ancestry and identical tested source tree; no useful work is
discarded. Only PR134 and PR143 remain open.

Compose PR134 head `699fcd14` with integrated main, retaining current default
array references, numeric casts, Windows output, byte strings, indexed cleanup
and array-text rendering. Shared receiver fields use getters when rendered as
array text. All seven receiver source files match the original PR bytes. No
existing generated snapshot or runtime oracle changes. The new receiver gate
requires ten sources (seven controls plus three unchanged integration fixtures)
and passes all 90 default/checked/unchecked O0/O1/O2 C/Rust pairs. Existing
30/51/207 gates also pass on the refreshed composition: 378 pairs total.

Fresh full C counts are 1610/107/79/1141/58/224/11; Rust counts remain
316/172, native 6/12/1/2, closures 36/1, concurrency 10/7/1 and toolchain 12,
all passing with zero skips. Raw-byte 63 executions, transport six executions,
Windows helper simulation and formatting pass. Evidence is `pr134-c.log`,
`pr134-rust.log`, `pr134-thread-receivers-pairs.json`,
`pr134-thread-ownership-pairs.json`, `pr134-concurrency-pairs.json` and
`pr134-{bytes,transport,windows-helpers}.log`. Hosted validation is pending.

One earlier local gate failed when another Make invocation synchronized runtime
headers during C compilation (`sn_string.h` temporarily absent). Preserve its
failed JSON as `pr134-local-concurrent-build-failure.json`; run builds and
dependent gates sequentially after that diagnosis. The successful final run
refreshes the complete combined revision and does not count the failed control.

The previous hosted Windows report shows Git converted all three `.sn.raw`
probe files from LF to CRLF; their hashes exactly match that conversion. C/Rust
pairs used identical files within each platform, but these were not the exact
committed raw bytes across platforms. Add `*.sn.raw -text` so future Windows
checkouts preserve the original bytes without source edits or reformatting.
Per-platform report hashes must be checked against the originals again.

Simulated Windows-style Git checkout (`core.autocrlf=true`, `core.eol=crlf`,
`cat-file --filters`) now preserves exact bytes for all three raw sources.
Runtime CI retains parity JSON on successful as well as failed runs, enabling
verification of source identities and raw streams even when every gate passes.


## PR143 native composition: local verification

The original PR143 head `1bf1b153` is composed onto verified main `432983b3`.
Two template conflicts preserve current numeric promotion and byte/platform
helpers while admitting the new native managed support. All 316 existing
Rust source-generation snapshots remain unchanged. The native struct rejection
continues to fail admission; only its obsolete list of unsupported constructs
was corrected after native array expressions became supported.

Fresh full C suites pass: 1610 unit, 107 generation, 79 model, 1141 integration,
58 integration negatives, 224 exploratory and 11 exploratory negatives, with
no skips. Fresh Rust suites pass: 316 generation, 172 negatives; native
8 tagged/17 extra/1 origin/4 negatives; closures 36/1; concurrency 10/7/1;
and 12 toolchain cases, with no skips.

The new native gate compares seven source-identical PR fixtures across all nine
optimization/arithmetic combinations: 63 exact raw-stream pairs. Existing core
30, concurrency 51 and ownership/array 207 pairs pass. Raw-byte 63 executions,
native transport 6 executions and simulated Windows helper checks also pass.
Evidence: `rust-parity-evidence/pr143-local-*` and `pr143-source-hashes.json`.

Added an explicit Windows binary oracle for the invalid UTF-8 managed result;
the harness must check its exact CRLF bytes and C/Rust agreement. This remains
pending actual hosted Windows validation. Native fixture counts in CI are
updated to require 8 tagged and 17 extra cases; the new 63-pair gate is required
on all three platforms. Review also repairs cleanup if allocating private
parameter temporary names fails, without changing normal emitted behavior.

This composition still needs refresh against the verified receiver integration,
fresh combined tests, publication and hosted CI before merging. It does not
complete native structs, callbacks, buffer/SDK or general ownership parity.


## PR134 integrated receiver verification

Exact head `01e71f5c` passes Compiler `37054754769` and Runtime
`37054754799`, all six jobs. Downloaded Windows JSON verifies 30 core,
51 concurrency, 207 ownership/array and 90 receiver pairs, all successful;
every source hash matches the worker original, including all three raw fixtures.
See `pr134-windows-report-verification.json` and the retained Windows ownership
report. PR134 is merged as `227fd27aa08932ccad6540d450f1370aa3423151`.
Post-merge Compiler `37056043226` and Runtime `37056043183` are running.
Only PR143 remains open; its local composition is refreshed against this main
and must pass combined tests before publication.


PR143 refreshed onto receiver main `227fd27a`: the complete C/Rust suite
counts above remain green without skips. All 441 differential pairs pass:
30 core + 51 concurrency + 207 ownership/array + 90 receiver + 63 native,
with exact statuses and raw stdout/stderr. Byte-string, native transport and
Windows helper checks pass. Only workflow artifact paths and appended ledger
entries conflicted; all receiver and native gates and evidence are retained.
Fresh combined evidence is `rust-parity-evidence/pr143-integrated-*`.
Hosted three-platform validation is required before this PR can merge.

## Sized-array reflection: local feature completion

Remove the Rust-only `typeOf` sized-array admission guard; existing reflection
metadata lowering already implements the C contract. Preserve resolved-type
validation and compile-time, non-evaluating behavior. Promote the original
three-line negative into a source-identical positive, add an independent output
oracle and generated snapshot, and remove its obsolete rejection expectation.
A second positive observes zero-length/int/string sized arrays, type identity
against dynamic arrays and an effectful operand which must remain unevaluated.
No existing positive snapshot or output oracle changes.

Full local C suites remain 1610/107/79/1141/58/224/11; Rust generation is
318 and negatives 171, all passing without skips. Native counts at this base
remain 6/12/1/2; PR143's 8/17/1/4 update will be composed after its CI passes.
Existing closure/concurrency/toolchain suites pass. Four reflection sources
pass 36 C/Rust pairs under all optimization/arithmetic combinations, including
byte-exact independent Linux oracles for both new positives. Formatting passes.
Evidence: `rust-parity-evidence/reflection-sized-array-*`.

The new reflection gate and report retention are required on every hosted
runtime platform. This local increment is not yet published; it must refresh
against native integration and pass combined checks before a direct main push.
Other reflection types and the broader language gaps remain required work.


## PR queue reconciled and broader C-corpus triage

Receiver main `227fd27a` passes all six post-merge CI jobs, recorded in
`pr134-main-ci-green.json`. Native PR143 head `55f71bcf` passes all six jobs;
its Windows artifacts verify every source hash and all 441 pairs, including
nine invalid-UTF-8 managed-result pairs against the exact CRLF oracle. Evidence:
`pr143-head-ci-green.json`, `pr143-windows-report-verification.json` and
`pr143-windows-native-managed-green.json`. PR143 merges as `9683f657`.
The open PR queue is empty; post-merge CI `37058052765`/`37058052661` is running.

On tested native head `55f71bcf`, the 1365 unchanged positive C integration and
exploratory sources have 1264 successful Rust O0 executable compiles. Running
all sources through the existing C-suite harness with only the compiler target
changed yields 1055/1141 integration and 206/224 exploratory passes, zero skips.
The 104 failures are 101 admission/compilation failures plus three output-oracle
failures: ignored sized-array defaults, and merged stdout/stderr order in assert
and thread-panic programs. This is useful triage, not complete optimization or
cross-platform parity. Raw admission records, commands, source/compiler hashes
and test logs are in `rust-corpus-*` evidence. Callbacks/native structs, composite
qualifiers, closures, numeric conversions, matching and array methods dominate
remaining failures; the existing Rust-negative inventory also remains required.

## Sized-array defaults: local semantic repair

Rust's sized allocation ignored the modeled default expression. A defaulted
allocation now follows the unchanged C statement-expression loop: evaluate the
initial capacity, reevaluate the bound for every check, and evaluate/copy the
default for each element. Defaults are validated instead of silently ignored.
Temporary vector/index names avoid every source identifier in the model;
allowed numeric widening is explicit. Existing nondefault allocation lowering
and existing positive snapshots are unchanged.

The unchanged C fixture `test_sized_array_syntax.sn` and a new independent
oracle cover int/bool/string initialization, repeated bound/default effects,
zero elements, temporary-name collisions and widening. Together with the
source-identical reflection control they pass 27 exact C/Rust pairs across all
optimization/arithmetic modes. Full C suites pass unchanged counts; Rust
319 generation/171 negatives and all other suites pass without skips. Formatting
passes. Evidence: `sized-default-*`.

An initially selected neighboring Rust-only fixture `array_values.sn` does not
compile through C: `sn_array_remove` returns void but the fixture stores its
result. Preserve its nine failed C controls in
`sized-default-rejected-c-neighbor.json`; it receives no parity credit. The final
positive gate uses an independently verified C-valid neighboring source.
A first new snapshot was emitted with default O2 instead of the harness's O0;
only that new snapshot was corrected after all-mode paired execution checks.
No existing golden was refreshed. This local increment still requires composition
with native main and fresh tests before direct-main publication.


Native main `9683f657` now passes all six post-merge CI jobs; see
`pr143-main-ci-green.json`. Refresh the reflection/default increments against
this exact main, retaining native, receiver, reflection and default gates and
all seven successful-report artifact paths. Required combined validation is
319 Rust positives/171 negatives, native 8/17/1/4 and 504 differential pairs
(441 existing + 36 reflection + 27 defaults), plus complete C and other suites.
These local increments will be pushed directly to main only after that passes.


## Reflection/default increments ready for direct main publication

The full composition with native main `9683f657` passes complete C counts
1610/107/79/1141/58/224/11, Rust 319/171, native 8/17/1/4, closures 36/1,
concurrency 10/7/1 and 12 toolchain cases, all with zero skips. All 504
C/Rust differential pairs pass; raw bytes 63 executions, transport 6 executions,
Windows helper simulation and formatting pass. Evidence is
`rust-parity-evidence/feature-composition-*`. Every existing snapshot remains
unchanged; only the three new positives add generated/runtime oracles.

The increment is authorized for direct-main push with hosted runtime CI required
on all platforms. The PR queue is empty and native main's CI is fully green.
Remaining native/closure/array/language work and the two observed diagnostic
stream-order gaps still prevent claiming overall parity completion.


## Buffered C text output and diagnostics: verified local repair

Main `40365381` passes all six jobs in Compiler `37114404046` and Runtime
`37114404042`. Retained Windows JSON verifies all 504 positive pairs and every
source hash, including the raw fixtures; evidence is
`reflection-default-main-ci-green.json` and
`reflection-default-windows-verification.json`.

Rust output now uses the C runtime's buffered stdout/stderr with opaque stream
pointers and `fwrite`. Scalar formatting still uses the existing Rust formatter;
byte strings/characters retain raw bytes. Each newline write is assembled before
writing, and native bodies share the same stream buffers. A main guard flushes
on normal/early return; explicit exits use C's exit path. All new helper/type/
guard names are allocated against the complete source model. The C backend,
language inputs and C runtime oracles remain unchanged.

The macOS stream imports follow [Apple's stdio declarations](https://github.com/apple-oss-distributions/Libc/blob/main/include/_stdio.h).
Windows uses its CRT accessor and existing matched
[gnullvm/UCRT toolchain](https://doc.rust-lang.org/rustc/platform-support/windows-gnullvm.html).
The forced-Windows helper test simulates only the formatting/text adapter;
actual CRT import, buffering and text translation must pass hosted Windows CI.

All 372 pre-change generation/closure/concurrency snapshots are compared against
the exact baseline compiler/oracles and a deterministic normalization of only
reviewed I/O sites. Every other byte of normalized generated code agrees before
refreshing these snapshots. Evidence: `stdio-snapshot-review.json`, `.py` and
`.log`. The new hygiene positive exercises source names matching every stdio
helper, the guard type and its local binding. It adds a fresh source/runtime
oracle and snapshot.

The old `assert_heap_message_order.expected` reflected Rust's earlier merged
stream order. Nine unchanged C/Rust controls first prove C's actual
`bad/message/condition` order, separate raw streams and status 1; only then is
that obsolete Rust oracle corrected. No source or C oracle is edited. The new
stdio gate requires 30 paired cases (120 separate/merged target executions):
assert and heap-message order across all nine modes, hygiene across nine, and
thread panic at all three checked optimization levels. Before-change evidence
retains all 18 originally attempted cases. Unchecked division by zero is
undefined in C, as established in `rust-numeric-divzero-optimizer.md`; those
controls receive no parity credit and cannot supply a deterministic panic oracle.

Complete local C suites pass 1610/107/79/1141/58/224/11. Rust passes 320/171,
native 8/17/1/4, closures 36/1, concurrency 10/7/1 and toolchain 12, zero skips.
All 504 existing positive pairs, the 30 diagnostic/order cases, raw-byte 63
executions, native transport 6 executions and helper/format checks pass.
Evidence: `rust-parity-evidence/stdio-*`.

The broader unchanged C-positive corpus now passes 1058 integration and 206
exploratory Rust oracle checks: all three observed runtime output failures are
resolved, while 83 + 18 compilation/admission failures remain required work.
Those corpus runs use the existing harness's O0/debug policy and are not a
substitute for full mode/platform differential verification. An earlier corpus
attempt used a missing temporary compiler wrapper; its setup failure was
identified before any result was counted, and the corrected run uses a
project-local wrapper. This increment is ready for direct main publication;
verification of the resulting exact hosted revision is still required.

## Buffered output published and verified on all platforms

Direct-main `6b9a2a33` omitted ten reviewed concurrency snapshots from its
staged paths. They were present in the full passing local run. The omission was
caught immediately and repaired in `ec797a75`; every one of the 372 published
snapshot blobs was verified against the reviewed manifest before repair push.
The superseded Compiler run failed on those stale snapshots; its raw failure
log is retained as `stdio-superseded-compiler-ci.log`. The superseded runtime
run was cancelled after publishing the repair. No unrelated integration occurred
while the latest revision awaited CI.

Exact main `ec797a75a30c4ffafd3f16c47c49ac69de9a81f0` passes all six jobs in
[Compiler run 37116886462](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37116886462)
and [Runtime run 37116886604](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37116886604).
All eight downloaded reports on each platform pass: 504 positive pairs and 30
diagnostic/order cases per platform, 1,602 total. Every recorded source hash
matches this checkout, including Windows path normalization and unchanged raw
fixtures. Actual Windows CRT imports, raw/text output and buffered diagnostic
order pass; this is independent of the forced-Windows helper simulation.
Evidence: `stdio-main-ci-green.json` and `stdio-hosted-*-order.json`.
The live open PR queue is empty.

## Mixed integral binary and storage conversions: local repair

The original C-positive corpus exposed missing Rust conversions for summing byte
array elements into an int and storing a byte arithmetic result in an int. Four
unchanged exploratory sources first passed all 36 C compilation/runtime controls
while Rust failed all 36. A fifth unchanged integration source had the same
storage-conversion failure in the retained original corpus inventory.

Rust now converts mixed integral operands at the existing C boundary. Checked
arithmetic uses the modeled result helper type; checked strict comparisons use
the left operand helper type; raw comparisons and unchecked arithmetic use the
C expression types and integer promotions. Existing checked overflow/zero-divisor
helpers and wrapping operations remain active. Integral initializers explicitly
convert at storage, preserving checked byte wrapping and unchecked byte promotion.
Nested promoted byte operands remain promoted until the enclosing C conversion.
Each operand is evaluated once; tuple initialization avoids capturing source locals
named `__sn_left` or `__sn_right`. The C backend/shared frontend and all existing
source fixtures, runtime oracles and generated snapshots are unchanged.

The new independent positive oracle observes signed/unsigned width boundaries,
C's asymmetric checked helper selection, byte multiplication stored in an int,
nested promotions, high comparison bits, private-name collisions and operand
call counts. A nested unchecked expression initially narrowed too early; retained
before evidence shows C's 66,025 versus Rust's 1,001. The final unchanged new
source agrees in all nine optimization/arithmetic combinations.

`make test-rust-parity-mixed-integral` requires ten C-valid sources in 90 positive
pairs and three checked failure sources in nine diagnostic cases. The failures
require exact status 1, stdout/stderr bytes and merged diagnostic-before-buffered-
stdout order at O0/O1/O2. New source hashes are pinned for diagnostic cases;
unchecked overflow/division by zero is not used as a defined failure oracle.
An initial neighboring selection exposed existing C compilation failures for
floating modulo and checked byte division. These remain recorded in
`mixed-integral-rejected-c-neighbors.json`, receive no parity credit and are
replaced in the final gate by unchanged C-valid int32/uint32 integration tests.

Final local C counts are 1610/107/79/1141/58/224/11, all pass. Rust is 324
positives/171 negatives, native 8/17/1/4, closures 36/1, concurrency 10/7/1
and toolchain 12, all pass with zero skips. All 594 positive pairs and 39
diagnostic/order cases pass, plus raw bytes (63 executions), native transport
(6 executions), Windows helper simulation and formatting. Four new snapshots
are emitted only after C controls; no prior snapshot is refreshed.

An initial overlapping C/Rust suite launch produced three missing-executable
errors: the Rust runner's startup cleanup removed the C runner's active shared
`sn_test_*` directory. Its log starts with that cleanup, and both runners' cleanup
functions remove all matching directories without an active-process check. The
failed C log is retained; the corrected complete C run with `--no-cleanup` passes.
This is a harness setup failure, not a language result or parity credit.

The broader original corpus source hashes all remain exact (1,365 sources).
Rust now passes 1,059 integration and 210 exploratory oracle checks, with
82 + 14 admission/compilation gaps and no observed admitted-program runtime
failures or skips. These O0/debug corpus checks are not full platform/mode parity.
The 96 remaining compilation gaps, rejected feature inventory and broader
lifetime/native/language coverage still prevent completing the goal.
Evidence: `rust-parity-evidence/mixed-integral-*`. The final gate is required in
all three platform runtime CI jobs and retains both positive and diagnostic JSON
reports. Hosted verification of the new direct-main revision remains required.


## Mixed integral increment published and verified on all platforms

Exact direct-main `bfc0cb99401c275ce5ac693f21def057e28d64a2` passes all six jobs in
[Compiler run 37118541456](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37118541456)
and [Runtime run 37118541472](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37118541472).
All ten downloaded reports pass on every platform: 594 positive pairs and 39
diagnostic/order cases each, 1,899 total. Every source hash matches this checkout,
including the new diagnostic fixtures and raw fixtures. Windows raw diagnostic
statuses and buffered order pass as well as the positive numeric conversions.
Evidence: `mixed-integral-main-ci-green.json`. The PR queue remains empty.

The next unchanged corpus source, `test_interop_types.sn`, passes nine C controls
but fails Rust compilation at its float initializer: the emitted expression has
type f64 and storage has type f32. C's emitted `sn_mul_double` result converts
at float storage. Preserve that boundary when implementing the next increment;
do not silently change arithmetic precision. The remaining 96 corpus failures
are all admission/compilation failures in the recorded O0/debug policy.


## Float/double conversion increment: local verification, 2026-10-03

Starting from main `bfc0cb99`, the unchanged `test_interop_types.sn` passes all
nine C controls but Rust rejects its `float fmul = fa * 2.0` initializer. C emits
`float fmul = sn_mul_double(fa, 2.0)`: double arithmetic precedes conversion to
float storage. Target-local Rust lowering now preserves that order instead of
rounding operands early. No shared checker, C renderer/runtime or default-target
selection changes are included.

Explicit floating boundaries cover declarations, assignment results, fields,
struct literals/defaults, array elements/literals/defaults, ordinary and resolved
method arguments, scalar native arguments, function/closure value arguments,
and expression/block returns. Existing ownership and reference annotations are
retained; borrowed arguments are not replaced with temporary cast values. Mixed
float/double `+=`, `-=`, `*=`, `/=` compute in double before converting to the
place's width. Local/direct-field, by-value/as-ref parameter, iterator binding,
mutable scalar snapshot and synchronized local/global cell observations are
included. Existing same-parameter RHS mutation/ref-forwarding guards remain.
Thread-spawn argument conversion and additional floating ownership shapes are
not established by this increment.

Five old rejection sources are moved unchanged to positive generation tests:
`by_value_parameter_direct_assignment_mixed_float_double`,
`floating_as_ref_parameter_mixed_type_mutation`,
`floating_compound_mixed_float_double`,
`iterator_protocol_mixed_float_double_mutation`, and
`closure_values_mixed_argument`. Each has nine successful C/Rust controls. Empty
output in three promotions proves admission only; adjacent precision and caller
mutation observations provide behavior evidence. One remaining by-value RHS
hazard expectation now identifies its actual safety guard. Floating/integer
rejections retain unchanged sources and accurate diagnostics; none earns parity
credit.

New independently checked output oracles observe 16,777,217 narrowing to
16,777,216, single evaluation counts and storage/result conversions. The
precision adversary starts at float 16,777,216 and adds double 1.00000001:
C and Rust store 16,777,218, while prematurely narrowing the RHS would produce
16,777,216. Snapshot closure reset behavior follows C: the second invocation
does not accumulate the first invocation's local mutation. A native scalar
fixture observes both ABI widths and one call per argument. The synchronized
fixture observes global/local conversions and all four compound operators in
both operand-width directions. The Linux gate records the established explicit
`-lpthread -lm -latomic` override; compiler configuration is unchanged.

`make test-rust-parity-float-conversions` requires thirteen sources across
O0/O1/O2 and default/checked/unchecked arithmetic: 117 positive pairs. Together
with existing gates this requires 711 positive pairs plus 39 diagnostic/order
cases per platform. Full local C counts are 1610/107/79/1141/58/224/11. Rust
counts are generation 332, negative 166, native 8/18/1/4, closures 36/1,
concurrency 10/7/1, and toolchain 12, all with zero failures or skips. Existing
Rust snapshots remain unchanged; only eight new positive snapshots are added.

The unchanged original 1,365-source corpus passes 1,060 integration and 210
exploratory executions; 81 integration and 14 exploratory sources still fail
Rust compilation, with no admitted runtime failures in this O0 sample. These
95 gaps and unverified mode/platform/feature families remain required work.
Float array search has a separate C bytewise needle-representation contract;
its mixed-width guard is retained pending observable representation evidence.
This increment does not establish full floating or whole-language parity.

Evidence: `rust-parity-evidence/float-conversions-*`. Hosted results for this
increment must be tied to its exact published revision; preceding mixed-integer
main CI evidence is included in this commit.
