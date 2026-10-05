# Rust backend completion ledger

Goal: verified feature and behavioral parity with C on integrated main, preserving
the established language contract and C as the default target. Rejections are
remaining implementation work, not parity. This ledger supersedes historical
head counts, not the language specification or the historical evidence itself.

## Native C callable definitions: local acceptance, 2026-10-05

This increment starts from published main
`ea1098d21a35df8dc30a6ac1bfe5826f238c0cc1`. That revision's compiler jobs pass
on Linux/macOS/Windows; Linux/macOS runtime artifacts are independently verified.
Its Windows runtime job remains active. Local acceptance for this increment is
complete; exact integrated-revision hosted acceptance and the full parity goal
remain required.

The Rust-native projection now preserves the reachable C lambda, thread and
resolved function-reference definitions. Selection follows actual emitted
nested definitions to a fixed point, rather than relying solely on constructor
snapshots. Imported C modules containing ordinary helpers are emitted. Shared
helpers remain available to Rust and C; helpers used exclusively by C stay out
of Rust validation and emission. Rust callable metadata is selected from Rust
roots, including ordinary methods and globals. Resolved function aliases are
admitted, while parameter/result types that cross the bridge retain their
independent ABI validation.

These programs use the established C closure/context representation. This does
not establish support for passing Rust callback values through arbitrary
foreign C function-pointer APIs. Such SDK/native transport remains required.

The unchanged callback typedef, comparator, callback interop and pointer interop
edge-case originals now pass. An unchanged former native closure-body rejection
is promoted into the runtime suite. New controls exercise mixed Rust/C nested
and owned string/array captures, imported ordinary and native helpers, function
references, repeated/grouped C helper threads, and deferred C global closure
initialization, replacement and explicit clearing. Global implicit process-exit
cleanup remains a separate lifetime requirement.

All complete C/Rust suites pass with zero failures/skips. Historical C/model/
Rust emission expectations are unchanged. Rust native positives increase to
**52**, with **four** remaining admission-error fixtures. All **40 reports /
3468 cases** pass; the previous **39 / 3405** are independently verified again
with the current compiler. CI runs the new **63-case** gate on all three
platforms. All **126 instrumented target executions** have independently frozen
output, zero exit statuses and empty sanitizer stderr. Actual compile commands
contain both sanitizer flags; every generated Rust-side C object contains ASAN
and UBSAN sites. Exact compilation paths resolve an initial audit mismatch
caused by concurrently selecting a different build directory.

On the previous main compiler, the exact final seven sources produce **63 C
successes and 63 Rust compilation failures**. All **1365 original source hashes**
remain unchanged. Broad original coverage is **1126 integration + 221
exploratory = 1347 / 1365 (98.7%)**, with **18 compilation gaps**, zero runtime
failures and zero skips. The newer integration control is counted separately.
Raw output/argv, native printing/flush ordering, diagnostics and Windows helper
logic also pass. Full completion still requires interface/iterator and
serialization support, callback/native SDK boundaries, broader native record
and array families, remaining ownership/lifetime edges, and hosted acceptance.

Evidence: [validation and all 18 gaps](rust-parity-evidence/native-callables-validation.json),
[63 frozen comparisons](rust-parity-evidence/native-callables-oracles.json),
[previous 3405-case preservation](rust-parity-evidence/native-callables-preservation.json),
[unchanged original sources](rust-parity-evidence/native-callables-source-preservation.json),
[126 instrumented executions](rust-parity-evidence/native-callables-asan.json),
[actual C compiler commands](rust-parity-evidence/native-callables-asan-compile-commands.jsonl),
[exact final-source baseline](rust-parity-evidence/native-callables-before.json),
[full C suite](rust-parity-evidence/native-callables-full-c.log),
[full Rust suite](rust-parity-evidence/native-callables-full-rust.log).

## Native value records with owned strings: local acceptance, 2026-10-05

Published directly to main as
`ea1098d21a35df8dc30a6ac1bfe5826f238c0cc1` after complete local acceptance.
Its compiler CI passes on Linux/macOS/Windows. Linux and macOS runtime artifacts
are independently verified at 39 reports / 3405 cases each; Windows runtime CI
remains active. The base `b8ad9a55` now has complete exact-revision three-platform
hosted acceptance, recorded below. The full parity goal remains active.

Native-facing value records containing strings now keep persistent C-compatible
source storage. A transparent owning field stores the C allocation itself;
native field replacement, NULL writes and later Rust reads/drop observe the
same pointer. Copies use C allocation helpers and native by-value transfer gives
C ownership of the wire fields. Nested scalar/string records preserve their
layouts. C support checks actual generated-header `sizeof`, alignment and
`offsetof` against both Rust source and wire storage before transport. It does
not use the model's synthetic reference-header offsets as ABI evidence.

Direct native string arguments from these fields borrow the actual allocation.
Ordinary string arguments retain the existing native string pool. Rust source
field writes, method/compound mutations, zero/default storage, record/array
copies, native returns and repeated/grouped thread joins are covered. Exclusively
owned C strings may move between threads; this adds no `Sync` implementation.

All complete C and Rust suites pass with zero failures/skips. Historical C/model/
Rust emission expectations remain unchanged. Rust-native positives grow from
44 to **49**: three new controls plus two unchanged former admission-error
sources moved into the positive suite. Their C-verified runtime oracles replace
the obsolete rejection expectations; the original diagnostics remain retained.
The five remaining native admission-error fixtures pass. All **39 parity
reports / 3405 cases**, including the previous **38 / 3351** independently
verified again with the current compiler, pass. Mandatory CI runs the new
**54-case** gate and the 49 positive/five negative native fixtures on all three
platforms.

All **108 instrumented target executions** (six sources, nine modes, both
backends) return zero with empty sanitizer stderr. ASAN symbols are verified in
every generated C object. The allocation-only promoted constructor has no
operation requiring a UBSAN check; its generated C source and actual compile
command with both sanitizers are retained, and its ABI support object contains
UBSAN sites. No skipped executions or resource failures receive credit.

On the previous compiler the exact final six sources yield **54 successful C
executions and 54 Rust compilation failures**. All **1365 original source
hashes** remain unchanged. Original coverage advances by the unchanged
`test_struct_as_ref.sn` to **1122 integration + 221 exploratory = 1343 / 1365
(98.4%)**, with **22 compilation gaps**, no original runtime failures and no
skips. The newer integration control is counted separately. Raw output/argv,
text/native-flush ordering, diagnostic transport and Windows helper logic pass;
the host helper check is not Windows ABI evidence.

The remaining work still includes interfaces/iterators, callbacks and SDK
interop, serialization, pointer/native record families, user-copy ownership,
array/thread/global lifetime edges and complete hosted acceptance. In
particular, owned string fields do not establish parity for native records
containing arrays or pointers, or indirect native string field loans.

Evidence: [validation and all 22 original gaps](rust-parity-evidence/native-managed-records-validation.json),
[54 frozen comparisons](rust-parity-evidence/native-managed-records-oracles.json),
[previous 3351-case preservation](rust-parity-evidence/native-managed-records-preservation.json),
[unchanged original sources](rust-parity-evidence/native-managed-records-source-preservation.json),
[108 sanitizer executions](rust-parity-evidence/native-managed-records-asan.json),
[exact final-source baseline](rust-parity-evidence/native-managed-records-before.json),
[full C suite](rust-parity-evidence/native-managed-records-full-c.log),
[full Rust suite](rust-parity-evidence/native-managed-records-full-rust.log).

## Global native owners and joined results: local acceptance, 2026-10-04

Published directly to main as
`b8ad9a55c5ac49c7f21451e434ad5a0cb664bf76` after complete local acceptance.
All six exact-revision compiler/runtime CI jobs pass on Linux/macOS/Windows.
All three runtime artifacts are independently verified at **38 reports / 3351
cases** each; hosted suite logs also prove zero failures/skips and the complete
C/Rust suite counts. Evidence: [main acceptance](rust-parity-evidence/native-globals-main-ci-green.json)
and [hosted verification](rust-parity-evidence/native-globals-hosted-verification.log).
The full parity goal remains active.

Canonical native-reference owners now move through process-wide globals and
threads. The Rust-private C header uses atomic reference credits; generated C
asserts the actual field type and atomic/C-int size and alignment before Rust
claims `Send`. Owners and uniquely owned array headers gain `Send`, with no
`Sync` implementation. Native code retains its C/foreign threading preconditions.
Default threaded array parameters share the selected global header without
copy callbacks. Global replacement selects a new owner; aliases retain their
selected header. Cell reads acquire ownership once, borrowed pointer views drop
mutex guards before subsequent arguments, and closure borrows capture the whole
native owner. Empty inferred call literals retain C's actual untyped metadata.
Native scalar field writes update the actual C object rather than Rust mirrors.

The unchanged module-array cleanup original passes all nine modes. New controls
verify duplicate global borrows, native owner moves, zero-copy array thread
transport, alias mutation and growth, one copy callback per explicitly copied
element, global replacement, lexical shadowing and threaded scalar reference
aliases. Repeated/grouped joins verify native result defaults and zero resources.
The baseline C compiler omitted cleanup of joined reference results and old owners
on overwrite: unchanged final sources leak one and three allocations under
LeakSanitizer. The C cleanup paths are repaired explicitly; the positive oracle
requires zero surviving resources. A separate C global-array initializer aliases
both owners and aborts at cleanup; its exact source and failure are retained and
receive no parity credit.

The mandatory gate checks **36 frozen C/Rust comparisons**. All **38 reports /
3351 cases**, including the previous **37 / 3315**, pass with one current
compiler; production and staged template hashes are recorded because template
changes alone do not change the compiler binary. **351 instrumented executions**
pass ASAN, UBSAN and leak detection: 342 native C/Rust executions and nine plain
global-array C controls. Complete C suites pass **1610 / 107 / 79 / 1142 / 58 /
224 / 11**; complete Rust suites pass **491 / 133 / 8 / 44 / 1 / 7 / 36 / 1 / 10 /
7 / 1 / 12**, with no failures or skips. Historical emission oracles and all
1365 original source hashes remain unchanged. Raw bytes/argv, text/native flush
ordering and Windows output helper controls pass locally. CI requires 44 native
fixtures and the new gate on all three platforms.

Original coverage is **1121 integration + 221 exploratory = 1342 / 1365 (98.3%)**,
with **23 compilation gaps**, no runtime failures and no skips. The newer
integration ownership control passes separately. Remaining originals cover
interfaces/iterators, callbacks/SDK interop, serialization, managed native value
records, pointer slices, array qualifiers/closures and thread record results.
Broader native array families, constructor/method/closure lifetimes, implicit
thread-result disposal, threaded temporary arrays, nonthreaded global reference
aliases and retained C ownership failures remain required full-goal work.

Evidence: [local validation and all 23 gaps](rust-parity-evidence/native-globals-validation.json),
[36 frozen comparisons](rust-parity-evidence/native-globals-oracles.json),
[previous 3315-case preservation](rust-parity-evidence/native-globals-preservation.json),
[351 sanitizer executions](rust-parity-evidence/native-globals-asan.json),
[exact final-source baseline](rust-parity-evidence/native-globals-before.json),
[C joined-result leak baseline](rust-parity-evidence/native-globals-c-joined-leak-before.json),
[excluded C initializer](rust-parity-evidence/native-globals-initializer-excluded.json),
[full C suite](rust-parity-evidence/native-globals-full-c.log),
[full Rust suite](rust-parity-evidence/native-globals-full-rust.log).

## Canonical native reference arrays: combined acceptance, 2026-10-04

This increment starts from published main
`3fa0bc91d8c75923e34fe331bc511db10890edf4`, whose six compiler/runtime jobs and
Linux/macOS/Windows **36 reports / 3288 cases** are independently verified below.
The implementation is published at
`c4cfbf78fa380198590c8fb1f365219f33cc5d4a`. All six exact-revision
compiler/runtime jobs pass. Linux/macOS/Windows artifacts independently verify
**37 reports / 3315 cases** per platform and the complete hosted suite counts,
including 41 native-extra fixtures. The full parity goal remains active.
Evidence: [hosted acceptance](rust-parity-evidence/native-arrays-main-ci-green.json),
[verification log](rust-parity-evidence/native-arrays-hosted-verification.log).

Rust arrays of canonical native reference handles now keep the actual C
`SnArray` header, data and element ownership callbacks. Native default parameters
borrow that header, including duplicate aliases, so native mutation and
reallocation remain visible. Array construction, copies, slices, concatenation
and the tested mutation methods use canonical C helpers; owned results adopt the
C header and cleanup uses its callbacks. Borrowed record arguments snapshot their pointer before later
argument effects can reallocate array data. Foreach caches the initial C length,
reloads current data and borrows each element; explicit owning aliases retain an
owner, and `continue` advances correctly. Native array declaration temporaries
live through their enclosing scope, matching C's visible resource counts.

Two unchanged originals now pass across all nine optimization/arithmetic modes:
array-literal argument cleanup and self/method forwarding after native training
updates. An additional control verifies header identity, native growth, ordinary
default-array mutation, metadata copies, clear/slice, borrowed iteration,
declaration temporaries, nil transport and zero resources. The ownership review
also found that native-array `concat` still returned a Rust `Vec`. Its exact
unchanged C-valid probe now passes all nine modes; the positive fixture includes
concat/reverse/insert/remove/pop and verifies both original inputs remain intact.
The mandatory gate checks **27 frozen C/Rust cases**, and native-extra CI now
requires **41 fixtures**.

All **37 reports / 3315 cases**, including **117 existing native-handle cases**
and the preceding **35 reports / 3171 cases**, pass with one compiler.
**288 instrumented target executions** pass ASAN, UBSAN and leak detection;
executable and generated C-object instrumentation is verified. Raw bytes/argv,
text/native flush ordering and Windows output helper controls pass locally.
Complete C suites pass **1610 / 107 / 79 / 1142 / 58 / 224 / 11**; complete Rust
suites pass **491 / 133 / 8 / 41 / 1 / 7 / 36 / 1 / 10 / 7 / 1 / 12**, with zero
failures or skips.

All **1365 original source hashes** and historical C/model/Rust emission oracles
remain unchanged. Original-program coverage is **1120 integration + 221
exploratory = 1341 / 1365 (98.2%)**, leaving **24 compilation gaps**, no runtime
failures and no skips. The newer integration ownership control passes separately
and receives no credit in the original denominator. The retained pre-repair array
temporary control observes C's eight surviving resources versus Rust's seven;
the implementation corrects the lifetime. An exploratory `len(nil)` control
faults in C's `sn_array_length` under ASAN and receives no parity credit. Its raw
source and diagnostic are retained, with the missing historical helper snapshot
explicitly documented. The positive fixture uses a nil-safe native count helper;
historical sources and expectations are preserved.

Global/thread native handle storage still blocks the remaining module-array
original. Managed native value-record parameters, callbacks/SDK interop, other
native array element families, qualifiers, sized/nested arrays, field stores,
constructors and broader method ownership/evaluation composition remain required.
Interfaces/iterators, serialization/copy hooks, broader closure/native/thread
lifetimes and retained C ownership failures also remain in the full goal.
Coverage measures programs, not remaining engineering effort or completion.

Evidence: [local validation and remaining 24 gaps](rust-parity-evidence/native-arrays-validation.json),
[27 frozen array comparisons](rust-parity-evidence/native-arrays-oracles.json),
[117 native-handle preservation comparisons](rust-parity-evidence/native-arrays-handles-preservation.json),
[288 sanitizer executions](rust-parity-evidence/native-arrays-asan.json),
[independent preservation verification](rust-parity-evidence/native-arrays-local-verification.log),
[unchanged originals before implementation](rust-parity-evidence/native-arrays-originals-before.json),
[pre-repair temporary lifetime](rust-parity-evidence/native-arrays-temporary-before.json),
[pre-repair concat compilation](rust-parity-evidence/native-arrays-methods-before.json),
[unchanged concat probe after repair](rust-parity-evidence/native-arrays-methods-after.json),
[excluded nil-length diagnostic](rust-parity-evidence/native-arrays-nil-length-excluded.json),
[full C suite](rust-parity-evidence/native-arrays-full-c.log),
[full Rust suite](rust-parity-evidence/native-arrays-full-rust.log).

## Canonical native reference handles: combined acceptance, 2026-10-04

This increment starts from published main
`5e48abc87d5a48d83731fafc4b9eb6f954550016`. All six preceding compiler/runtime
jobs pass; Linux/macOS/Windows artifacts independently verify 35 reports /
3171 cases in the [preceding hosted evidence](rust-parity-evidence/native-reference-borrow-main-ci-green.json).
The implementation is published at
`3fa0bc91d8c75923e34fe331bc511db10890edf4`. All six exact-revision compiler/runtime
jobs pass. Retained Linux/macOS/Windows artifacts independently verify all
**36 reports / 3288 cases** per platform, including the 117 native handle cases,
and hosted complete C/Rust suite counts have zero failures or skips in the
[hosted acceptance evidence](rust-parity-evidence/native-handles-main-ci-green.json).

Rust native reference records now keep the canonical C allocation behind an
opaque handle. C helpers compiled against the actual generated record definition
provide retain/release and scalar field reads, avoiding Rust layout assumptions
and detached copies. Default native arguments and method receivers borrow without
an extra C retain. Returned borrowed pointers acquire one owner; already retained,
fresh and nil results preserve their existing ownership. Shared/nested record
fields and indexed elements expose the same visible C counts. Method-only native
records, default native symbols, receiver-name hygiene and explicit `using`
disposal are covered. Native declaration temporaries live through their enclosing
scope, matching C's observable reference counts. No thread-safety claim is added.

The audit also found and repaired a C native method result lifetime failure:
borrow inference omitted the receiver, leaving two owners with a reference count
of one. The preserved pre-repair control produces an ASAN use-after-free. Receiver
and parameter snapshots now follow the actual flattened call operands, including
static methods, with once-only evaluation. Historical C/model/Rust emission
expectations remain unchanged. C is still the default target.

Nine unchanged originals now pass with both targets in all nine optimization /
arithmetic modes. Four ownership controls add borrowed/retained/fresh/nil results,
shared fields, temporaries, nested calls, indexed side effects, exact reference
counts and zero surviving resources. The new mandatory gate independently verifies
**117 frozen C/Rust cases**. **234 instrumented target executions** pass ASAN,
UBSAN and leak detection; instrumentation is checked in executables and generated
C objects. All **36 reports / 3288 cases**, including the preceding 35 preservation
gates, pass with the same compiler. Raw bytes/argv, text/native flush ordering and
Windows helper controls pass.

Complete C suites pass **1610 / 107 / 79 / 1142 / 58 / 224 / 11** checks;
complete Rust suites pass **491 / 133 / 8 / 40 / 1 / 7 / 36 / 1 / 10 / 7 / 1 /
12**, with zero failures or skips. The three new native controls are explicitly
registered in the native-extra suite; hosted CI requires all 40 fixtures.
All **1365 original sources** and historical emission oracles are unchanged.
Original-program coverage increases to **1118 integration + 221 exploratory =
1339 / 1365 (98.1%)**, leaving **26 compilation gaps**, no runtime failures and
no skips. The newer integration ownership control passes separately and is not
added to the original denominator. Coverage measures programs, not remaining
engineering effort or full feature completion.

Native reference-array ABI and global handle storage still block three native
originals. Managed value-record native parameters, callbacks/SDK interop, broader
native method/constructor/field ownership composition, interfaces/iterators,
serialization/copy hooks, broader closure lifetime contracts and retained C
ownership failures remain in the full goal. Unsupported forms retain explicit
guards and receive no parity credit. The open PR queue is empty at publication.

Evidence: [local validation and remaining 26 gaps](rust-parity-evidence/native-handles-validation.json),
[117 frozen execution comparisons](rust-parity-evidence/native-handles-oracles.json),
[234 sanitizer controls](rust-parity-evidence/native-handles-asan.json),
[unchanged originals before implementation](rust-parity-evidence/native-handles-originals-before.json),
[pre-repair C method failure](rust-parity-evidence/native-method-receiver-before.json),
[borrowed-field prototype discrepancy](rust-parity-evidence/native-handle-field-borrow-before.json),
[independent preservation verification](rust-parity-evidence/native-handles-local-verification.log),
[full C suite](rust-parity-evidence/native-handles-full-c.log),
[full Rust suite](rust-parity-evidence/native-handles-full-rust.log),
[integration corpus](rust-parity-evidence/native-handles-corpus-integration.log) and
[exploratory corpus](rust-parity-evidence/native-handles-corpus-explore.log).

## Native reference-return ownership: C prerequisite repair, 2026-10-04

This increment starts from published main
`e8eba717d3915f80e602eb5643d8df7f9fca2fc3`. All six preceding compiler/runtime
jobs pass and all Linux/macOS/Windows artifacts independently verify 35 reports /
3171 cases in the [hosted evidence](rust-parity-evidence/owned-closure-records-main-ci-green.json).
The repair is published at `5e48abc87d5a48d83731fafc4b9eb6f954550016`.
All six exact-revision compiler/runtime jobs pass, and all three platforms
independently verify 35 reports / 3171 cases in the
[hosted acceptance evidence](rust-parity-evidence/native-reference-borrow-main-ci-green.json).

Auditing native reference-handle transport exposed a language-valid C lifetime
failure: `return token_borrow(token_create(20))` generated two factory calls.
The ownership check snapshotted one allocation but passed a different flattened
argument to native code. It failed to retain the borrowed result before the
argument's cleanup, causing a sanitizer-confirmed use-after-free. Native pointer
argument expressions now evaluate once: checks follow actual argument positions,
use flattened temporaries and pass the snapshot to the native call. Nested
inferred calls receive owning temporary cleanup. Already retained results are
not retained again; distinct fresh results and nil keep their existing contract.
Compiler-local C snapshot names remain separate from valid source bindings.
The matching Rust model projection shares the normalized argument rather than
regenerating its AST. Native Rust admission is unchanged, and C remains default.

A new integration fixture checks borrowed/retained/fresh/nil returns, temporary
and nested arguments, self-assignment, repeated aliases, an index with a side
effect, a checked record in a later parameter position, source-name hygiene,
exact visible reference counts, exactly 14 allocations and zero surviving
resources. It passes all nine optimization/arithmetic modes with verified ASAN
instrumentation, UBSAN and leak detection. Twelve unchanged native-resource
originals also pass instrumented C controls: **21 clean controls** in total.

Complete C suites pass **1610 / 107 / 79 / 1142 / 58 / 224 / 11** checks;
complete Rust suites pass **491 / 133 / 8 / 37 / 1 / 7 / 36 / 1 / 10 / 7 / 1 /
12** with no failures or skips. All **35 reports / 3171 preservation cases**
pass and are independently checked against existing frozen oracles, complete
mode coverage and current source/compiler hashes. Raw bytes/argv, text/native
flush ordering and Windows helper controls pass. All **1365 original sources**
and every historical emission oracle are unchanged.

Two temporary verification failures were resolved without changing expectations:
the parallel test runners deleted each other's temporary directories, repaired
by an isolated project-local TMPDIR; a newline accidentally added at a C partial
EOF changed 75 source snapshots, repaired by restoring the original formatting.
Fresh full C/Rust suites and sanitizer controls pass after the formatting repair.

The original Rust coverage remains **1330 / 1365 (97.4%)**, with **35 original
compilation gaps**. This C prerequisite and its new C-only control receive no
new Rust feature or portable parity credit. The twelve native-reference
originals still require implementation; a separate manual Rust/C owner experiment
supports canonical opaque C handles with C retain/release and live, once-evaluated
borrow snapshots, but is not compiler acceptance. Native/SDK callbacks, general
native methods/ownership, interfaces/iterators, serialization/copy hooks, broader
closure contracts and the retained C ownership failures remain in the full goal.

Evidence: [local validation](rust-parity-evidence/native-reference-borrow-validation.json),
[pre-repair sanitizer failure](rust-parity-evidence/native-reference-borrow-before.json),
[exact failing source](rust-parity-evidence/native-reference-borrow-before.sn.raw),
[exact native helper](rust-parity-evidence/native-reference-borrow-before-helper.c.raw),
[21 clean sanitizer controls](rust-parity-evidence/native-reference-borrow-asan.json),
[independent preservation verification](rust-parity-evidence/native-reference-borrow-local-verification.log),
[full C suite](rust-parity-evidence/native-reference-borrow-full-c.log) and
[full Rust suite](rust-parity-evidence/native-reference-borrow-full-rust.log).

## Owned and reference record closures: combined validation, 2026-10-04

This increment is based on exact verified main
`ed9e8c9c7767c17a1581b2ede317791338b06790`. All six preceding
compiler/runtime jobs pass; the retained Linux/macOS/Windows artifacts
independently verify 34 reports / 2982 cases per platform in the
[preceding hosted evidence](rust-parity-evidence/scalar-parameters-main-ci-green.json).
All six compiler/runtime CI jobs for published main
`e8eba717d3915f80e602eb5643d8df7f9fca2fc3` pass. The retained Linux/macOS/Windows
artifacts independently verify all 35 reports / 3171 cases per platform in the
[hosted acceptance evidence](rust-parity-evidence/owned-closure-records-main-ci-green.json).

Default heap-owning record callable parameters preserve C's caller borrow and
alias behavior through shared field owners. Repeated parameters observe each
other's writes, including whole-record reassignment. Ordinary reference records
retain their identity through callable parameters, captures and methods.
Consumed scalar field assignments return their stored value. Imported function
fields with owned record signatures now pass the unchanged router original.

Captured value records keep an owned environment and create a shallow body view
for each invocation: scalar changes reset between calls, while array-element
updates persist. A deep Clone of every field per call would silently change C's
behavior. The target-local snapshot helper copies scalar slots, shares array
owners and recursively snapshots nested value records. Nested field projection
shares the actual record owners without keeping a guard across source callbacks.
Both ordinary borrowing calls and direct closure-body mutations are covered.
C generation, runtime and default-target selection are unchanged.

The unchanged original corpus passes **1109 integration + 221 exploratory
programs**: **1330 / 1365 (97.4%) original-program coverage**. Five earlier gaps
close, leaving **35 compilation gaps**, zero runtime failures and no skips.
This percentage measures coverage, not remaining engineering effort. All 1365
original source hashes remain unchanged. One former reference-capture rejection
moves byte-for-byte into positive coverage; thirteen new generation fixtures
exercise mutation, aliases, snapshots, assignment results and nested lifetimes.
Four of 540 tracked historical Rust emission snapshots change; 536 are unchanged.
The historical foundation's Rust output remains identical in all nine modes;
its failing C run receives no portable parity credit.

The first prototype corpus sweep exposed a temporary regression in a previously
passing nested reference-field original: the reference field wrapper lacked
`map_read`. The wrapper now supports the same nil-checked projection operation;
the unchanged original is included in the mandatory **189-case gate**. The
failed sweep is retained. All 21 gate sources and two earlier nested-record
controls pass instrumented C ASAN with leak detection: **23 clean controls**.
All frozen oracles pass across nine optimization/arithmetic modes.

Complete C suites pass **1610 / 107 / 79 / 1141 / 58 / 224 / 11** checks.
Complete Rust generation/negative suites pass **491 / 133**; native
8 / 37 / 1 / 7, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12
also pass, with zero failures or skips. Raw-byte/argv, text/flush ordering and
Windows helper controls pass. All preceding gates remain green: the combined
compiler passes **35 reports / 3171 cases**. These local results do not replace
required Linux/macOS/Windows hosted verification.

C failure probes remain explicit ownership/verification work. Replacing a
captured string, copying the probed callable handle, nesting a borrowed-record
capture, and returning a borrowed owned-record parameter expose C use-after-free
or stack-overflow defects. The historical foundation also crashes in C callable
identity conversion. Their exact sources and available sanitizer evidence are
retained separately; neither failed execution nor a changed oracle is counted
as parity. Their language ownership contracts remain part of the full goal.
Qualified/scalar closure references, broader callable lifetime and method
composition, native/SDK callbacks, interfaces/iterators, serialization and
general copy hooks also remain required work.

Evidence: [combined local validation](rust-parity-evidence/owned-closure-records-validation.json),
[189 frozen pairs](rust-parity-evidence/owned-closure-records-pairs.json),
[baseline four-original controls](rust-parity-evidence/owned-closure-records-before.json),
[23 instrumented C controls](rust-parity-evidence/owned-closure-records-asan.json),
[source/snapshot preservation](rust-parity-evidence/owned-closure-records-preservation.json),
[remaining original diagnostics](rust-parity-evidence/owned-closure-records-gap-diagnostics.json),
[temporary failed sweep](rust-parity-evidence/owned-closure-records-corpus-integration-pre-reference-projection-fix.log),
[excluded C probes](rust-parity-evidence/owned-closure-records-excluded-c-probes.json) and
[historical foundation control](rust-parity-evidence/owned-closure-records-foundation-control.json).

## Character and scalar parameters: combined validation, 2026-10-04

This increment is based on verified main
`b857f00ebfa5d489a99bda333891c2f769282ae7`. All six compiler/runtime CI jobs
pass, and the retained Linux/macOS/Windows artifacts independently verify
33 reports / 2856 cases per platform. The
[hosted acceptance evidence](rust-parity-evidence/reference-records-main-ci-green.json)
closes the preceding increment's pending verification.

Character reference parameters now support assignment, forwarding and postfix
mutation; by-value character parameters preserve caller isolation. Their
boundary fixture checks all 256 byte identities without assuming signed C
`char`. Character references also retain mutation and return values across
thread synchronization. Repeated scalar reference arguments use shared cells
whose ownership propagates through forwarding functions, preserving aliases
for all ten scalar types while distinct arguments remain independent.

Integer storage boundaries retain the C arithmetic promotion before narrowing
back to byte storage. Consumed scalar assignment expressions return the stored
value. Maximum-width literal controls prevent Rust literal-inference overflow.
The original-corpus sweep exposed a temporary byte-array insertion regression:
the shared function signature retains element/index parameter order while
lowered arguments are index/element. Conversions now follow the lowered order;
the unchanged original insertion program is a permanent all-mode control.
All nine temporary failures are retained rather than hidden by snapshot edits.

The unchanged original corpus passes **1104 integration + 221 exploratory
programs**, leaving **40 compilation gaps**, zero runtime failures and no skips:
**1325 / 1365 (97.1%) original-program coverage**. This percentage measures
coverage rather than remaining engineering effort. All 1365 source hashes are
unchanged. Three C-valid character rejection sources move byte-for-byte into
positive coverage. Two of 520 historical snapshots receive explicit byte
conversion casts; the other 518 are unchanged.

A mandatory **126-case gate** covers fourteen frozen independent output
oracles in all nine optimization/arithmetic modes. All fourteen C controls
pass instrumented ASAN with leak detection. Complete C and Rust suites pass;
Rust generation/negative counts are 477 / 134, native tagged/extra/origin/negative
8 / 37 / 1 / 7, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12.
Raw-byte/argv, native flush ordering and Windows helper logic controls pass.
The combined compiler passes all **34 reports / 2982 cases**, including every
preceding gate.
All six compiler/runtime jobs for exact main
`ed9e8c9c7767c17a1581b2ede317791338b06790` pass. The retained
Linux/macOS/Windows artifacts independently verify all **34 reports / 2982
cases** per platform in the
[hosted acceptance evidence](rust-parity-evidence/scalar-parameters-main-ci-green.json).

A shared formatter repair keeps complete character literal spellings together,
including hexadecimal escapes. The 256-value fixture is byte-identical after
formatting and passes the formatter's idempotence check. C code generation,
runtime and default-target selection are unchanged. Direct scalar field/index
reference arguments rejected by the shared frontend receive no parity credit;
field mutation in this gate passes the whole owning record. Dynamic closure,
method/global reference aliasing and the remaining native/SDK, callback,
closure lifetime, interface/iterator, serialization and general copy-hook gaps
remain part of the full goal.

Evidence: [combined local validation](rust-parity-evidence/scalar-parameters-validation.json),
[126 independent pairs](rust-parity-evidence/scalar-parameters-pairs.json),
[baseline controls](rust-parity-evidence/scalar-parameters-before-controls.json),
[temporary insertion regression](rust-parity-evidence/scalar-parameters-insert-regression-before.json),
[ASAN controls](rust-parity-evidence/scalar-parameters-asan.json),
[preservation](rust-parity-evidence/scalar-parameters-preservation.json),
[formatter preservation](rust-parity-evidence/scalar-parameters-formatter-preservation.json) and
[remaining original diagnostics](rust-parity-evidence/scalar-parameters-gap-diagnostics.json).

## Reference records: combined validation, 2026-10-04

This increment is based on repaired main
`7b5d2fdaece19df06b29ade1cbebe29b5d229abd`. All six exact-revision CI jobs and
retained Linux/macOS/Windows artifacts independently pass 32 reports / 2685
cases. The reference increment is published directly to main after complete
combined local checks. All six compiler/runtime jobs for `b857f00e` subsequently pass, and
33 reports / 2856 cases per platform independently verify in the
[hosted acceptance evidence](rust-parity-evidence/reference-records-main-ci-green.json).

Ordinary reference records retain one identity across local aliases, array and
field reads, calls, returns, foreach bindings, global replacement and owner
release. Nil records preserve that same identity contract without initializing
field values. Array `contains` and `indexOf` compare identities. Explicit
reference-record value parameters and `copyOf` detach contents, while a local
`as val` binding preserves C's existing alias behavior. Array-field forwarding
continues to carry the actual shared field owner.

Owned value-record `return self` now supports ordinary content copies. A
C-valid probe exposed a copy-hook mismatch in the prototype: C invoked the
user-defined hook while Rust used derived Clone. The repaired operation invokes
the hook and propagates its receiver mutation through conditional returns and
forwarding methods. General user-copy operations remain completion work.

The unchanged original corpus passes **1103 integration + 221 exploratory
programs**, with **41 compilation gaps**, zero runtime failures and no skips:
**1324 / 1365 (97.0%) original-program coverage**. Eleven originals close; this
percentage measures coverage rather than remaining development effort. All
1365 original source hashes are unchanged. One former reference-operator
rejection moves unchanged into positive coverage and has a clean instrumented
C control. Five historical emission snapshots reflect the reference field and
identity representation; the other 507 historical snapshots remain unchanged.

A mandatory 171-case gate covers 19 independent output oracles across all nine
optimization/arithmetic modes. All nineteen C sources pass instrumented ASAN
with leak detection. Complete C and Rust suites pass without failures or skips. The combined
compiler passes all 33 reports / 2856 cases, including every preceding gate.
Rust generation/negative counts are 467 / 137; native tagged/extra/origin/negative
8 / 37 / 1 / 7, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12.
Raw byte/argv, native flush ordering and Windows text-helper controls pass.
The subsequent hosted evidence verifies this increment on all three platforms.

C production and default-target selection remain unchanged. Native/SDK owned
bridges, callbacks and closure lifetimes, interfaces/iterators, serializable
records, general copy hooks and the remaining language/ownership gaps remain
part of the full active goal. C-invalid method-copy, nested-hook declaration,
and `copyOf(nil)` probes are excluded from portable parity credit.

Evidence: [combined local validation](rust-parity-evidence/reference-records-validation.json),
[171 independent pairs](rust-parity-evidence/reference-records-pairs.json),
[original admission controls](rust-parity-evidence/reference-records-before-controls.json),
[copy-hook admission controls](rust-parity-evidence/reference-records-copy-hook-before.json),
[copy-hook diagnosis](rust-parity-evidence/reference-records-copy-hook-diagnosis.json),
[ASAN controls](rust-parity-evidence/reference-records-asan.json),
[preservation](rust-parity-evidence/reference-records-preservation.json) and
[remaining original diagnostics](rust-parity-evidence/reference-records-gap-diagnostics.json).

## Owned record index oracle: CI repair, 2026-10-04

The owned-record increment was published as
`e595e1d639bf2c95cfcd3f8b8ce7da5c06453c75`. All three compiler jobs pass;
Linux runtime and its 32 reports / 2685 retained cases independently pass.
macOS exposes nine failures in one newly added oracle: a scalar array assignment
with observable callbacks in both its index and RHS. C leaves that operand order
unspecified: GCC prints index/value while Clang prints value/index. Both targets
produce the same final mutations, and the other 31 macOS reports pass. This
probe cannot receive portable evaluation-order parity credit; its unchanged
source and all nine failures are retained in the
[CI diagnosis](rust-parity-evidence/owned-record-index-order-ci-diagnosis.json)
and [original probe](rust-parity-evidence/owned-record-index-unsequenced.sn.raw).

The replacement fixture preserves an inline index callback that grows the
array and then stores through its updated negative index. A following, explicitly
sequenced stable-index store runs an RHS callback that reads the preceding
mutation. The managed string index/RHS sequence remains covered. The independent
raw-byte oracle is updated for these defined operations; the mandatory count
remains 216. C production, default-target selection and all original corpus
sources and oracles remain unchanged. The repair is published as
`7b5d2fdaece19df06b29ade1cbebe29b5d229abd`; compiler jobs and Linux/macOS
runtime checks pass on Linux, macOS and Windows. Complete local C/Rust suites, all 216 independent
owned-record comparisons and 24 instrumented C controls pass. The other
31 report inputs and compiler binary are unchanged.
[Repair validation](rust-parity-evidence/owned-record-index-order-validation.json).
All six repair CI jobs now pass, and retained artifacts independently verify
all 32 reports / 2685 cases per platform. Integration can resume.
[Exact-main repair proof](rust-parity-evidence/owned-record-index-order-main-ci-green.json).

## Owned record parameters: direct-main increment, 2026-10-04

This increment is based on main `775fea71e2bf5e5e0504a67e4238c68be6b4b1d5`.
All six CI jobs and retained Linux/macOS/Windows artifacts for that revision
independently verify 31 reports and 2469 cases. The current increment adds a
mandatory 216-case gate covering 24 unchanged or newly defined C-valid sources
across all nine optimization/arithmetic combinations. This increment is published directly to main after the complete local checks.
Exact-revision hosted verification is pending; local results below do not
claim cross-platform acceptance.

Ordinary owned value-record reference parameters share existing field owners;
value arguments and assignments copy their contents into independent owners.
Whole-record reference assignment updates the caller and other aliases.
Array-field writes and mutating methods retain the actual owner, and private
function/method specializations carry it through array parameters, repeated
aliases, recursion and callbacks. Callback-bearing values and indices resolve
before borrowing storage. Nested record references use the same original
owners, including nested arrays and C's repeated negative-index length reads.
Floating array inserts retain C's value-before-index evaluation and byte-storage
conversions. This work also fixes field text rendering to use the supported
field-read operation.

The unchanged original corpus now passes **1092 integration + 221 exploratory
programs**, with **52 compilation gaps**, no runtime failures and no skips.
That is **1313 / 1365 (96.2%) original-program test coverage**, compared with
1301 / 1365 on the previous main. All twelve newly passing originals match C
across all nine modes; the denominator and original source files are unchanged.
This percentage is coverage, not an estimate of total remaining effort.

Five previous owned-parameter rejection sources move unchanged into Rust
positive coverage. Their canonical C controls abort with ownership errors or
fail to link, so they receive no parity credit. Independent Rust execution
checks preserve their specified outputs. One existing Rust emission snapshot,
`resolved_calls`, changes to reflect shared owned-record operator arguments;
its source and runtime oracle remain unchanged and all nine independent Rust
runtime checks pass. The other 494 historical snapshots remain unchanged.

C production and the default target remain unchanged. Native owned-record
bridges, callback/resource lifetimes, general reference-return behavior,
interfaces/iterators and serializable records remain completion work; the full
backend goal remains active. The PR queue is empty.

Evidence: [local validation](rust-parity-evidence/owned-record-parameters-validation.json),
[216 independent pairs](rust-parity-evidence/owned-record-parameters-pairs.json),
[baseline admission controls](rust-parity-evidence/owned-record-parameters-before-controls.json),
[ownership controls](rust-parity-evidence/owned-record-parameters-asan.json),
[preserved sources and snapshots](rust-parity-evidence/owned-record-parameters-preservation.json),
[Rust-only regression controls](rust-parity-evidence/owned-record-parameters-rust-regressions.json)
and [remaining original diagnostics](rust-parity-evidence/owned-record-parameters-gap-diagnostics.json).

## Native record references: direct-main increment, 2026-10-04

This increment is locally verified against main
`4446a9e5de915a9a972a673ce3bcb8c46f36d307`, whose six jobs and retained
Linux/macOS/Windows artifacts independently pass 30 reports and 2433 cases.
This increment is integrated directly on main after the complete local gates;
it is integrated as `775fea71e2bf5e5e0504a67e4238c68be6b4b1d5`.
All six exact-revision jobs pass and retained artifacts independently verify
all 31 reports and 2469 cases on Linux, macOS and Windows.
[Exact-main CI proof](rust-parity-evidence/native-record-reference-main-ci-green.json).
The compiler passes
**31 reports / 2469 cases**, including 36 independent reference comparisons
across O0/O1/O2 and all arithmetic modes. Complete C suites pass unchanged.
Rust generation/negative pass 442 / 143, native tagged/extra/origin/negative
8 / 37 / 1 / 7, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12,
without failures or skips.

Heap-free record references use persistent C-layout source storage. Nested
records and one-byte C character fields share their original address with C;
the bridge preserves same-place aliases, distinct-place mutation and C-retained
addresses read after subsequent source updates. Source character reads remain
logical characters, while writes, methods, postfix operations and compound
assignment convert at the storage boundary. All 256 byte values and their
increment/decrement/compound mutations are checked, along with later by-value
returns and independent copies. The original retained-address rejection is
promoted unchanged into positive coverage, with its original C helper unchanged.

The baseline verifies 36 successful C executions against 36 Rust rejections.
Another nine C-defined executions establish the still-unfinished thread-managed
storage obligation. Thread field wrappers cannot be cast to a C record layout;
that composition retains an explicit guard. General record-array join remains
rejected against nine successful C controls and receives no parity credit.
Owned/pointer fields, packed/refcounted records, user copy hooks and broader
lifetime/callable/SDK obligations remain work.

Five C programs and four Rust/native programs pass instrumented ASAN with leak
detection. Rust/native instrumentation covers the C bridge/runtime, not Rust
allocations. All 1365 original source hashes and 495 historical Rust snapshots
remain unchanged. The unchanged corpus still passes 1084 integration and 217
exploratory programs with **64 original compilation gaps**, no runtime failures
or skips. Benchmark coverage remains **95.3% (1301 / 1365)**; this increment
advances feature coverage without closing an original-corpus compilation gap.
C production, original expectations and the default target remain unchanged.
The refreshed PR queue is empty.

Evidence: [local validation](rust-parity-evidence/native-record-reference-validation.json),
[36 independent pairs](rust-parity-evidence/native-record-reference-pairs.json),
[C baseline and admission control](rust-parity-evidence/native-record-reference-before-controls.json),
[ASAN](rust-parity-evidence/native-record-reference-asan.json),
[remaining record-array text composition](rust-parity-evidence/native-record-reference-remaining-text.json)
and [remaining original diagnostics](rust-parity-evidence/native-record-reference-gap-diagnostics.json).

## Native value records: verified on main, 2026-10-04

This increment is verified against main
`af0893c456aa1c682b7c3678064e6d4641b101f4`, whose six CI jobs and complete
Linux/macOS/Windows artifacts independently pass 29 reports and 2388 cases.
The new local compiler passes **30 reports / 2433 cases**, including 45
independent native-record comparisons across O0/O1/O2 and all arithmetic modes.
Complete C suites pass unchanged. Complete Rust generation/negative pass
442 / 143, native tagged/extra/origin/negative 8 / 33 / 1 / 7, closures 36 / 1,
concurrency 10 / 7 / 1 and toolchain 12, without failures or skips.
It is integrated as `4446a9e5de915a9a972a673ce3bcb8c46f36d307`.
All six Compiler/Rust Runtime jobs pass, and complete retained artifacts
independently verify all 30 reports and 2433 cases on Linux, macOS and Windows.
[Exact-main CI proof](rust-parity-evidence/native-record-main-ci-green.json).

Heap-free native and ordinary value records use private C-layout wire records
and explicit field conversions. The original C ABI and sidecars remain intact;
Rust source structs retain their language storage. Nested records, every scalar
width, all 256 character bytes, bool, default and explicit by-value copies,
hygienic names, mixed string/byte-array results and aliased scalar character
references are covered. C char stays one byte at the ABI boundary. Record
references, owned/pointer fields, packed/refcounted records and user-defined
copy hooks retain their guards and remain work. Rejecting them receives no
parity credit.

The unchanged native-struct thread-return and interop edge-case programs pass
all nine comparisons. Optimized interop initially failed because removing
`i * 1.0` left an integer argument at a floating call boundary. Rust-private
lowering now writes the C-compatible conversion explicitly, preserving the
argument's evaluation and leaving C optimization unchanged. The original
corpus passes 1084 integration and 217 exploratory programs, leaving **64
original compilation gaps locally**, down from 66. That is 1301 / 1365
original programs, or **95.3% benchmark coverage**; it is not a percentage of
full feature parity. Remaining ownership, lifetimes, callbacks and SDK work
can require more effort than the raw test count suggests. There are no new runtime
failures or skips. All 1365 original source hashes and 495 historical Rust
snapshots remain unchanged.

The baseline independently verifies 45 successful C executions against 45 Rust
rejections. Another 27 C-defined executions establish three remaining admission
obligations, including a C-retained record address read after source mutation;
they remain rejected. Eight C programs and five Rust/native programs pass
instrumented ASAN with leak detection. Rust/native instrumentation covers the
C bridge/runtime, not Rust allocations. A scratch direct owned-field print
triggered canonical C cleanup double-free and is excluded; the final guard
control uses interpolation and passes C ASAN. Native-body controls were moved
into the native suite after the emission-only runner rejected their placement;
final complete suites forbid skips. C production, original source/output
expectations and the default target remain unchanged. The PR queue is empty.

Evidence: [local validation](rust-parity-evidence/native-record-validation.json),
[45 independent pairs](rust-parity-evidence/native-record-pairs.json),
[C baseline and admission controls](rust-parity-evidence/native-record-before-controls.json),
[ASAN](rust-parity-evidence/native-record-asan.json),
[remaining diagnostics](rust-parity-evidence/native-record-gap-diagnostics.json)
and [previous main CI proof](rust-parity-evidence/array-copy-store-main-ci-green.json).

## Array stores and owned copies: verified on main, 2026-10-04

This combined increment is locally verified against integrated nil-array main
`99798871a8561481f4e0c2bb9f29a74864c86899`, whose six CI jobs and complete
retained artifacts are now verified. All 29 local reports pass **2388 cases**,
including 45 independent nested-store cases and 45 owned-array copy/concat cases
across O0/O1/O2 and all arithmetic modes. Full C suites pass unchanged. Rust
generation/negative pass 442 / 143, native 8 / 30 / 1 / 4, closures 36 / 1,
concurrency 10 / 7 / 1 and toolchain 12, without failures/skips. Ten C controls
pass instrumented ASAN with leak detection. Raw-byte, transport, helper and
formatting checks pass. It is integrated on main as
`af0893c456aa1c682b7c3678064e6d4641b101f4`. All six Compiler/Rust Runtime jobs
pass, and each retained Linux/macOS/Windows artifact independently verifies all
29 reports and 2388 cases. The exact-revision proof is
[array-copy-store-main-ci-green.json](rust-parity-evidence/array-copy-store-main-ci-green.json).

Nested stable array stores resolve destination indices before taking the final
mutable borrow, including ordinary vectors and constant negative indices.
String stores preserve C's explicit index-before-replacement ordering when
replacement calls mutate the source index. Plain and nullable representations,
three dimensions, field owners, deep-copy mutation and private-name collisions
are covered. Effectful nested owners and borrowed returns remain separate work.

Array concatenation and copyOf support validated auto-copy value structs with
owned strings, arrays and nested value fields; concatenation also supports nested
arrays. Their deep copies survive replacement and cleanup of source elements.
Nil versus allocated empty state survives nested field copies and nullable-array
concatenation. Existing native, memory-qualification and user-copy-hook guards
remain; general shared array/reference/callable identity remains unfinished.

The unchanged original corpus passes 1083 integration and 216 exploratory
programs, leaving **66 original compilation gaps locally**, down from 68.
The unchanged nested-array copy and struct-array concat programs now pass all
nine mode/optimization combinations. No original runtime failures or skips occur.
All 1365 original source hashes are unchanged. Of 487 historical Rust snapshots,
485 are byte-identical; two reviewed one-line changes move index bindings ahead
of the replacement value, backed by the 90-case nil-array gate. Eight new O0
snapshots are added, and all eight controls preserve their final prepared bytes.

The baseline contains 36 nested-store compiler rejections, nine actual nullable
string-store output mismatches, and 45 owned-array compiler rejections, against
90 independently checked successful C executions. A scalar probe that changes
its own destination index in the RHS has unspecified C operand order and is
excluded. Unsupported member-copy syntax and intermediate ordering/formatting
failures are retained separately. C production, templates and original source/
output expectations are unchanged; C remains the default target.

Evidence: [combined local validation](rust-parity-evidence/array-copy-store-validation.json),
[nested-store pairs](rust-parity-evidence/nested-store-pairs.json),
[owned-array pairs](rust-parity-evidence/owned-array-pairs.json),
[nested baseline](rust-parity-evidence/nested-store-before-controls.json),
[owned baseline](rust-parity-evidence/owned-array-before-controls.json),
[nested ASAN](rust-parity-evidence/nested-store-asan.json),
[owned ASAN](rust-parity-evidence/owned-array-asan.json) and
[remaining diagnostics](rust-parity-evidence/array-copy-store-gap-diagnostics.json).

## Nil arrays: verified on main, 2026-10-04

This increment is locally verified against integrated nil-string main
`0e2a3d2e7fea6a353ff91bc0a0069741acbe9b15`, whose six jobs and complete retained
artifacts are verified. It is pushed directly to main as
`99798871a8561481f4e0c2bb9f29a74864c86899`. All six Compiler/Rust Runtime
jobs pass. Complete retained artifacts independently verify all 27 reports /
2298 cases on Linux, macOS and Windows. The open PR queue is empty.
[Exact-main CI proof](rust-parity-evidence/nil-array-main-ci-green.json).
All 27 reports pass **2298 cases**, including 90 independent nil-array cases
at O0/O1/O2 in default/checked/unchecked arithmetic modes. Complete C suites pass
unchanged; Rust generation/negative are 434 / 143, native 8 / 30 / 1 / 4,
closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12, without failures or skips.
Ten C controls and two Rust/native executables pass instrumented ASAN with leak
detection. Rust/native instrumentation covers the C bridge/runtime. Raw-byte,
text-transport, helper and formatting checks pass.

Language arrays retain nil state separately from allocated empty vectors.
Contextual nil values, default arrays, literal returns, copies, concatenation,
field/free/instance/static call contexts and native NULL byte-array results
preserve that state. Slice/range/sized/spread and string-split constructions remain
allocated arrays. Floating equality keeps C object-byte comparisons while checking
nil state. Nested owned copies retain independent mutation; stable nested stores
resolve indices before taking a mutable borrow. Private wrapper and store names
avoid source identifiers. Native array-only programs no longer emit an unused
string bridge helper when its Rust type is absent.

The unchanged original corpus passes 1081 integration and 216 exploratory
programs, leaving **68 original compilation gaps locally**, down from 69.
There are no original runtime failures or skips. All 1365 original source hashes
and 480 existing Rust snapshots are unchanged; seven new O0 snapshots are added.
Nine new controls preserve their prepared source bytes. The 90-case baseline
contains 81 compiler rejections and nine actual runtime mismatches, with distinct
compiler hashes retained. Formatting of the new hygiene control was revalidated
against its baseline; its generated snapshot is byte-identical.

Borrowed array returns remain a separate gap: nine unchanged, C-valid nil-return
comparisons are still rejected and receive no parity credit. Memory-qualified
array parameters and general array/reference/callable identity remain unfinished.
C-generated cleanup failures and frontend-rejected probes are retained separately;
they do not supply positive oracles. C production, templates and original
expectations are unchanged, and C remains the default.

Evidence: [local validation](rust-parity-evidence/nil-array-validation.json),
[90 independent pairs](rust-parity-evidence/nil-array-pairs.json),
[baseline](rust-parity-evidence/nil-array-before-controls.json),
[C ASAN](rust-parity-evidence/nil-array-asan.json),
[Rust/native ASAN](rust-parity-evidence/nil-array-native-asan.json),
[remaining borrowed returns](rust-parity-evidence/nil-array-remaining-borrowed-returns.json)
and [remaining diagnostics](rust-parity-evidence/nil-array-gap-diagnostics.json).

## Nil strings: verified on main, 2026-10-04

This increment is locally verified against main
`08aef21116b8009cfcdcc9d00ff03b37abe01146` and integrated as
`0e2a3d2e7fea6a353ff91bc0a0069741acbe9b15`. All six exact-revision CI jobs pass;
complete retained artifacts independently verify all 26 reports and 2208 cases
on Linux, macOS and Windows. All 26 local reports pass
**2208 cases**, including 72 independent nil-string cases at O0/O1/O2 in
all arithmetic modes. Full C suites pass unchanged; Rust generation/negative
are 427 / 143, native 8 / 28 / 1 / 4, closures 36 / 1, concurrency 10 / 7 / 1
and toolchain 12, with no failures or skips. Eight C controls pass instrumented
ASAN with leak detection; four Rust/native executables also pass with
instrumented C bridge/runtime. Raw bytes, text transport and formatting pass.

Nil string state is distinct from allocated empty bytes. Contextual nil values,
default strings, field/array-element copies and literal nil returns preserve that
state. Null-safe printing emits empty output; concatenation produces allocated
strings even when appending an empty suffix to nil. Native NULL results and nil
arguments retain NULL, including programs with no nil literal. Free/instance,
static and native call contexts are tested. Nullable representation is enabled
when observable null semantics are required; existing snapshots remain unchanged.

The unchanged original corpus passes 1080 integration and 216 exploratory
programs, leaving **69 compilation gaps**, down from 70. No original
runtime failures or skips occur. All 1365 original source hashes and 477 existing
Rust snapshots are unchanged; three new O0 snapshots are added. Seven new
controls preserve their prepared source bytes. The baseline contains 54 Rust
compiler rejections and 18 actual runtime mismatches, with distinct compiler
hashes retained. Nil arrays and C-undefined borrowed nil returns remain separate.

Evidence: [local validation](rust-parity-evidence/nil-string-validation.json),
[72 independent pairs](rust-parity-evidence/nil-string-pairs.json),
[baseline](rust-parity-evidence/nil-string-before-controls.json),
[C ASAN](rust-parity-evidence/nil-string-asan.json),
[Rust/native ASAN](rust-parity-evidence/nil-string-native-asan.json),
[exact-main CI proof](rust-parity-evidence/nil-string-main-ci-green.json) and
[remaining diagnostics](rust-parity-evidence/nil-string-gap-diagnostics.json).
C production, templates and original expectations are unchanged; C remains default.

## CI retention repair: 2026-10-04

The helper-scope gate passes all 36 cases in the Linux job for main
`68f870f0c104986760e99d1b4df0d46ebb034420`, but the explicit artifact upload
list omitted its report. Only 24 reports were retained, so complete retained
25-report evidence is unproven for that revision. The correction is pushed as
`afa6cf25607aed29c208041ecbf12b88042d2a53`: the artifact retains every
`.sn/rust-parity-*.json` report alongside runtime diagnostics. All 25 current
local report paths were checked against the pattern; compiler semantics are
unchanged. Exact-revision CI and complete platform artifacts are pending.

The first glob correction on `afa6cf25607aed29c208041ecbf12b88042d2a53`
matched no files because the uploader excludes hidden directories. The Linux
job passed its suites and gates but uploaded no artifact; it is not accepted as
retained parity evidence. Main `08aef21116b8009cfcdcc9d00ff03b37abe01146`
now sets `include-hidden-files: true` and makes an empty upload fail CI.
A project-local replay with the uploader's actual `@actions/glob@0.6.1` library
reproduces zero reports with the old setting and all 25 required reports with the
new setting. [Replay evidence](rust-parity-evidence/artifact-hidden-glob-validation.json)
is committed. All six exact-revision jobs now pass and every platform's
25 reports independently verify 2136 cases. The Linux Compiler job's first
attempt passed all tests but timed out creating the build artifact; one targeted
rerun passed. Its failed upload log and hash remain recorded in the proof.
[Exact-main CI evidence](rust-parity-evidence/native-scope-main-ci-green.json)
includes that attempt; no compiler change was needed for the upload timeout.

## C-only native helpers: local validation, 2026-10-04

This implementation increment is locally verified against main
`1221cc7b9946737f609bc29d4b0de94356239639`; all six hosted jobs pass on helper-scope revision
`68f870f0c104986760e99d1b4df0d46ebb034420`. Complete retained artifact evidence
is verified on unchanged compiler revision `08aef21116b8009cfcdcc9d00ff03b37abe01146`
after the two retention corrections described above.
All 25 runtime reports pass **2136 cases**, including 36 independent helper-scope
cases at O0/O1/O2 in default/checked/unchecked arithmetic modes. Complete C suites
pass unchanged; Rust generation/negative are 424 / 143, native 8 / 24 / 1 / 4,
closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12, with no failures/skips.
Four C sources pass instrumented ASAN with leak detection.

Rust previously required bridge-compatible signatures for every native function,
including functions called only from C. Native dependencies now remain compiled
in the sidecar without redundant Rust wrappers. Rust callers, including lambda
bodies, retain the bridge and ABI validation. Unreferenced declarations retain
validation, and all native bodies retain checks for unavailable runtime constructs.
This executes the full native programs; it does not implement public aggregate ABI.

Three unchanged originals now pass: native struct interop, native reference-struct
arrays and streaming-state mutation. The original corpus passes 1079 integration
and 216 exploratory cases, leaving **70 original compilation gaps**, down from 73.
There are no runtime failures/skips; all 1365 original source hashes and 477
historical Rust snapshots are unchanged. A byte-preserved transitive helper control
also verifies a shared scalar function called both by C and by Rust directly and
through a lambda. All 36 controls previously passed C and were rejected by Rust;
the two baseline compiler hashes/revisions are recorded separately.

Evidence: [local validation](rust-parity-evidence/native-scope-validation.json),
[36 independent pairs](rust-parity-evidence/native-scope-pairs.json),
[baseline controls](rust-parity-evidence/native-scope-before-controls.json),
[ASAN](rust-parity-evidence/native-scope-asan.json) and
[remaining diagnostics](rust-parity-evidence/native-scope-gap-diagnostics.json).
C production/templates/original expectations are unchanged, C remains default,
main is refreshed and the PR queue is empty. Hosted verification and the remaining
language/native/SDK obligations are still required for full parity.

## Indexed string copies: verified on main, 2026-10-04

The next implementation increment is locally verified against main
`1c192f1d9cbf557e5587089eadd2568a148f7d02`; integrated main
`1221cc7b9946737f609bc29d4b0de94356239639` passes all six hosted CI jobs.
Each platform independently verifies all 24 retained reports and 2100 cases.
Evidence: [exact-main CI proof](rust-parity-evidence/indexed-string-main-ci-green.json).
All 24 runtime reports pass **2100 cases**, including 54 independent indexed
string cases at O0/O1/O2 in default/checked/unchecked arithmetic modes. Complete
C suites pass unchanged; Rust generation/negative are 424 / 143, native
8 / 23 / 1 / 4, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12, all
with zero failures/skips. Six C sources pass instrumented ASAN with leak detection.

C returns an independent strdup of indexed string values. Rust now acquires
those borrowed values in free/static returns, preserving independent result
mutation and the caller's array. Computed owners and indices evaluate once,
in C's order, while the owner remains alive. Local initialization of a string
from a computed array has the same lifetime/evaluation guarantees and avoids a
redundant outer copy. Private temporary names are checked against all source
names; the collision control deliberately uses those names. Instance method
behavior stays verified alongside free/static and generic calls.

The unchanged original corpus passes 1077 integration and 215 exploratory cases,
leaving **73 original compilation gaps**, down from 74, with no runtime failures
or skips. All 1365 original source hashes and 472 historical Rust snapshots
are byte-identical; five new snapshots are added. The baseline has 45 compiler
rejections and nine defined runtime mismatches among the 54 C controls. The
runtime mismatch evaluates a computed owner twice during local initialization;
the new control requires exactly one evaluation. C-invalid direct borrowed-string
printing and generic aggregate-copy probes are retained separately and receive
no parity credit.

Evidence: [local validation](rust-parity-evidence/indexed-string-validation.json),
[54 independent pairs](rust-parity-evidence/indexed-string-pairs.json),
[baseline controls](rust-parity-evidence/indexed-string-before-controls.json),
[ASAN](rust-parity-evidence/indexed-string-asan.json) and
[remaining diagnostics](rust-parity-evidence/indexed-string-gap-diagnostics.json).
C remains default, main is refreshed and the PR queue is empty. Exact-revision
hosted checks and broader array/reference/callable identity, aliases, mutation,
native/SDK and language obligations remain required for full parity.

## sizeof fallback and reference calls: local validation, 2026-10-04

The next implementation increment is locally verified against main
`8cb11594f577c7c497244bc549c9000dca5566a2`; its hosted CI is verified below.
All 23 runtime reports pass **2046 cases**, including 126 new independent
sizeof/reference-call comparisons at O0/O1/O2 in default/checked/unchecked
arithmetic modes. Full C suites pass unchanged; Rust generation/negative are
419 / 143, native tagged/extra/origin/negative 8 / 23 / 1 / 4, closures 36 / 1,
concurrency 10 / 7 / 1 and toolchain 12, with no failures or skips.

Standalone callable/nil/void sizing now matches C's long-long fallback;
interfaces use target pointer size. Unevaluated sizeof operands do not register
runtime lambdas or capture mutations, including native zero-capture queries.
Borrowed reference-call lvalues acquire handles, and reference parameter fields
are classified as interior mutation places. Controls verify free/instance/static
calls, repeated aliases, returned handles, field/index arguments, mutation
visibility and read-only reentry using the existing shared thread field owners.
These changes do not complete general reference storage or callable/interface ABI.

The unchanged original corpus passes 1076 integration and 215 exploratory cases,
leaving **74 original compilation gaps**, down from 75. There are no runtime
failures or skips; all 1365 original source hashes and 460 historical Rust
snapshots are byte-identical. Four exact negative sources are promoted, and 12
new Rust snapshots are added. The unused former negative queries already pass
at O1/O2 through dead-code elimination; printed controls establish actual sizing
support in every mode. Four initial new snapshots used O2 instead of the
runner's required O0; their correction is retained in the evidence, and the
complete final suite passes.

Fourteen C sources and one C/Rust native pair pass instrumented ASAN with leak
detection. C remains default, remote main is refreshed, and the PR queue is empty.
Evidence: [local validation](rust-parity-evidence/call-reference-validation.json),
[126 independent pairs](rust-parity-evidence/call-reference-pairs.json),
[ASAN](rust-parity-evidence/call-reference-asan.json) and
[remaining diagnostics](rust-parity-evidence/call-reference-gap-diagnostics.json).
A probe that mutates the same target inside a compound-assignment RHS has
indeterminate C evaluation ordering and receives no portable-order parity credit;
rejected frontend probes are likewise excluded and recorded explicitly.
Full parity and exact-revision hosted verification remain required.

## Current verified status: 2026-10-04

Main `af0893c456aa1c682b7c3678064e6d4641b101f4` passes all six Linux/macOS/Windows
Compiler and Rust Runtime jobs. Complete hosted C/Rust suites pass without
failures or skips. Each platform's 29 retained runtime reports independently
verifies **2388 cases**: 2088 positive C/Rust pairs, 39 diagnostic/order cases,
153 process-exit cases and 108 main-return cases. The 45 nested-store and 45
owned-array cases verify ordered stores and independent copies. C remains default.

The unchanged original corpus passes 1083 integration and 216 exploratory
programs, leaving 58 and eight compilation gaps: **66 remaining original gaps**
on this verified revision. All 1365 original source hashes are unchanged. The
495 passing Rust snapshots include two reviewed order corrections and eight new
snapshots from the combined store/copy increment. Ten C controls pass
instrumented ASAN with leak detection. The open PR queue is empty.

Evidence: [local validation](rust-parity-evidence/array-copy-store-validation.json),
[nested-store pairs](rust-parity-evidence/nested-store-pairs.json),
[owned-array pairs](rust-parity-evidence/owned-array-pairs.json),
[remaining diagnostics](rust-parity-evidence/array-copy-store-gap-diagnostics.json)
and [exact-main CI proof](rust-parity-evidence/array-copy-store-main-ci-green.json).

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


## Float conversion main: hosted verification, 2026-10-03

Exact direct-main `133c145225521d1f9f3d127b9fc3f890499b7eb9` passes all six
jobs in [Compiler CI 37121624679](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37121624679)
and [Rust Runtime CI 37121624792](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37121624792).
All eleven downloaded reports pass on every platform: 711 positive pairs and
39 diagnostic/order cases per system, 2,250 hosted cases total. Every source
hash matches the published fixture. All 117 floating cases also match their
independent output oracle on both targets, applying Windows CRT newline
transport to the expected bytes. Complete C/Rust suite logs retain the local
counts above on all three platforms, with zero failures or skips.
Evidence: `float-conversions-main-ci-green.json`. The open PR queue is empty.

New local diagnosis receives no parity credit. The unchanged
`test_compound_assignment.sn` passes nine C controls and fails Rust admission
in every mode. Its indexed place is classified as computed; the current C
renderer also reevaluates the indexed target for the store. Preserve the actual
defined evaluation and mutation behavior when implementing that place.

The unchanged `tests/rgen/float_array_search.sn` compiles and runs on both
targets in nine controls, but their raw outputs differ every time. For example,
C finds the literal `9.5f` and reports index zero for the literal `1.5f`, while
Rust reports false and index one. Current restored C emits those direct source
float literals as double C expressions; its array-search macro compares the
first four bytes of the actual argument object. Rust currently casts exact
float source arguments to f32 and compares f32 bits. A fresh observable double
needle on `float[]` likewise succeeds through C's byte-prefix comparison while
Rust rejects it. These are implementation gaps, not evidence for changing C.

Historical PR100 did merge a C literal suffix repair, but
`48975cbe` deliberately restored the tagged C production and quarantined
post-tag C fixtures, as documented in `restoration/README.md`. The older audit's
float-search prerequisite and C integration-fixture claims describe that earlier
checkpoint and do not establish current C behavior. The next array-search repair
must use the restored C contract and observable unchanged controls. A narrower
actual argument object searched in a wider-element array needs separate
analysis of defined memory access; out-of-object reads earn no parity credit.
Raw follow-up reports and source/model provenance are retained locally under
`.sn/float-followup-diagnosis.json` and its referenced reports for the next
implementation increment. This remains an active completion goal.


## Floating array representation increment: local verification, 2026-10-03

Starting from main `133c1452`, the unchanged `float_array_search.sn` compiles
and runs through both targets but differs in all nine paired controls. The
current C macro compares `elem_size` bytes from an object of the rendered
argument's actual type. The restored renderer emits float source literals as
C double expressions, so searching `float[]` with a literal uses its double
byte prefix; a stored float still uses four float bytes. Numerically casting
all needles to f32 was incorrect. No C/shared production or default-target
selection changes are included in this repair.

Target-local copies of floating array argument expressions now retain actual C
literal, unary, arithmetic-helper/raw-operator and signature/storage widths.
Ordinary/method/function-value/native boundaries continue to convert at their
C signature. Searches compare native-endian byte prefixes, preserving literal
and stored-value differences, signed zero, copied NaN bits and first-hit/miss
results. For stable variable receivers the needle is evaluated before the Rust
borrow, allowing a needle function to mutate the same array and preserving the
observed contents without holding an incompatible borrow across that call.

The mutation adversary also exposed float-array `push` representation: C copies
bytes from the value object, truncating a larger object and zero-filling a
smaller one through `sn_array_push_safe`. Rust now preserves those bytes instead
of numerically converting. `insert` also copies a byte prefix, evaluates its
value before its index and leaves invalid negative/oversized positions unchanged.
The source checker/model retains its existing argument order; private rendering
handles C's already reordered macro arguments. Tests observe new elements,
lengths, call counts, and the decimal evaluation trace 12/1212/121212. Captured
array mutation persists inside the capture across calls while the original array
stays unchanged; the observed results are true/false/1/false.

A wider C element compared or inserted from a narrower actual object can read
outside that object when the operation reaches the memory copy/comparison. No
such execution is credited as defined parity. Rust uses a bounds-aware prefix
comparison and zero-initialized storage, avoiding unsafe reads. Empty wider
array searches are defined C controls and are covered, including evaluation of
the narrower producing call. `push`'s zero-filled widening is defined and covered
separately. The exploratory outer-capture indexed read beyond its logical/allocated
array extent is retained as an excluded probe, not passing behavior evidence.

The former `float_array_search_double_needle` rejection source moves unchanged
to a positive generation test after nine C/Rust controls pass. Its unused found
value proves admission only; adjacent representation, miss, call and mutation
observations provide behavior evidence. Three stale output lines in the existing
float-search Rust oracle are corrected to measured C behavior; neither existing
source changes. Only the two historical float/double-search Rust snapshots are
updated, with four new positive snapshots added. All other historical Rust
snapshots remain byte-identical.

`make test-rust-parity-floating-arrays` requires nine unchanged or new C-valid
sources in 81 positive pairs across O0/O1/O2 and default/checked/unchecked modes.
Its arithmetic-width case deliberately observes index one for checked helpers
and index zero for raw arithmetic (including default O2). Independent checked
and unchecked output oracles pin both behaviors. A scalar native case observes
both ABI widths. Combined gates require 792 positive pairs plus 39
diagnostic/order cases per platform (831 cases in twelve reports).

Full local C counts remain 1610/107/79/1141/58/224/11. Rust counts are generation
336, negative 165, native 8/19/1/4, closures 36/1, concurrency 10/7/1 and toolchain
12, with zero failures or skips. Raw-byte, text-transport/helper and formatter
checks are required before publication. The original 1,365 corpus source hashes
are unchanged; 1,060 integration and 210 exploratory executions pass, with
81+14 Rust compilation gaps and no admitted runtime failures in this O0 sample.
The broader 95 compilation gaps and unverified ownership, SDK, module/type and
behavior families remain required work; this increment does not prove full
floating-array or whole-language parity. Indexed compound assignment is the next
confirmed corpus gap. Evidence: `rust-parity-evidence/floating-arrays-*`.
Publication and exact-revision hosted verification are pending.


## Floating arrays main: hosted verification, 2026-10-03

Exact direct-main `ee3879f5313a1093c5f5952c0a252b26d1fbb824` passes all six
jobs in [Compiler CI 37125552626](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37125552626)
and [Rust Runtime CI 37125552621](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37125552621).
All twelve downloaded reports pass on every platform: 792 positive pairs and
39 diagnostic/order cases per system, 2,493 hosted cases total. Every source
hash matches its committed source. All 81 floating-array cases additionally
match independent checked/unchecked output oracles on both targets, applying
Windows CRT newline transport to the expected bytes. Complete hosted C/Rust
suite logs verify the local counts above with zero failures or skips.
Evidence: `floating-arrays-main-ci-green.json`. The open PR queue is empty.

The next local behavior diagnosis remains a failure and earns no parity credit.
A new C-valid signed-zero/NaN equality source compiles and exits zero through both
targets in all nine controls, but their outputs differ each time. C outputs
false/true/true/false/true for float signed-zero arrays, copied float NaN arrays,
self float NaN arrays, double signed-zero arrays and copied double NaN arrays.
Rust outputs true/false/false/true/false. Tagged `sn_array_equals` uses pointer
identity, lengths and a contiguous raw byte comparison; Rust currently falls
through to numerical vector equality. This is separate from the repaired array
method argument representation and the 95-source compilation denominator.
Mixed-width array equality needs the left runtime element width and contiguous
byte-prefix behavior, not numerical conversion or per-item prefix comparison;
out-of-object/uninitialized reads do not establish defined parity. Exact local
source/report and implementation notes are retained in
`.sn/floating-array-equality-before.json`, `.sn/floating-array-equality-probe.sn`
and `.sn/floating-array-equality-notes.md` for the next increment. Indexed numeric
mutation notes are retained in `.sn/indexed-numeric-implementation-notes.md`.
The full parity goal remains active.


## Floating-array equality: implementation and remaining ownership gap, 2026-10-03

This increment replaces Rust vector numerical equality/inequality for float and
double arrays with contiguous native-byte comparison. Signed zero remains
distinct and copied NaN objects compare equal, matching tagged C. Equal element
counts are required, and mixed widths use the left array byte count. The mixed
width control uses zero bytes and works independently of endianness; it proves
that C compares a contiguous prefix rather than corresponding item prefixes.
Narrow right-hand storage beyond the available object is never executed or
credited as defined parity.

Typed slice bindings fix element widths for standalone array literals and keep
temporary operands alive through the comparison. A right array literal
runs before the left, matching the C template's explicit materialization;
independent trace oracles check 21 for both equality and inequality. Stable
variable/member reads wait until the other operand completes, permitting the
RHS callback to mutate the same local array or struct field. Temporary names
are reserved against the whole model and a source fixture tests collisions.
Four unchanged C-valid fixtures cover default/checked/unchecked arithmetic at
O0/O1/O2: 36 positive pairs with independent output expectations, default float
and double parameters, function returns, closure captures, struct fields and
nested array access.

A separate unchanged C-valid probe returns a borrowed default-array parameter
directly. C succeeds; Rust still cannot return its mutable vector reference as
an owned vector. The source and before/final diagnostics are retained and earn
no parity credit. The successful mutation fixture uses explicit copyOf in both
targets, preserving an independent observable test of mutation visibility.
General array identity/returned aliases, nil versus allocated-empty arrays and
mixed-width array parameter transport remain required work. This increment
does not claim full floating-array parity or alter the 95-source original
corpus compilation denominator. Indexed numeric mutation remains next.

CMake now discovers added Rust templates as dependencies, so later edits are
staged by the normal build. Every staged Rust template is byte-identical to its
source before final validation. No historical Rust snapshot or existing C
source/oracle was changed. Final local required checks pass: C unit 1610,
cgen 107, model 79, integration 1141/58, exploratory 224/11; Rust generation
340, negatives 165, native 8/19/1/4, closures 36/1, concurrency 10/7/1 and
toolchain 12, all with zero failures or skips. Raw bytes (63 executions),
transport (6 executions), helper simulation and formatting also pass.
All 13 gates pass: 828 positive pairs and 39 diagnostic/order cases (867 total).
The original 1365 source hashes are unchanged; broader Rust corpus sampling
remains 1060+210 successful executions and the same 81+14 compile failures.
Exact-revision hosted verification follows publication. Evidence:
rust-parity-evidence/floating-array-equality-*.


## Floating-array equality main: hosted verification, 2026-10-03

Exact direct-main `d26b391f47df7c30238d047e7214c100feb8960c` passes all three
[Compiler jobs](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37129078700)
and all three [Rust Runtime jobs](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37129078696).
Downloaded all 13 reports on Linux/macOS/Windows and verified 867 successful
cases per platform (828 positive pairs and 39 diagnostic/order cases), matching
source hashes and all 36 new independent output oracles on each platform.
Windows verification uses its native CRLF output bytes and normalizes only the
reported source-path separator for local source lookup. Hosted full C/Rust logs
have zero failures/skips and the required final suite counts on all platforms.

Fresh origin/main matches the tested revision; the open PR queue remains empty.
The broad corpus still has the same 95 compilation gaps, and returned array
identity, nil/empty distinctions, cross-width parameter transport, computed
operand aliases, native/SDK and other unverified families remain required work.
Next implementation: indexed numeric compound mutation, beginning with unchanged
`tests/integration/test_compound_assignment.sn` and its nine C-successful controls.
The full parity goal remains active.

Exact-run metadata/report hashes and hosted suite-log hashes:
`rust-parity-evidence/floating-array-equality-main-ci-green.json`.
Downloaded artifacts and logs: `.sn/floating-equality-hosted-{linux,macos,windows}`
and `.sn/floating-equality-{compiler,runtime}-hosted.log`. Hosted-green metadata
and this final ledger update are ready to bundle with the next implementation
commit, avoiding a separate documentation-only CI cycle.

## Computed numeric mutation increment: 2026-10-03

Based on hosted-green `d26b391f`, Rust now emits compound and postfix mutation
of fixed-integer and floating scalar computed places. The unchanged original
`test_compound_assignment.sn` moves from nine C-successful/Rust-rejected cases
to nine successful pairs. Six portable C-valid former negatives move byte-for-byte into generation
coverage. A seventh GCC-accepted atomic-struct member probe remains Rust-only:
Apple Clang rejects that expression as undefined behavior, so it earns no
parity credit. Two returned value-struct field probes remain negative because
C rejects those fields as lvalues.

Compound mutation copies the old scalar, releases its borrow, evaluates the
RHS at the actual C arithmetic width, and reevaluates the store indices before
acquiring a short mutable borrow. Postfix resolves one place and returns the
old scalar. Negative indices retain the repeated parent length reads observed
in C. Checked helper signatures, source LL/double literals, ordinary raw
arithmetic, and final storage conversion are separate width boundaries. The
encoded uint64 maximum literal now renders its stored bit pattern correctly.

Returned owned array receivers are hoisted once before their statement, as in
the C chain pass, including short-circuit and while timing. C collects lambda
definitions before that pass: the tested negative-index lambda receiver has
one eager construction-time call and two calls per invocation. Rust matches
those observed effects. The initial mismatch and rendered C body are retained.
Mutable array snapshot captures preserve their own state while isolating the
outer array; default/thread array parameters and nested by-value/as-ref struct
storage have explicit observers. RHS callbacks can modify arrays without a
surviving mutable borrow. Same-array default parameters retain alias behavior.

Local final verification uses one exact compiler hash, staged templates checked
byte-for-byte, and unchanged C/shared production sources and C oracles:

| Evidence | Result |
|---|---:|
| Numeric gate: 17 sources × nine optimization/arithmetic modes | 153 positive pairs |
| All 14 differential reports | 981 positive + 39 diagnostic/order cases |
| Rust generation / diagnostics | 356 / 158 passed, zero failed/skipped |
| Native tagged / extra / origin / diagnostics | 8 / 19 / 1 / 4 passed |
| Closure positive / diagnostics | 36 / 1 passed |
| Concurrency positive / promoted / diagnostics | 10 / 7 / 1 passed |
| Rust toolchain | 12 passed |
| C unit / generation / model | 1610 / 107 / 79 passed |
| C integration / diagnostics / explore / diagnostics | 1141 / 58 / 224 / 11 passed |
| Raw byte / output transport | 63 / 6 executions passed |
| Broader unchanged Rust integration / explore corpus | 1061 / 210 passed; 80 / 14 compilation gaps |
| Historical Rust snapshots changed | 0 |

The 153 pairs are additionally checked against independent mode-aware raw
output oracles. Empty-output promoted fixtures establish admission only;
the nine new observer programs and threaded parameter control establish
behavior. The corpus denominator remains the same 1,365 C-positive sources,
all hashes unchanged. Its only removed failure is `test_compound_assignment`;
94 compilation gaps remain, with no newly admitted runtime failures.

Evidence: [numeric validation](rust-parity-evidence/numeric-places-validation.json),
[main baseline rejection](rust-parity-evidence/numeric-places-before.json),
[expanded unpublished probes](rust-parity-evidence/numeric-places-expanded-before.json),
[former-negative C audit](rust-parity-evidence/numeric-places-former-negatives-before.json),
[lambda mismatch](rust-parity-evidence/numeric-places-lambda-before.json),
and the `numeric-places-*.log` files. The runtime workflow retains the new
153-case report on Linux, macOS and Windows.

No parity credit is assigned to invalid array-as-ref qualifiers or the C
unchecked precedence failure in a proposed enclosing compound expression;
original sources/reports are retained, and the valid owner-timing control uses
postfix. General returned-array identity, nil/empty and cross-width transport,
character arithmetic, broader mutable struct/closure lifetimes, native/SDK and
remaining language/concurrency gaps still require completion. Exact-revision hosted verification for the repaired increment is recorded below.


### Immediate CI repair for atomic-struct classification

The first numeric push, `cb189c3d`, passed all three Compiler jobs and Linux
Runtime, but macOS Runtime rejected the nested member mutation of an atomic
struct with `-Watomic-access`: accessing a member of an atomic structure is
undefined behavior. GCC accepting this source was insufficient evidence of a
portable C contract. No diagnostic suppression or C production change is used.
The original source remains byte-identical in Rust-only generation coverage,
and its GCC results are explicitly superseded as parity evidence.

The cross-platform gate now exercises ordinary nested struct mutation under a
primitive sync lock, with independent observations of postfix old values,
compound new values and final state. It still enforces 17 sources and 153
positive pairs across all modes. The full Rust suite adds this observer, for
356 generation passes and 158 diagnostic passes; C/shared production and old
Rust snapshots remain unchanged. The first CI failure report is retained in
[atomic struct CI evidence](rust-parity-evidence/numeric-places-atomic-struct-ci-failure.json).
Corrected exact-revision hosted verification is green and recorded below.

The underlying C atomic-struct emission defect and retained unchecked compound
precedence defect remain follow-up work. Their acceptance or rejection is not
counted as verified behavior; resolve their language contract deliberately
before any future parity credit.


### Corrected main verification: `7c7482e1`

The immediate repair is integrated on main as
`7c7482e17896d0675a8d8c87e5780c505d548491`. All six exact-revision jobs pass:
[Compiler CI](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37136604460)
and [Rust Runtime CI](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37136604428).
All 14 retained differential reports on each platform pass: 1,020 cases,
comprising 981 positive pairs and 39 diagnostic/order cases. Source hashes,
compiler consistency within each platform, raw output/status and 153
independent new mode-aware output oracles per platform are verified, including
Windows CRLF transport. Full hosted suite counts match the local 356/158 Rust
and unchanged C counts, with zero failures or skips. The PR queue is empty,
and origin/main matches the verified head.

[Exact hosted evidence](rust-parity-evidence/numeric-places-main-ci-green.json)
records the runs, job links, report hashes and complete suite summaries. The
first failed CI report remains retained; the original atomic-struct probe earns
no parity credit. The goal remains active: 94 original corpus compilation gaps
and the remaining ownership, nil/empty, native/SDK, callable, language and
concurrency families still require implementation and verification.


## Captured array indexed writes: local verification, 2026-10-03

Rust now supports scalar indexed assignment into captured array snapshots,
including nested arrays and shared array variables. The snapshot belongs to
one closure, retains changes across calls, and stays independent of sibling
closures and the original array. Shared captures use their existing shared
storage. Reads before the first write receive the same storage annotation as
later reads. RHS expressions and callback-bearing indices finish before the
short mutable borrow used for the final store. Assignment expressions return
the stored scalar value; float/narrow integer storage follows C expression
widths, while characters keep Rust's character representation. Temporary names
are reserved against the entire model.

Five new unchanged C-valid sources have independent output expectations and
45 positive C/Rust pairs across O0/O1/O2 and default/checked/unchecked arithmetic.
They observe sibling isolation, nested negative indices, shared callback
visibility, reads preceding writes, consumed assignment results, scalar widths
and temporary-name collisions. All historical Rust snapshots and all C sources,
expectations and production code remain unchanged. The write-only original
closure_values_array_mutation negative is also C-invalid because C's capture
collector misses that access; it stays negative and receives no parity credit.

Required local checks pass with zero failures/skips: Rust generation 361 and
negative generation 158; native 8/19/1/4, closures 36/1, concurrency 10/7/1,
toolchain 12; C unit 1610, generation 107, model 79, integration 1141/58 and
exploratory 224/11. All 15 differential reports pass: 1026 positive pairs and
39 diagnostic/order cases, 1065 total. Raw-byte 63 executions, transport 6,
Windows helper simulation and formatting pass. Staged templates match their
source bytes. The original 1365 source hashes are unchanged; broader Rust
integration/explore results remain 1061/210 successes and 80/14 compilation
gaps, with no admitted runtime failures.

Additional lifetime diagnosis is qualified: four C fixtures pass AddressSanitizer
and LeakSanitizer. Shared-array replacement reports an existing 88-byte C leak.
The original failed report is retained. All five address-only checks pass with
`ASAN_OPTIONS=detect_leaks=0`; this does not establish leak-free C ownership.
No C baseline change or suppressed failure receives parity credit.

The goal remains active. Default callable array parameters still need caller
mutation, duplicate-alias and reentrant observer behavior. Named default array
parameter returns preserve C pointer identity, while borrowed array-field
returns use copies; treating every borrowed return as a clone is incorrect.
Whole-parameter rebinding and expression-body borrowed-return probes crash in
C and are not admitted as positive tests. Nil versus allocated-empty arrays,
mixed-width transport, general ownership/native/SDK support, the 94 corpus
compilation gaps and previously recorded language/concurrency gaps remain open.
Exact-revision hosted verification is recorded below. Evidence:
[capture-index-validation.json](rust-parity-evidence/capture-index-validation.json),
[capture-index-pairs.json](rust-parity-evidence/capture-index-pairs.json), and
[capture-index-asan.json](rust-parity-evidence/capture-index-asan.json).


## Captured array indexed writes main: hosted verification, 2026-10-03

Main revision `39acf61e0e559c9077a36e24ce2526cb25d5b7eb` passes all six jobs:
[Compiler CI](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37145744604)
and [Rust Platform Runtime](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37145744660).
Linux, macOS and Windows each retain 15 passing reports with 1065 cases,
including all 45 new cases checked against independent raw output expectations
(CRLF on Windows). Artifact source hashes match unchanged repository sources.
Full hosted C/Rust suite counts pass without failures or skips. The open PR
queue is empty. This increment does not resolve the remaining 94 original
corpus compilation gaps or the broader ownership/transport work. Evidence:
[capture-index-main-ci-green.json](rust-parity-evidence/capture-index-main-ci-green.json).


## Synchronized character stepping: local verification, 2026-10-03

Rust mutex-backed character increment/decrement now changes the character's
8-bit language value with wrapping addition/subtraction and restores a Rust
character for storage. The existing mutex/gate scope and postfix result path
remain intact: local atomics return the previous value; tagged global sync
postfix expressions reread the updated value after releasing their guards,
as in C. C remains the default target; C/shared production code, original
sources and expectations, and every historical Rust snapshot remain unchanged.

The unchanged integration/test_sync_byte_char and
exploratory/test_sync_byte_threading programs now pass all nine optimization /
arithmetic combinations. Three new sources independently observe local and
global postfix results, zero/maximum wraparound, unchanged byte stepping and
500 increments across five joined threads. All 45 new gate cases match explicit
output expectations. The source fixtures use zero/decrement and ASCII values
because a separate shared formatter defect splits hexadecimal character
escapes; that defect is retained as a follow-up without changing the formatter.

Required local validation passes: Rust generation 364 and negatives 158,
native 8/19/1/4, closures 36/1, concurrency 10/7/1, toolchain 12; C unit 1610,
generation 107, model 79, integration 1141/58 and exploratory 224/11. After the
final boundary fixture adjustment, all three strict snapshot/runtime checks
and the complete 45-case gate passed again. All 16 reports pass: 1071 positive
pairs and 39 diagnostic/order cases, 1110 total. Raw-byte 63 executions,
transport 6, Windows helper simulation and formatting pass. Template bytes
match staged files; template hashes are retained alongside the compiler hash.

All 1365 original source hashes are unchanged. Broad Rust corpus results are
now 1062 integration and 211 exploratory successes, with 79 and 13 compilation
gaps respectively. The two removed failures are exactly the two synchronized
character sources, with no new runtime failures or skips. The goal remains
active: 92 original compilation gaps and the broader ownership, array/callable
transport, native/SDK and language/concurrency gaps still require completion.
Exact-revision hosted verification is recorded below. Evidence:
[sync-character-validation.json](rust-parity-evidence/sync-character-validation.json),
[sync-character-pairs.json](rust-parity-evidence/sync-character-pairs.json), and
[sync-character-formatter-defect.json](rust-parity-evidence/sync-character-formatter-defect.json).


## Synchronized character stepping main: hosted verification, 2026-10-03

Main revision `3e784b74034b64c59210d76e43691797c50f4fb5` passes all six jobs:
[Compiler CI](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37147557568)
and [Rust Platform Runtime](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37147557541).
Each Linux/macOS/Windows artifact set has 16 passing reports with 1110 cases,
including all 45 synchronized-character cases checked against explicit output
expectations (CRLF on Windows). All source hashes match the repository; complete
hosted C/Rust suites have their required counts, zero failures and zero skips.
The original corpus still has exactly 92 compilation gaps and no new admitted
runtime failures. The full parity goal remains active. Evidence:
[sync-character-main-ci-green.json](rust-parity-evidence/sync-character-main-ci-green.json).

## Character methods and CRT string semantics: local verification, 2026-10-03

Rust now emits all eight C character methods: `toString`, `toInt`, `toUpper`,
`toLower`, `isDigit`, `isAlpha`, `isWhitespace`, and `isAlnum`. A reserved private
module calls the platform CRT with unsigned byte inputs. Integer conversion
uses the platform C `char` signedness; string conversion preserves one raw byte
and returns an empty string for NUL. Receivers execute once, including named
callbacks, closure parameters/captures and array/struct members.

String casing, `trim()` and `isBlank()` use the same active C locale. The
unchanged C probe exposed Rust retaining vertical tabs during trim; it now
agrees with C. `splitWhitespace()` retains C's separate space/tab/LF/CR rule.
The native reference checks all 256 byte values in both required C and
environment locales. Missing locale selection fails its independent oracle;
this does not claim coverage of every locale available on other hosts.

The new eight-source gate runs 72 independent raw-output/status checks across
O0/O1/O2 and default/checked/unchecked arithmetic. It includes unchanged
`test_char_methods` and `test_str_comprehensive`, four new Rust snapshot/runtime
fixtures, unchanged `string_operations`, and the native C reference. Five
selected existing Rust snapshots were reviewed for helper-module and call
changes; existing sources and output expectations remain unchanged.

Complete local validation passes: Rust generation 368, negatives 158,
native 8/19/1/4, closures 36/1, concurrency 10/7/1 and toolchain 12; C unit 1610,
cgen 107, model 79, integration 1141/58 and exploratory 224/11. Raw byte/text
transport, Windows helper logic and formatting pass. All 17 reports contain
1182 passing cases: 1143 positive pairs and 39 diagnostic/order checks.
The original corpus now passes 1063 integration and 212 exploratory sources,
with 78 and 12 compilation gaps, no new runtime failures or skips, and all
1365 original source hashes unchanged. The full goal remains active.

C expression-bodied closure chains exposed a stack-use-after-scope, confirmed
by ASAN. Positive closure probes use explicit intermediates; the undefined
original probes are retained as diagnostic evidence. The C string fixture also
exposes a 314-byte leak from temporary split results; its leak failure is
retained, and a separate address-only run passes with leak detection disabled.
Three existing Rust string-match fixtures crash in restored C on a null
reference argument, so their snapshots remain Rust runtime coverage and do not
count toward differential parity. C emission, C runtime, original sources and
existing output oracles were not changed to hide these defects.

Evidence: [ctype-validation.json](rust-parity-evidence/ctype-validation.json),
[ctype-pairs.json](rust-parity-evidence/ctype-pairs.json),
[ctype-string-before.json](rust-parity-evidence/ctype-string-before.json),
[ctype-asan.json](rust-parity-evidence/ctype-asan.json), and
[ctype-string-match-c-asan.json](rust-parity-evidence/ctype-string-match-c-asan.json).

## Character/CRT main: hosted verification, 2026-10-03

Main revision `d62390dfb2f6f1da11dcc8a78ca3c477f63d91fe` passes all six jobs:
[Compiler CI 37150991352](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37150991352)
and [Rust Runtime CI 37150991322](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37150991322).
Downloaded Linux, macOS and Windows artifacts verify all 17 reports and 1182
cases per platform, including all 72 independent new raw-output oracles and
Windows CRLF behavior. Source hashes match the committed sources; all reports
on each platform use one compiler binary. Full hosted suite counts pass without
failures or skips. The original corpus still has 90 compilation gaps; broader
ownership/native/language work remains. Evidence:
[ctype-main-ci-green.json](rust-parity-evidence/ctype-main-ci-green.json).

The next byte-encoding characterization identifies missing `toHex`, `toBase64`
and `toStringLatin1` methods. Five programs pass 45 C runs against independent
raw-byte oracles and ASAN; parameter/member/shared-capture probes add 18 C runs
and two ASAN successes. A C-defined integral-array-to-byte-array read exposes
object representation rather than numeric element narrowing. These baseline
probes are preparation for the next increment and do not count as Rust parity.


## Byte-array encoding: local validation, 2026-10-03

Rust now emits `toHex`, `toBase64`, `toStringLatin1` and `toString` for byte
arrays. Hex is lowercase, Base64 preserves standard padding, Latin-1 converts
nonzero bytes to UTF-8, and string methods stop at the first NUL as restored C
does. Receiver evaluation occurs once. Runtime helpers use names reserved
against the complete model, including deliberate user-name collisions.

Readonly default encoder parameters retain the concrete scalar representation.
The C-valid `int[]` to `byte[]` call reads the first logical-length bytes of the
integer storage; narrowing each integer would produce different bytes. Rust
uses native-endian integer object bytes without adding unsafe code. The proof
excludes callbacks and other mutable array operations; it does not solve the
general array/callable aliasing ABI. Native returned byte arrays also accept
wider unmanaged scalar storage and read its byte prefix rather than panicking
on a non-unit element size. Managed elements and dynamic header mutation remain
required ownership work.

Synchronized array captures copy the array while holding its lock, release that
lock, and use an ordinary captured snapshot. Nested captures propagate the
snapshot representation. Indexed writes within a captured closure persist
across its calls while leaving the outer synchronized array unchanged.

`make test-rust-parity-byte-encoding` verifies nine sources in 81 source-identical
pairs across O0/O1/O2 and default/checked/unchecked arithmetic. Independent
Python oracles check all 256 byte values, Base64, Latin-1/control bytes, NUL,
receiver effects, closure state, and integer object prefixes. The native C
reference independently checks the prefix ABI. Windows checks require the
actual CRT CRLF translation. All nine C programs pass ASAN with leak detection.
Six new generated-source/runtime regressions are added. Only the historical
`array_join_hygiene` Rust snapshot changes, after its unchanged source and output
passed all nine differential cases and C ASAN.

| Local gate | Result |
|---|---:|
| Rust generation / negative | 374 / 158 passing, no skips |
| Native tagged / extra / origin / negative | 8 / 19 / 1 / 4 passing |
| Closures positive / negative | 36 / 1 passing |
| Concurrency positive / negative / promoted | 10 / 7 / 1 passing |
| Rust toolchain | 12 passing |
| C unit / cgen / model | 1610 / 107 / 79 passing |
| C integration / negative / exploratory / exploratory negative | 1141 / 58 / 224 / 11 passing |
| Differential reports | 18 reports, 1263 cases: 1224 positive + 39 diagnostic/order |
| Raw-byte transport / Windows transport / helper simulation | 63 / 6 executions, helper passing |
| Original source hashes | 1365 unchanged |
| Original Rust integration / exploratory | 1064 / 212 passing; 77 / 12 compilation gaps |

The original `test_byte_encoding` now passes. The remaining original corpus has
89 compilation gaps, with no new runtime failures or skips. Hosted CI
is verified on the exact direct-main revision in the following section.
The PR queue is empty; C remains the default target and C production, sources
and oracles are unchanged.

Excluded characterization is retained explicitly: eight other scalar-array
conversions are rejected by the shared frontend in these contexts, and named
array-function-value probes crash restored C under ASAN. These are not positive
parity results. The shared formatter truncates very long literal lines; the new
domain fixtures use short loops instead. Broader array identity/mutation,
callbacks, ownership/lifetimes, native/SDK and other language gaps remain part
of the full goal.

Evidence: [byte-encoding-validation.json](rust-parity-evidence/byte-encoding-validation.json),
[byte-encoding-pairs.json](rust-parity-evidence/byte-encoding-pairs.json),
[byte-encoding-asan.json](rust-parity-evidence/byte-encoding-asan.json),
[byte-encoding-type-admission.json](rust-parity-evidence/byte-encoding-type-admission.json),
and [byte-encoding-excluded-callables-asan.json](rust-parity-evidence/byte-encoding-excluded-callables-asan.json).


## Byte-encoding main: hosted verification, 2026-10-03

Main revision `878381a7157a1ceeb221d2165f6eb24b9f006239` passes all six jobs:
[Compiler CI 37156133811](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37156133811)
and [Rust Runtime CI 37156133845](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37156133845).
Downloaded Linux, macOS and Windows artifacts verify all 18 reports and 1263
cases per platform, including 81 independent byte-encoding and 72 character/CRT
raw-output oracles. Windows CRLF translation, source hashes and a single
compiler binary per platform are verified. Complete hosted C and dedicated Rust
suite counts have no failures or skips. The PR queue is empty; 89 original
compilation gaps and broader ownership/native/language work remain. Evidence:
[byte-encoding-main-ci-green.json](rust-parity-evidence/byte-encoding-main-ci-green.json).

Follow-up C controls are prepared for the next increments: unchanged `exit()`
status/output in nine modes; native buffered output, once-only exit arguments,
exit-code conversion and LIFO `atexit` callbacks in 54 cases; and unchanged
statement-return/character-value match fixtures in 18 cases. Rust rejects these
programs, so the controls add no parity credit. An additional readonly
same-array/two-encoder-parameter probe passes all nine C/Rust pairs; it is not
part of the mandatory 81-case gate and does not settle general mutable aliasing.


## Process exit and native buffered streams: local validation, 2026-10-03

Rust now emits the builtin `exit(int)` through the same CRT exit bridge used by
assertions and integer `main` results. The argument evaluates once and narrows
to the target C `int`, matching `sn_exit(int)`. The process terminates, buffered
C streams are flushed, and registered C exit callbacks run in LIFO order.
Ordinary functions, methods/static methods, closures, value-match prefixes and
worker-thread exits are checked without changing the shared frontend or C.

A new native callback probe exposed an existing stream-order mismatch: Rust
forced a flush after native calls and native global initialization, while C did
not. Since Rust printing already uses the C streams, those extra flushes caused
buffered stdout to appear before C's stderr callback trace. Automatic native
flushes are removed; source-requested flushes and CRT termination retain their
normal effect. The existing Windows transport helper's comment is corrected;
its sources and output oracles are unchanged.

`make test-rust-parity-exit` verifies 12 sources and 17 distinct invocations in
153 independent cases across O0/O1/O2 and default/checked/unchecked arithmetic.
Separate and merged streams require exact output, status and order: 612 target
executions. Native C and Python C-int references check zero, one, 255, 256,
negative one and a value wider than C `int`, without assuming that Windows and
Unix report the same numeric OS status. Two unchanged original programs,
`test_exit` and `test_static_import`, are included. The latter's imported string
mutation and termination now work as well. All 12 C sources pass ASAN with leak
detection at their required exit statuses, and callback stderr contains only
the specified trace. Eight new Rust snapshot/runtime regressions are added;
no historical Rust snapshot is changed.

| Local gate | Result |
|---|---:|
| Rust generation / negative | 382 / 158 passing, no skips |
| Native tagged / extra / origin / negative | 8 / 19 / 1 / 4 passing |
| Closures positive / negative | 36 / 1 passing |
| Concurrency positive / negative / promoted | 10 / 7 / 1 passing |
| Rust toolchain | 12 passing |
| C unit / cgen / model | 1610 / 107 / 79 passing |
| C integration / negative / exploratory / exploratory negative | 1141 / 58 / 224 / 11 passing |
| Differential reports | 19 reports, 1416 cases: 1224 positive + 39 diagnostic/order + 153 process-exit |
| Raw-byte / Windows transport / helper simulation | 63 / 6 executions, helper passing |
| Original source hashes | 1365 unchanged |
| Original Rust integration / exploratory | 1066 / 212 passing; 75 / 12 compilation gaps |

There are now 87 original compilation gaps, with no new runtime failures or
skips. The exact direct-main revision still requires hosted verification after
this local increment. C remains the default target and the PR queue is empty.

Full parity remains incomplete. A separate C-valid normal return from void
`main` still flushes stdout before C exit-callback stderr in Rust; its source
and raw trace are retained as required follow-up, not positive parity. The
entrypoint repair must preserve source-level calls, recursion and function
values of `main` along with cleanup/return behavior. Broader array/reference
identity, callbacks, native/SDK, matching, nil and other language work remains.

Evidence: [builtin-exit-validation.json](rust-parity-evidence/builtin-exit-validation.json),
[builtin-exit-pairs.json](rust-parity-evidence/builtin-exit-pairs.json),
[builtin-exit-asan.json](rust-parity-evidence/builtin-exit-asan.json),
[builtin-exit-native-order-before.json](rust-parity-evidence/builtin-exit-native-order-before.json),
and [normal-main-return-callback-order-before.json](rust-parity-evidence/normal-main-return-callback-order-before.json).


## Process-exit main: hosted verification, 2026-10-04

Main revision `89de39597088c8e8cd353e9cf8c29d706d78ec6d` passes all six jobs:
[Compiler CI 37159722402](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37159722402)
and [Rust Runtime CI 37159722372](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37159722372).
Downloaded Linux, macOS and Windows artifacts verify all 19 reports and 1416
cases per platform. All 153 process-exit cases match independent status and raw
stream oracles, including Windows C-int/OS status conversion and CRLF output.
The existing 81 byte and 72 character oracles also pass independently. Full
hosted C/Rust suite counts have zero failures or skips; source hashes and one
compiler binary per platform are verified. The PR queue is empty; 87 original
compilation gaps and broader ownership/native/language work remain. Evidence:
[builtin-exit-main-ci-green.json](rust-parity-evidence/builtin-exit-main-ci-green.json).


## Normal main return: local validation, 2026-10-04

The restored C entrypoint flushes stdout at fallthrough. Explicit source
returns bypass that footer; CRT exit then runs native callbacks before the
remaining buffered output is flushed. Rust's previous scope guard flushed all
C streams on every void-main exit, so early returns placed stdout before
callback stderr. Integer-main fallthrough also omitted C's explicit flush.

The Rust main template now emits a stdout-only flush at fallthrough, including
integer-main fallback. The void-main scope guard binding is removed. Early
returns retain ordinary Rust cleanup and leave C stream flushing to CRT exit.
Source-main calls retain their existing function behavior; the fix adds no
entrypoint wrapper or return interception. The compiler executable and C/shared
frontend are unchanged; validation records the staged template hashes.

`make test-rust-parity-main-return` checks seven gate sources and
twelve invocations in 108 cases across all nine modes: 432 target executions.
Independent oracles distinguish fallthrough from early returns and check
native initialization, argument count, helper-name hygiene, C-int result
narrowing, once-only result evaluation, LIFO callbacks, separate streams and
merged stream order. All 108 C controls pass; before the fix, Rust passed 81
and failed exactly 27 ordering cases. After the fix, all 108 pairs pass. Seven
C sources and twelve invocations pass ASAN with leak detection and exact
required statuses/stdout/stderr.

All 435 Rust snapshots are changed mechanically: remove the guard binding from
426 void mains and add the stdout-only flush to main fallthrough, including
nine integer mains. Every other byte is preserved, with original/current
hashes and resolved hygienic helper names retained. Complete C/Rust suites,
existing 19 differential reports, 63 raw-byte executions, six Windows transport
executions and the helper simulation pass. There are now 20 reports and 1524
cases: 1224 positive pairs, 39 diagnostic/order, 153 process-exit and 108
normal-return cases. All 1365 original source hashes are unchanged; the Rust
corpus still passes 1066 integration and 212 exploratory cases, with the same
75 and twelve compilation gaps, zero new runtime failures and no skips.

Argv indexed-string printing followed by normal cleanup double-frees in C;
both early-return and fallthrough ASAN probes are retained separately and
receive no parity credit. The valid argv control reads the argument count.
All 36 attempted C source-main recursion/function-value controls fail to link,
so they are excluded from C-valid parity. Existing direct void-main recursion
still passes nine Rust preservation checks. The forced-Windows helper
simulation removes the actual main CRT flush footer because a forced cfg on
Linux cannot provide the Windows CRT; it verifies only the byte adapter, while
the hosted gate must verify real ABI/lifecycle behavior.

Hosted verification is required after direct-main integration. Full parity
remains incomplete: 87 original compilation gaps plus broader array/reference,
callable, native/SDK, matching, nil and language families remain.

Evidence: [main-return-validation.json](rust-parity-evidence/main-return-validation.json),
[main-return-pairs.json](rust-parity-evidence/main-return-pairs.json),
[main-return-before.json](rust-parity-evidence/main-return-before.json),
[main-return-asan.json](rust-parity-evidence/main-return-asan.json),
[main-return-snapshot-review.json](rust-parity-evidence/main-return-snapshot-review.json),
[main-return-c-undefined-args-early.json](rust-parity-evidence/main-return-c-undefined-args-early.json),
[main-return-c-undefined-args-indexed.json](rust-parity-evidence/main-return-c-undefined-args-indexed.json),
[main-return-source-main-excluded.json](rust-parity-evidence/main-return-source-main-excluded.json),
and [main-return-recursion-preserved.json](rust-parity-evidence/main-return-recursion-preserved.json).


## Normal-return main: hosted verification, 2026-10-04

Main revision `aaf14d36147b7a3eddf09bb04e36ef8cf055eebf` passes all six jobs:
[Compiler CI 37161893730](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37161893730)
and [Rust Runtime CI 37161893778](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37161893778).
Downloaded Linux, macOS and Windows artifacts verify all 20 reports and 1524
cases per platform. The 108 normal-return cases match independent status,
separate stream and merged-order oracles, including Windows CRLF and C-int/OS
status conversion. Existing independent process-exit, byte and character
oracles also pass. Source hashes and a single compiler binary per platform are
verified; complete hosted C/Rust suite counts have zero failures or skips.
The PR queue is empty, and C remains default. There are still 87 original
compilation gaps plus broader ownership/native/language work. Evidence:
[main-return-main-ci-green.json](rust-parity-evidence/main-return-main-ci-green.json).

The next match controls are prepared without changing original sources or
claiming parity: enclosing-function return arms, character-valued matches over
all 256 byte values, and character-subject matches over the same domain. Four
sources pass 36 independent C status/raw-output checks and four C ASAN/leak
controls; Rust rejects all. The projected model shows that an implicit outer
return wraps a void statement match whose arms return from the enclosing
function. The repair must preserve that control flow rather than treating the
arms as ordinary value-result expressions. Character subjects/results require
their own admission and byte-semantics validation. General mutable array
identity, nil/allocated-empty distinctions, native/SDK and other language
families remain required work.

## Match control flow, character subjects/results and local atomic locks

Verified against main `aaf14d36147b7a3eddf09bb04e36ef8cf055eebf`, with direct-main
integration after all final local tests pass. The before evidence contains 117
independently checked C status/raw-output controls, all rejected by Rust. The
completed gate has 20 sources and 180 C/Rust pairs with independently specified
oracles across default/checked/unchecked arithmetic and O0/O1/O2.

The private Rust validator now handles a void statement match specifically in
return contexts. A value-returning callable evaluates that match as a statement;
its returning arms leave the enclosing function, method or lambda. Callable
scopes are tracked separately, and an unreachable fallback satisfies Rust's
value type only for paths lacking a defined C return value. Void-valued value
initializers continue to receive their existing structural diagnostics.

Character value matches support byte-valued results and literal character
subjects. Ordinary character postfix updates wrap at eight bits and return
the previous value; computed places reuse the existing index/owner lowering.
Full byte domains, nested indices and fields, private captured arrays and C's
negative-index length read are covered. Value-match arm prefixes may contain
validated declarations, conditions, loops, locks, return, break and continue;
exact result/tail and supported-feature validation remains in place.

A real lock-prefix control exposed a Rust deadlock: local atomic postfix
updates incorrectly reacquired the separate explicit-lock gate. C's live-symbol
postfix path gates globals, while local atomic postfix directly updates storage.
The Rust projection now follows that distinction. Explicit local locks and
existing global/thread character controls verify old/new values and wraparound.
Six existing Rust snapshots remove only ten such local gate acquisitions;
all other bytes and every existing runtime oracle are preserved.

| Final local evidence | Result |
|---|---|
| Rust generated / negative | 397 / 151, all pass; no skips |
| Rust native tagged / extra / origin / negative | 8 / 19 / 1 / 4, all pass |
| Closures positive / negative | 36 / 1, all pass |
| Concurrency positive / promoted / negative | 10 / 7 / 1, all pass |
| Toolchain | 12, all pass |
| C unit / generation / model | 1610 / 107 / 79, all pass |
| C integration / negative / exploratory / negative | 1141 / 58 / 224 / 11, all pass |
| Runtime reports | 21 reports / 1704 cases, all pass |
| New independent match controls | 180 paired cases; 20 sources |
| C ASAN / leaks | 20 instrumented controls, all clean |
| Original Rust integration / exploratory | 1067 / 215 passing; 74 / 9 compilation gaps |
| Original source preservation | All 1365 hashes unchanged |

The hexadecimal-character pattern probe is retained verbatim as `.sn.raw`
because the shared formatter corrupts that literal. A distinct ordinary fixture
covers an ASCII pattern across the same complete character subject domain.
The initial ad-hoc void-capture and implicit exhaustive-bool-return candidates
were rejected by the authoritative C frontend/compiler and are excluded; the
accepted controls use explicit parameters and a defined fallback statement.
An initial concurrent test attempt lost temporary files to a runner cleanup;
those results were discarded. Final suites use cleanup-disabled runners and
one final staged compiler. Neither guards nor undefined fallthrough earn credit.

Evidence: [validation](rust-parity-evidence/match-control-validation.json),
[raw pairs](rust-parity-evidence/match-control-pairs.json),
[original before controls](rust-parity-evidence/match-control-original-before.json),
[expanded before controls](rust-parity-evidence/match-control-expanded-before.json),
[ASAN controls](rust-parity-evidence/match-control-asan.json),
[snapshot proof](rust-parity-evidence/match-control-snapshot-review.json) and
[remaining source diagnostics](rust-parity-evidence/match-control-gap-diagnostics.json).
The 83 remaining failures are source-level compilation gaps, not 83 distinct
features; native structs/callbacks/SDK and general owning-handle/reference
semantics remain the largest unresolved groups.

### Match-control hosted closeout

Main revision `f0e3d43bce63eb97183e31581bef5e8ecb177590` passes all six jobs:
[Compiler CI 37165654983](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37165654983)
and [Rust Runtime CI 37165654984](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37165654984).
Downloaded Linux, macOS and Windows artifacts independently verify all 21
reports and 1704 cases per platform, including the new 180 match-control cases.
Status, exact raw output, character byte domains, Windows CRLF transport,
mutation values and control-flow effects match their fixed/Python oracles.
Source hashes and compiler provenance are verified. Complete hosted C/Rust
suite counts have no failures or skips. The PR queue is empty; C remains default.
Proof: [match-control-main-ci-green.json](rust-parity-evidence/match-control-main-ci-green.json).

The next aggregate-size controls preserve three original sources and add
padding/field-order, nested/managed-handle and operand-non-evaluation probes.
All six sources pass 54 independent C mode/optimization controls and six
instrumented ASAN/leak checks, while Rust rejects every case. They add no parity
credit. For example, the shared model reports Point size 24, whereas the
unchanged C output and native ABI layout give 16. A Rust repair must reproduce
C's actual type layout instead of trusting bookkeeping sizes or Rust owning
storage sizes, and must keep sizeof operands unevaluated. General ownership,
native/SDK and the 83 original compilation gaps remain required work.

## Aggregate sizeof and C-local native aggregates: local validation, 2026-10-04

Rust computes aggregate `sizeof` using private, never-instantiated `repr(C)`
layout types. Ordered scalar fields retain C widths, characters occupy one byte,
and strings, arrays, callable/interface fields and reference structs use C
pointer representations. Nested values recurse through their C layouts; packed
values use `repr(C, packed)`. The shared model's size/offset bookkeeping is not
used: its Point metadata reports 24 bytes where authoritative C reports 16.
Rust's owning runtime storage is also independent of these layout proxies.
`sizeof` operands remain unevaluated, including calls and member receivers.
Generated helper names are reserved against source identifiers and reused per
declared type. A fixed-point scan of the partitioned Rust roots preserves every
runtime-needed declaration and its field/method dependencies; declarations used
only for size queries or native C bodies need no Rust value representation.

Native functions can now construct, mutate and operate on aggregates wholly
inside their projected C bodies, while their public arguments/results retain the
existing supported ABI. Owned string and byte-array field results are checked.
The first corpus run exposed an owning native method-chain leak. Standalone C
already lifted seven intermediate Vec2 results into cleanup-bearing temporaries;
the Rust C projection omitted that pass. Calling the unchanged authoritative
`gen_model_flatten_chains` pass before splitting restores those lifetimes. Both
targets now pass the original chain under instrumented ASAN with leak detection.
The [pre-fix comparison](rust-parity-evidence/sizeof-reference-chain-pre-fix-asan.json)
and [seven repaired native pairs](rust-parity-evidence/sizeof-native-asan.json)
retain the evidence.

The 216-case gate covers eight unchanged originals, four byte-preserved promoted
negative sources, padding/ordering/nesting, packed and pointer-sized reference
layouts, managed fields, non-evaluation and helper-name collisions. Its layout
oracles use Python ctypes and fixed independently checked output contracts;
checks require exact raw output, successful statuses, source hashes and complete
mode/optimization coverage. Four scratch fixtures were formatted on promotion
and revalidated; they are not claimed as byte-preserved historical sources.
Standalone function/interface/nil/void sizeof and general aggregate transport
remain work.

| Local check | Result |
|---|---|
| Runtime reports | 22 / 1920 cases, all pass |
| New independent sizeof gate | 216 / 216 pass |
| Rust generation / negative | 407 / 147 pass |
| Native tagged / extra / origin / negative | 8 / 22 / 1 / 4 pass |
| Closures positive / negative | 36 / 1 pass |
| Concurrency / promoted / negative | 10 / 7 / 1 pass |
| Rust toolchain | 12 pass |
| C unit / cgen / model | 1610 / 107 / 79 pass |
| C integration / negative / exploratory / negative | 1141 / 58 / 224 / 11 pass |
| Original Rust integration / exploratory | 1075 / 215 pass; 66 / 9 compilation gaps |
| C instrumented ASAN | 24 clean sources |
| Native C/Rust instrumented ASAN | Seven clean pairs |
| Existing Rust snapshots | 450 byte-identical; ten new snapshots |
| Original corpus source hashes | 1365 unchanged |
| Formatter / raw-byte / Windows transport / helper checks | Pass |
| Open PR queue | Empty |

Eight newly passing original sources are stack/heap struct allocation,
struct sizeof/equality, comprehensive sizeof, packed declarations, struct code
generation validation, native defaults and native reference method chaining.
All remaining original failures are compilation gaps, not distinct feature
counts or established C-defined obligations. No guards or skips receive credit.
Hosted checks completed successfully for the integrated sizeof implementation.

## Aggregate sizeof: hosted verification, 2026-10-04

Main implementation revision `8cb11594f577c7c497244bc549c9000dca5566a2` passes
all six jobs in
[Compiler CI 37169311621](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37169311621)
and [Rust Runtime CI 37169311620](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37169311620).
Retained Linux/macOS/Windows artifacts each verify all 22 reports and 1920
cases, including 216 independent sizeof cases. Every report uses the one
recorded compiler binary for its platform; source hashes, exact raw outputs,
statuses and complete mode/optimization identities are checked. Windows
comparisons enforce the platform's actual CRLF text transport.

The exact-revision proof is in
[sizeof-main-ci-green.json](rust-parity-evidence/sizeof-main-ci-green.json).
Complete hosted C suites retain counts 1610 / 107 / 79 / 1141 / 58 / 224 / 11;
Rust generation/negative are 407 / 147, native tagged/extra/origin/negative
8 / 22 / 1 / 4, closures 36 / 1, concurrency 10 / 7 / 1 and toolchain 12.
All have zero failures/skips. Remote main still equals the verified revision
and the PR queue is empty. The final hosted proof and ledger closeout are
retained locally for inclusion with the next tested implementation increment.

Full parity remains required: 75 original compilation gaps, wider array and
callable identity/mutation/lifetimes, native/SDK and language features. C-defined
controls for the remaining sizeof categories, shared reference parameters and
nil storage have been prepared; no additional production changes are included
in this closeout. Guards, dead-code-eliminated feature queries, skipped tests
and C-undefined probes receive no feature-completion credit.

## sizeof/reference calls: hosted verification, 2026-10-04

Implementation revision `1c192f1d9cbf557e5587089eadd2568a148f7d02` passes all six
jobs in [Compiler CI 37173059553](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37173059553)
and [Rust Runtime CI 37173059587](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37173059587).
Linux/macOS/Windows retained artifacts each verify all 23 reports and 2046 cases,
including the 126 independent sizeof/reference cases. Compiler hashes, source
hashes, statuses, raw outputs and complete mode/optimization identities are
checked; Windows uses actual CRLF text transport. Complete hosted C suite counts
are 1610 / 107 / 79 / 1141 / 58 / 224 / 11. Rust generation/negative are 419 / 143,
native tagged/extra/origin/negative 8 / 23 / 1 / 4, closures 36 / 1, concurrency
10 / 7 / 1 and toolchain 12, all with zero failures/skips. Remote main equals the
verified revision and the PR queue is empty.

The proof is [call-reference-main-ci-green.json](rust-parity-evidence/call-reference-main-ci-green.json).
The hosted closeout is retained for the next meaningful tested implementation
commit. Full parity remains required; 74 original compilation gaps and wider
language, array/reference/callable and native/SDK obligations remain. C-defined
nil and indexed string-result controls are prepared independently; C-invalid
probes are preserved separately and receive no parity credit.
