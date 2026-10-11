# Rust backend completion ledger

Goal: verified feature and behavioral parity with C on integrated main using the
shared C runtime and mixed-language package architecture, preserving the language
contract, C-backed SDK and C as the default target. Required scope and acceptance
are recorded in [the Rust completion goal](rust-completion-goal.md). Rejections are
remaining implementation work, not parity. This ledger supersedes historical
head counts, not the language specification or the historical evidence itself.

## Typed canonical arrays and reusable SDK artifacts: local validation, 2026-10-11

ABI 1.7 adds typed canonical scalar/string arrays with rank 1..32. Borrowing
and owned transfer retain actual C headers, slots, aliases and native hooks;
copying fills missing string/nested copy hooks with independent canonical storage.
Original SDK Environment storage now has a durable lifetime control. C native
providers and C Sindarin facade exports support these contracts for C/Rust apps.
Rust/Go generic native providers and Rust generic body exports remain unsupported
and explicitly diagnosed; the runtime clients alone do not establish those paths.

The reusable SDK stager builds nine original public modules as 18 C/SN units,
with 197 exports, preserving the actual C backing implementations and public
Sindarin modules. The original Bytes, TextFile, BinaryFile, Path, Directory,
Environment, OS and Crypto callers/oracles pass 288 combinations: C/Rust,
source/sealed artifacts, O0/O1/O2, default/checked/unchecked. Sealed consumption
removes backing C sources. Stdio is built but its interactive caller is outside
this check. This remains a partial SDK migration, not whole-SDK acceptance.

The original Path caller exposed invalid Rust length-cast syntax before `<`.
The lowering now parenthesizes that operand; a regression verifies single
function evaluation for string and array lengths across 18 C/Rust mode cases.
Nested int32 array adapters pass 36 source/sealed C/Rust mode cases normally
and under strict address/undefined/leak checking. Runtime ABI checks pass 15
C/Rust/Go clients and seven instrumented C clients. Full C/Rust suites pass
3,242/1,097 checks; existing artifact/contract/CI helper checks pass 20/15/22.
The final complete package import suite passes all 48 tests.
The CI inventory becomes 301 gates in 26 groups, including the SDK check on
Linux/macOS/Windows. Hosted acceptance is pending publication of this batch.

Whole SDK migration, wider record/interface/generic and provider contracts,
dependency lifecycle graphs, final original-corpus mode/platform review and
final integrated hosted acceptance remain required. See [ABI documentation](runtime-abi.md),
[SDK staging](native-artifacts.md#staging-the-canonical-sdk) and the retained
[typed-array validation](rust-parity-evidence/typed-native-array-validation.json).

## Windows path handling: required CI correction, 2026-10-11

Compiler revision `fb1e8cc6` failed Windows core validation in
[CI 38111625738](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38111625738).
The root native-contract diagnostic changed because `_fullpath` normalizes even
missing declaration paths. The importer then skipped that nonexistent API as an
unloaded module and failed later on a missing backing source. Declaration path
resolution now requires an existing regular file. A directory at the declaration
path reproduces this bug on POSIX; all six C/Rust normal/source/model controls
fail before the repair and preserve the required diagnostic afterward.

Namespace revision `3d059a09` also failed Windows native validation in
[CI 38112343192](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38112343192).
Its private C projection prepended source_dir to an absolute drive-qualified
header, producing a doubled drive path. Absolute POSIX, drive-qualified and UNC
header names now remain absolute. A POSIX shadow header at the wrongly joined
path reproduces this failure locally; the exact namespace controls retain
their original outputs and pass after the repair, normally and under sanitizers.

Integrated correction `0d802850` passes all 34 jobs in
[CI 38113328488](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38113328488).
Coverage confirms 26 groups and 298 gates, using one compiler hash per platform.
This accepts the path repairs, method qualifier repair, independent API imports
and empty native namespaces together. Feature publication is open again.
Complete local C/Rust suites pass 3,242/1,097 checks; package imports pass 46,
runtime contracts 15 and CI helpers 22. Twelve ABI clients and five sanitized C
ABI clients pass. Namespace path controls pass 27 normal and strict mode cases.
Evidence: [combined correction validation](rust-parity-evidence/windows-path-correction-validation.json)
and [hosted acceptance](rust-parity-evidence/windows-path-correction-ci-green.json).
The typed-array prototype from stash `e321f77` has been applied locally and
extended; its current feature batch is described above and remains separate
from this correction's hosted acceptance.
Complete SDK, wider ABI/package contracts and final corpus acceptance remain
required.

## Empty native static namespaces: local validation, 2026-10-11

Rust now retains empty native static method owners such as Crypto and OS without
requiring a foreign record value contract. Ordinary static bodies and native
methods keep their original implementation; zero-field language sizeof remains
the C result, zero. Empty record inputs/results still require wider ABI work.
The existing declaration forwarding machinery now also materializes native
namespace methods from C headers, preserving header-only inline definitions.

Three focused controls cover 27 case/mode combinations and 54 C/Rust compilations
for ordinary/native methods, exact sizeof and independent owned byte results.
All pass normally and under strict address/undefined/leak checking. The unchanged
SDK Crypto and OS callers pass 36 C/Rust checks across all nine modes. Existing
native value/reference record gates pass 45/36 independent mode cases. Complete
C/Rust suites pass 3,242/1,097 checks and CI-helper tests pass 22. A dedicated
Linux/macOS/Windows gate brings the coverage inventory to 298 gates in 26 groups.
Original SDK implementations, public modules, callers and oracles are unchanged.
Hosted acceptance requires this feature's own complete CI after publication.
The existing Bytes/TextFile/BinaryFile artifact-import controls also pass.
Evidence: [namespace validation](rust-parity-evidence/empty-native-namespace-validation.json).

The SDK artifact builder remains an unpublished prototype. Wider nested array,
record/interface and complete SDK contracts, dependency lifecycle graphs and
final original-corpus platform/mode acceptance remain required.

## Method qualifiers and independent API modules: local validation, 2026-10-11

Broader SDK artifact construction exposed two compiler defects. The struct type
constructor omitted each method's return memory qualifier, leaving the package
validator dependent on uninitialized storage. A poisoned-arena regression fails
before the repair and verifies default, borrowed and owned return qualifiers
afterward. The constructor now preserves the declared qualifier.

The application importer also required bindings from every API module in a
package to resolve in the application's AST, even when only one module was
imported. It now prepares adapters for loaded declaration files; the independent
provider build still validates all package exports. Source/prebuilt C/Rust
controls import either of two APIs alone or both with aliases, and preserve the
missing-callable diagnostic for a loaded API. All 12 combinations and both
negative controls pass normally and under strict address/undefined/leak checks.

Complete C/Rust suites pass 3,242/1,097 checks; the complete import suite passes
46 tests, artifact tests pass 20 and CI-helper tests pass 22. Three package
controls also pass with `MALLOC_PERTURB_=165`. Original DSL callers, behavioural
oracles and source snapshots are preserved. Hosted compiler acceptance is pending.

The SDK TextFile header now permits public-before-standalone inclusion with
independent guards, verified by the native client and repeated includes. SDK
`89f3b83` passes all four jobs in
[CI 38110801585](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/38110801585).
Normal/sanitized SDK checks pass 26/3 and the rebuilt legacy suite passes all 28.
Compiler CI pins this accepted revision.

An unpublished builder prototype stages eight unchanged SDK public modules,
their existing C implementations and generated forwarding wrappers. It builds
16 C/SN units with 188 exports. C Crypto/Directory/OS and Rust Directory callers
pass their original oracles locally. Rust Crypto/OS still reject empty native
static namespaces. The original Path fixture imports Environment, whose nested
string-array API needs wider contracts before the artifact can include it.
These remain implementation work; the prototype is not complete SDK acceptance.
The complete SDK, broader contracts and final original corpus/platform/mode
acceptance remain required. Evidence:
[TextFile header acceptance](rust-parity-evidence/sdk-textfile-header-order-ci-green.json).
[Compiler metadata validation](rust-parity-evidence/package-metadata-validation.json)
retains the failing-before controls and successful complete local suites.

## Native entrypoint parity: hosted acceptance, 2026-10-11

Compiler main `a182e350` passes all 34 jobs in
[CI 38108965543](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38108965543).
Retained coverage verifies 26 groups, 295 gates and one compiler per platform.
This accepts the entrypoint repair and its integrated lifecycle checks on
Linux/macOS/Windows. Evidence:
[native entrypoint acceptance](rust-parity-evidence/native-entrypoint-ci-green.json).

## BinaryFile SDK artifacts: hosted acceptance, 2026-10-11

Compiler main `89c9ce1d` passes all 34 jobs in
[CI 38110035844](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38110035844).
Retained coverage verifies 26 groups, 295 gates and one compiler per platform.
This accepts BinaryFile's unchanged facade, original caller/oracle, canonical
record/byte transport and mixed source/prebuilt package controls on
Linux/macOS/Windows. Evidence:
[BinaryFile compiler acceptance](rust-parity-evidence/sdk-binaryfile-artifact-ci-green.json).

## BinaryFile SDK artifacts: local validation, 2026-10-11

The actual SDK BinaryFile module now builds independently with canonical C
record storage and atomic lifecycle functions. Its existing file operations and
public Sindarin module remain the implementation. Compatibility checks retain
generated-record layout for legacy C callers. Separate helper backing names
allow generated exports to preserve the original C symbols used by the facade.
SDK `6e39091` passes all four jobs in
[CI 38109720921](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/38109720921),
including the complete 28-test SDK suite and standalone module on all platforms.
Compiler CI pins that accepted revision.

The first SDK BinaryFile run failed only its Windows nil-path stderr oracle:
exit 1 and the exact original error text were correct, but C emitted CRLF where
the checker expected LF. Correction `83b9d7c` preserves each platform's exact
newline and passes all four jobs in
[CI 38109554080](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/38109554080).
No original SDK caller, public Sindarin source or expected output was changed.

Compiler controls cover 90 C/Rust build/run combinations over all nine
optimization/arithmetic modes. The unchanged SDK fixture consumes source-built
and sealed prebuilt C native/facade libraries after backing sources are removed.
Additional controls verify shared file aliases, live readInto mutation observed
through borrowed buffer aliases, independence of ordinary array copies, repeated
disposal, exposed fields and sizeof, owned paths/names, helper exports and byte
results surviving file closure. Mixed applications combine the SDK with
Rust-native, Go-native and pure Sindarin RS packages, then consume their sealed
archives after backing sources are removed. Normal and strict
address/undefined/leak checks pass locally; hosted compiler acceptance is pending.
The sizeof language value is the as-ref pointer width; canonical record storage
layout is verified separately by generated C assertions.

Standalone SDK normal/sanitized validation passes 26/3 checks. The complete
artifact import suite passes 45 tests and 22 CI-helper tests pass. Eighteen
additional strict case/mode controls verify exposed pointers and exact sizeof
after the 90-case baseline. The complete SDK,
wider record/interface/package contracts and final original-corpus/platform/mode
acceptance remain required. Evidence:
[SDK module acceptance](rust-parity-evidence/sdk-binaryfile-module-ci-green.json),
[Windows oracle correction](rust-parity-evidence/sdk-binaryfile-oracle-ci-green.json),
[artifact and lifecycle validation](rust-parity-evidence/sdk-binaryfile-artifact-validation.json).

## Native entrypoint parity: local validation, 2026-10-11

Native source `main` bodies now use a private C callable and an ordinary Rust
process entry wrapper. This avoids conflicting process symbols and preserves the
native body, command-line arguments, initialization and integer exit conversion.
Native argv uses canonical C array storage. The private C projection now retains
C's borrowed-string print annotations so printing a slot or field cannot free it.
Void early returns stay void in the private native implementation.

Eleven controls cover 99 case/mode combinations and 198 C/Rust compilations,
including void/int/negative/wide exits, repeated argument reads and mutation,
initialization, generated-name collisions, recursion, string fields and ordinary
main forwarding argv to native functions. All pass normal and strict
address/undefined/leak validation. Complete C/Rust suites pass 3,241/1,097 checks;
original sources/oracles/snapshots are preserved. A dedicated CI gate runs these
controls on Linux/macOS/Windows. Hosted acceptance is pending in
[CI 38108965543](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38108965543)
for compiler main `a182e350`.

The complete SDK, wider record/interface/package contracts and final original
corpus/platform/mode acceptance remain required.

## Byte arrays and unchanged Bytes facade: hosted acceptance, 2026-10-11

Compiler main `f727be87` passes all 34 jobs in
[CI 38106995022](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38106995022).
Retained coverage verifies 26 groups, 292 gates and one compiler per platform.
This accepts byte transport/providers, mixed C/RS body guards and the unchanged
SDK Bytes facade with the accepted SDK dependency. Evidence:
[byte artifact acceptance](rust-parity-evidence/native-byte-array-adapter-ci-green.json).

## Parameter qualifiers and TextFile facade: hosted acceptance, 2026-10-11

Corrected integrated compiler main `5c8f1733` passes all 34 jobs in
[CI 38104419931](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38104419931).
The retained coverage verifies 26 groups, 292 gates and one compiler per
Linux/macOS/Windows platform. This accepts the qualifier repair and integrated
unchanged TextFile facade work. Evidence:
[complete correction acceptance](rust-parity-evidence/parameter-sync-ci-green.json).

Byte-array ABI/adapters and the unchanged Bytes facade have hosted acceptance above.
SDK `775fb67` adds the standalone C module and native check to `make test`.
Its Windows CI failed because the checker assumed resources beside an installed
copied executable. SDK `a8d378f` followed the compiler's installed resource lookup.
That correction still failed because the exact installed release contains
`sn.windows.cfg`, not the current compiler's generic config marker. SDK `7115937`
validates actual runtime/header artifacts in both resource locations; lookup was
verified against the exact v0.0.83 Windows release archive. All four jobs in
[SDK CI 38106674043](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/38106674043)
now pass, including the standalone C module on Linux/macOS/Windows. Compiler CI
pins this exact accepted revision. Evidence:
[SDK module acceptance](rust-parity-evidence/sdk-bytes-module-ci-green.json).
The compiler byte-feature batch has its own complete hosted acceptance above.

The byte feature passes 3,241 C and 1,097 Rust checks, 20 artifact, 44 import,
14 runtime-contract and 22 CI-helper tests. Twelve ABI clients and five sanitized
C runtime clients pass. Strict package checks cover 108 C/Rust/Go native controls,
72 C/RS mixed-array body controls and 40 unchanged SDK Bytes/helper/namespace
controls (220 total). Source/prebuilt controls remove backing implementations;
public SDK source, callers and existing oracles remain unchanged. Evidence:
[byte adapter validation](rust-parity-evidence/native-byte-array-adapter-validation.json).

The namespace controls exposed a source `native fn main()` gap: C compiled it,
while Rust produced duplicate entry symbols. The native-entry repair above
supersedes that local gap; its hosted acceptance remains separate. Complete SDK,
wider contracts and final original-corpus/platform/mode acceptance remain open.

## Function parameter qualifiers: CI correction, 2026-10-11

Facade revision `e38cc04f` failed Windows core validation in
[CI 38103118038](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38103118038).
The generated native-provider artifact test and the Rust Sindarin mutable-array
package publication test rejected ordinary declarations as reference-qualified.
Linux/macOS core passed; coverage verification and the aggregate correctly failed
on the Windows gate failures. The facade batch is not hosted-accepted.

`ast_create_function_stmt` copied parameter memory qualifiers but omitted
`sync_modifier`, leaving that field uninitialized in arena storage. The new
package validator exposed this defect. Both exact package failures reproduce
locally with `MALLOC_PERTURB_=165`; the focused poisoned-arena regression also
fails before the repair. The constructor now copies the actual synchronization
qualifier. The regression verifies both `SYNC_NONE` and `SYNC_ATOMIC`, preserving
the validator's rejection of unsupported synchronized package inputs.

Complete local C/Rust suites pass 3,241/1,097 checks without failures/skips.
The increase is one deterministic AST regression. Both previously failing package
tests pass with nonzero heap contents. ABI/lifecycle, runtime-contract and CI-helper
checks pass: 20 artifact, 41 import, 14 runtime-contract, 22 CI-helper, ten ABI
clients, four sanitized C ABI clients and C/Rust/Go package lifecycle clients.
The correction awaits its own complete hosted acceptance after publication.
Feature publication remains frozen until corrected integrated main passes all
required Linux/macOS/Windows CI. Unpublished byte-array ABI work is held separately;
the complete SDK, broader contracts and final corpus acceptance remain required.

Evidence: [parameter qualifier correction](rust-parity-evidence/parameter-sync-ci-correction-validation.json).

## Unchanged TextFile facade libraries: local validation, 2026-10-11

SN bindings now resolve public static/instance `Type.method` exports and public
declarations retaining their original bodies. C/RS static libraries preserve
their method owners; C instance libraries use canonical record receivers.
Generated consumers call independent exports and preserve declarations on disk.
Explicit C `provides_sources` mappings are checked against actual include
dependencies and prevent per-application recompilation of owned legacy sources.
C method returns now copy borrowed parameters rather than transferring them.

The actual unchanged SDK TextFile public module, including its original Sindarin
methods, compiles into a C facade archive linked to the original canonical C
module. Its original caller and oracle pass 36 C/Rust target/mode/source-or-sealed
controls with backing C removed for prebuilt consumption. A mixed Rust application
links that SDK archive with an independent pure Sindarin RS HTTP library,
native Rust and native Go, preserving array mutation, file aliases and cleanup.
These controls and C/RS static exports pass strict address/undefined/leak checks.

Complete local suites pass 3,240 C and 1,097 Rust checks without failures/skips.
All 20 artifact, 41 import, 14 runtime-contract and 22 CI-helper tests pass.
Formatting and original source/oracle/snapshot preservation pass. SDK construction
and atomic credit inspection are exported by SDK revision `53a82f0`, accepted by
[SDK CI 38101406664](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/38101406664)
on Linux/macOS/Windows. Compiler CI now pins that exact SDK revision.

This migrates the TextFile module path, not the complete SDK package. Other SDK
modules/types, interface/value/packed/generic contracts, RS body record guards,
reference rebinding, dependency lifecycle graphs and final corpus/platform
acceptance remain required. The compiler facade batch needs separate hosted CI.

Evidence: [facade validation](rust-parity-evidence/unchanged-sdk-facade-validation.json).

## Canonical record artifacts: hosted acceptance, 2026-10-11

Integrated main `effbfcfd` passes all 34 jobs in
[complete unified CI 38100097842](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38100097842).
The retained summary verifies 26 groups, 292 gate executions and one compiler per
Linux/macOS/Windows platform. It accepts the record storage/adapters and prior
SDK declaration-adapter transport; the unchanged facade extension requires its
own validation.

Evidence: [record acceptance](rust-parity-evidence/canonical-record-artifact-ci-green.json).

## Canonical C record artifacts: local validation, 2026-10-11

ABI 1.5 `native.types` binds public native as-ref declarations to package-owned
C storage, constructors and atomic owner functions. Generated C providers and
C/Rust consumers use typed shared-runtime resources, preserving actual pointers,
fields, aliases and native methods. Borrowed results acquire a credit before
input cleanup; record/string guards survive callbacks consuming external credits.
Public C layouts/prototypes and required lifecycle symbols are checked before
publication, and transitive public headers are sealed for relocated consumption.
Native field aliases also work in constructors, and dual direct/namespace imports
emit one record definition and its ownership helpers.

Complete local suites pass 3,240 C and 1,097 Rust checks without failures/skips.
All 19 artifact, 39 import, 14 runtime-contract and 22 CI-helper tests pass.
Record controls cover 36 target/mode/source-or-relocated-prebuilt cases plus four
dual-import controls. Actual canonical C SDK TextFile transport covers 36
target/mode/source-or-prebuilt cases, including public fields, native methods,
strings/arrays surviving record cleanup, and exactly-once explicit/automatic file
closure. Backing SDK sources are removed for prebuilt controls. All 76 controls
and the direct callback-credit/tag/error/output-preservation client pass strict
address/undefined/leak checks. Formatting passes; original programs, output
oracles and snapshots are unchanged.

This uses a declaration adapter retaining the original SDK record fields and
the original C implementation. Independent compilation of the complete unchanged
SDK facade and Sindarin method bodies remains required. RS/GO/SN record providers,
interfaces, value/packed/generic records, dependency lifecycle graphs, reference
rebinding and final corpus/platform acceptance remain required. This feature
requires its own complete hosted CI and does not complete the goal.

Evidence: [canonical record validation](rust-parity-evidence/canonical-record-artifact-validation.json).

## Rust body source bundles: hosted acceptance, 2026-10-11

Corrected integrated main `c9513e59` passes all 34 jobs in
[complete unified CI 38097359995](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38097359995).
The retained summary verifies 26 groups, 292 gate executions and one compiler per
Linux/macOS/Windows platform. This accepts source bundles and RS body live-input
adapters, including the CI checkout/report/SDK/toolchain preservation correction.
It supersedes the pending acceptance and feature freeze statements below.
The full SDK, record/interface, lifecycle graph and corpus requirements remain.

Evidence: [source-bundle acceptance](rust-parity-evidence/rust-body-source-bundle-ci-green.json).

## Source-bundle CI fixture isolation: correction, 2026-10-11

Source-bundle revision `7bf58e1b` failed the Linux, macOS and Windows core jobs in
[CI 38096321120](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38096321120).
All 18 artifact tests passed, but the new direct source-bundle invocation ran
from the compiler checkout with package synchronization enabled. Synchronization
removed `.sn/ci-reports` and the pinned SDK integration checkout as orphaned
packages, so the gate runner could not write its report or continue SDK checks.
On Windows it also removed cached LLVM-MinGW files under `.sn/toolchains`,
timed out during cleanup and caused subsequent missing-library/header failures.
This also explains the missing local SDK checkout recorded below.

The test now runs from its temporary package and uses `--no-install` for every
direct source-bundle invocation. Report, SDK and toolchain sentinels verify
preservation.
Removing that flag in a temporary fixture reproduces the missing report; the
corrected focused test, all 18 artifact, 37 import and 22 CI-helper tests, and
24 canonical SDK TextFile checks pass. Compiler
and package synchronization behavior are unchanged. Feature publication remains
frozen until complete corrected integrated CI passes on all three platforms;
the source-bundle feature has no hosted acceptance yet.

## Rust body source bundles and live inputs: local validation, 2026-10-11

RS implementation libraries now emit complete source bundles, compile canonical
array support and original native C sources with C, and index those objects into
Rust archives. Generated helper names include package/build identity, preserving
external backing symbols and literal bytes. Package-local C forwarding supports
header-only native callbacks. Borrowed input guards preserve actual C headers and
release newly owned local storage; comparisons avoid extra C copy hooks and
string stores retain the C cleanup contract.

Complete local suites pass 3,240 C and 1,097 Rust checks with no failures/skips.
All 18 artifact, 37 import, 14 runtime-contract and 22 CI-helper tests pass. RS
body array controls cover 36 C/Rust target/mode/source-or-prebuilt combinations;
those controls and the native-owner/copy-hook/error/shutdown client pass strict
address/undefined/leak checks. Two independently built RS bodies link together
with distinct C helper symbols and C source-backed initializers. Bundle emission
requires no toolchain execution or main. The pinned SDK integration clone was
restored at the unchanged CI revision after the first run found it absent; the
complete import suite then passed. Original sources/oracles/snapshots are unchanged.

Reference-qualified rebinding, managed record/interface/callback contracts,
dependency lifecycle graphs, SDK artifact migration and complete corpus/platform
acceptance remain required. This batch requires separate complete hosted CI.

Evidence: [Rust body bundles](rust-parity-evidence/rust-body-source-bundle-validation.json).

## Mutable adapters: hosted acceptance, 2026-10-11

Integrated main `63dc5137` passes all 34 jobs in
[complete unified CI 38093383215](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38093383215).
The retained summary verifies 26 groups, 292 gate executions and one compiler per
Linux/macOS/Windows platform. This accepts C body and C/Rust/Go native live-input
adapters. Subsequent RS body emission and full SDK/corpus requirements need
separate acceptance.

Evidence: [mutable adapter acceptance](rust-parity-evidence/mutable-string-array-adapter-ci-green.json).

## Live mutable string-array adapters: local validation, 2026-10-10

Native package ABI 1.5 now connects generated C Sindarin body exports and ordinary
C/Rust/Go native functions to the caller's actual C string-array header. Inputs
preserve in-place mutation, aliases, growth and live callback visibility, with
call credits protecting adopted storage when a callback consumes the caller's
credit. Rust/Go backing functions receive opaque C headers and retain their
original implementations/toolchains. Owned array results keep the existing
managed-value wire format and independent element lifetimes.

Complete local suites pass 3,240 C and 1,097 Rust checks without failures/skips.
All 16 artifact, 35 import, 14 runtime-contract and 22 CI-helper tests pass.
C body exact-output controls cover 36 source/prebuilt target/mode combinations;
the same 36 controls and the C callback-credit/error/shutdown client pass strict
address/undefined/leak checks. Eight C/Rust caller controls consume generated
Rust/Go exports from source and relocated prebuilt archives with backing sources
removed; Go forces GC under `cgocheck2`. Original sources, oracles and snapshots
are unchanged. This feature requires its own complete hosted acceptance.

Rust Sindarin body input emission, reference-qualified rebinding, record/interface
and callback declarations, dependency lifecycle graphs and full C-backed SDK
migration remain required. C-unsafe borrowed parameter rebind and nil-length
controls receive no parity credit; this batch does not complete the full goal.

Evidence: [mutable adapters](rust-parity-evidence/mutable-string-array-adapter-validation.json).

## Bitwise and native string-array views: hosted acceptance, 2026-10-10

Both integrated main revisions pass all 34 unified CI jobs across
Linux/macOS/Windows: bitwise `0b7ef90d` in
[CI 38090421189](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38090421189)
and native-view `0b61548b` in
[CI 38091261038](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38091261038).
Each retained summary verifies 26 groups, 292 gate executions and one compiler per
platform. Their earlier local/pending sections below are superseded by this
acceptance; subsequent mutable adapters require separate hosted validation.

Evidence: [bitwise acceptance](rust-parity-evidence/bitwise-compound-ci-green.json),
[native-view acceptance](rust-parity-evidence/native-string-array-view-ci-green.json).

## Canonical native string-array views: local validation, 2026-10-10

ABI 1.5 transports borrowed or owned views of the actual C string-array header,
preserving allocation, element hooks, aliases and live callback mutation. Views
remain distinct from generic managed-value arrays. Retaining a borrowed view does
not extend its native owner's lifetime; the owner must outlive every view credit.
Owned copies use canonical C hooks. A valid empty native header also exposed a
zero-byte/null-source `memcpy` failure under UBSan; canonical zero-element copies
now skip that call.

Ten staged C/Rust/Go and Rust-consuming-Go ABI clients and four strict C sanitizer
clients pass, including 64 nested callbacks through relocated storage and cleanup
after callbacks consume a caller credit. All 15 artifact, 32 import and 22 CI
helper checks pass. Complete local suites pass 3,240 C and 1,097 Rust checks
with no failures/skips, and package lifecycle clients pass. Original source
programs, output oracles and Rust snapshots are unchanged. The ABI 1.4
prerequisite has complete hosted acceptance. Generated mutable body adapters,
Rust parameter alias/rebind semantics and wider SDK/package contracts remain work;
these runtime views do not complete those adapters.

Evidence: [native string-array views](rust-parity-evidence/native-string-array-view-validation.json).

## Managed-array mutation: hosted acceptance, 2026-10-10

Main `e8d56990` passes all 34 jobs in
[complete unified CI 38089634563](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38089634563).
The retained summary verifies 26 groups and 292 gate executions across one compiler
per Linux/macOS/Windows platform. This accepts ABI 1.4 managed-array mutation on
integrated main. Mutable body-input adapters and full package/SDK acceptance
remain required; subsequent bitwise and native-view batches require separate CI.

Evidence: [array mutation acceptance](rust-parity-evidence/array-mutation-ci-green.json).

## Bitwise and shift compound comparisons: local validation, 2026-10-10

Rust now preserves raw C operator precedence for integral bitwise and shift
compound assignments used as comparison operands. Bitwise operations combine the
old value with the comparison boolean and return the stored value's truthiness;
shifts compare the promoted operation and store the boolean. Checked strict
comparisons keep their helper boundary, and calling lambda value parameters keep
their isolated updates. Captured and reference scalar bitwise operations are
supported, including promoted byte shifts before narrowing to storage.

Complete local suites pass 3,240 C and 1,097 Rust checks with no failures/skips.
The numeric gate passes 207 differential checks and 54 independent exact-output
oracles across all nine optimization/arithmetic combinations. Sixteen C/Rust
address/undefined/leak controls and all 22 CI helpers pass. The strict native
inventory is updated to 393 and its discovery guard passes. Existing source
programs, output oracles and Rust snapshots are unchanged. Undefined shift and
arithmetic cases receive no parity credit. This batch is independent of pending
ABI 1.4; its shared integral comparison prerequisite has hosted acceptance.
Complete hosted acceptance for this batch, original-corpus independent-oracle
and platform acceptance, and package/SDK work remain required.

Evidence: [bitwise comparisons](rust-parity-evidence/bitwise-compound-comparison-validation.json).

## Managed-array mutation: local validation, 2026-10-10

ABI 1.4 adds insertion, ownership-transferring take/pop, removal, clear and
reverse to the canonical C managed-array transport. Array handles and retained
aliases preserve identity. Removal and clear publish changed slots before
reentrant destruction; an operation credit protects the array when cleanup
consumes its external credit, relocates slots and performs nested mutations.
Older version layouts, masks and failed-output preservation remain unchanged.

All eight staged C/Rust/Go and Rust-consuming-Go ABI clients pass. Three C clients
pass strict address/undefined/leak checks. Complete local suites pass 3,240 C and
1,095 Rust checks, with no failures/skips; all 15 artifact, 32 import and 22 CI
helper checks and package lifecycle clients pass. Original programs, runtime
oracles and existing Rust snapshots are unchanged. Corrected integrated main is
accepted; ABI main `e8d56990` has complete
[hosted acceptance in CI 38089634563](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38089634563).
Mutable body-input adapters, live callback visibility, Rust
default-array alias/rebind semantics and wider SDK/package contracts remain work.

Evidence: [array mutation validation](rust-parity-evidence/array-mutation-validation.json).

## Original corpus: complete Linux all-mode discovery, 2026-10-10

Compiler `4a88f178` compiles all 1,365 original programs through both targets in
all nine O0/O1/O2 and default/checked/unchecked combinations: 12,285 unique
mode cases and 24,570 successful target compilations. All source hashes are
enforced, including temporary staging of the exact historical closure-array
source. The compiler binary remained unchanged throughout the complete scan.

There are 12,263 raw C/Rust matches outside the specifically adjudicated
C-undefined cases: 18 unsynchronized concurrent mutations and four unchecked
division-by-zero cases. Those undefined cases receive no parity credit. Raw
matching alone does not prove every independent original oracle or freedom from
all other C undefined behavior. Original-oracle review and macOS/Windows corpus
acceptance remain required; this is a complete discovery inventory, not final
corpus acceptance. The full compressed report is retained with its sealed hashes.

Evidence: [all-mode discovery](rust-parity-evidence/original-corpus-all-mode-discovery.json).

## Inventory corrections: hosted acceptance, 2026-10-10

Main `951d44f2` passes all 34 jobs in
[complete unified CI 38086642421](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38086642421).
The retained summary verifies 26 groups and 292 gate executions using one compiler
per Linux/macOS/Windows platform. This accepts the integrated integer compound
comparison repair, 391-fixture count and Windows inventory guard correction,
superseding their earlier failed/pending runs. The CI feature freeze is lifted;
the larger completion goal has resumed, and the subsequent ABI mutation work
requires separate hosted acceptance.

Evidence: [corrected inventory main](rust-parity-evidence/inventory-corrected-main-ci-green.json).

## Inventory guard Windows correction: local validation, 2026-10-10

Correction main `2f1153bd` failed its Windows build in
[CI 38084602628](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38084602628).
The new negative inventory helper matched only forward-slash glob patterns, so
Windows did not inject its simulated extra fixture. The resulting failed build
never published the compiler bundle; downstream missing-artifact errors have the
same root. The real native inventory remains 391 and its strict guard is intact.

The helper now normalizes injected patterns and exercises both incoming path
separators and Windows-form returned fixture paths. The complete 22 helper tests
pass locally. The focused correction `951d44f2` is published on main; complete hosted
acceptance is pending [CI 38086642421](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38086642421). Feature publication remains
frozen until complete corrected integrated main CI is green.

Evidence: [separator correction](rust-parity-evidence/inventory-separator-correction.json).

## Native fixture count correction: local validation, 2026-10-10

Integral comparison main `4a88f178` failed the strict native-extra inventory gate
on Linux/macOS/Windows in
[CI 38083908519](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38083908519).
The gate still required 389 fixtures after two new controls expanded the catalog
to 391. The actual 391 native behavioral tests passed on each platform.

The required count is now 391; the exact-cardinality guard remains enforced.
Two early CI helper checks use the runner's real catalog, raw fixture manifests
and filtered gates to verify every strict fixture count and to reject an added
fixture before execution when its metadata is stale. The final regression
reproduces the published 389-versus-391 failure. All 22 CI helpers and the real
391-case strict native suite pass locally with no failures/skips. This correction
changes inventory metadata and its guard only. Unpublished ABI mutation work is
excluded. Feature pushes remain frozen until complete corrected integrated main
CI is green. Correction `2f1153bd` is published; its hosted run exposed the
test-only Windows issue described above.

Evidence: [inventory correction](rust-parity-evidence/native-inventory-count-correction.json).

## Corrected integrated main: hosted acceptance, 2026-10-10

Main `25c69e5b` passes all 34 jobs in
[complete unified CI 38082597871](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38082597871).
The retained summary verifies 26 groups and 292 gate executions with one compiler
per Linux/macOS/Windows platform. This accepts the integrated mixed floating
compound arithmetic/comparisons, ABI 1.3 managed-array replacement and Windows
fixture-path correction, superseding their earlier pending and failed runs.
The feature publication freeze is lifted. Whole-corpus, mutable body adapters,
independent SDK migration and wider language/package acceptance remain required.

Evidence: [corrected main acceptance](rust-parity-evidence/corrected-integrated-main-ci-green.json).

## Integral compound comparisons: local validation, 2026-10-10

Rust now preserves the raw C comparison/store behaviour for arithmetic compound
assignments with integral operands. C integer promotions apply before comparison;
unsigned arithmetic wraps at the promoted width, and the comparison boolean is
stored in the declared target. Checked strict comparisons retain their helper
argument boundary. Lambda value parameters with calling right-hand sides retain
their isolated update. Controls cover arithmetic operators, byte promotion,
mixed signed/unsigned widths, floating comparisons, fields/indexes, reference and
native-resource aliases, captures, synchronized variables and helper collisions.

Complete local suites pass 3,240 C and 1,095 Rust checks with no failures/skips.
The expanded numeric gate passes 189 C/Rust pairs and 36 independent exact-output
oracles across O0/O1/O2 and default/checked/unchecked modes. Twenty CI helpers and
eight C/Rust native address/undefined/leak sanitizer controls pass. Existing Rust
snapshots and tracked original programs/oracles are unchanged; all 1,155 original
output oracles match the tag. The known historical closure-array source deviation
predates this batch and is unchanged; the earlier original-source staging remains
required for that program's audit.

Implementation `4a88f178` is published on main. Complete hosted acceptance is
pending [run 38083908519](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38083908519).
Bitwise compound contexts require different C precedence lowering and remain
work. C-undefined arithmetic is not credited, and complete original-corpus,
package/SDK and platform acceptance remain required.

Evidence: [integral comparisons](rust-parity-evidence/integral-compound-comparison-validation.json).

## Windows comparison report correction: local validation, 2026-10-10

Arithmetic implementation `aac3cae8` failed Windows numeric CI in
[run 38081042679](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38081042679).
All 171 actual differential cases passed. The independent checker filtered on
forward-slash fixture IDs, whereas Windows reports use backslashes, so it skipped
both comparison fixtures and incorrectly reported incomplete mode coverage.
Integrated `ee2a0ffd` failed
[run 38081916057](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38081916057)
with the same checker blob and the same Windows path-selection failure.

The checker now normalizes only fixture identifiers. Source hashes, successful
compile/exit requirements, exact raw output and complete unique mode/optimization
coverage remain enforced. Replaying the actual retained Windows artifact passes
all 18 independent comparison oracles with exact CRLF transport; the retained
Linux report passes its 18 independent oracles. The complete 20-test CI helper
suite includes focused separator, transport, missing/duplicate case, source hash,
compile failure and stderr controls. Compiler, runtime and language fixtures are
unchanged by this correction. Feature publication is frozen until complete
Linux/macOS/Windows CI on corrected integrated main is green. Correction
`25c69e5b` is published; complete hosted acceptance is pending
[run 38082597871](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38082597871).

Evidence: [Windows path correction](rust-parity-evidence/windows-compound-path-correction.json).

## Managed-array replacement: local validation, 2026-10-10

The canonical C runtime implements ABI 1.3 bulk managed-array slot replacement.
It preserves the destination handle and its retained aliases, acquires incoming
element credits before publication, and releases detached old slots afterward.
Nil source clears to a non-nil empty destination; wrong-kind errors preserve
contents and self-assignment is a no-op. An operation credit protects destination
lifetime while destructors inspect, replace and grow it or release its external
credit. Contained resources retain their identity; independently retained elements
survive clearing and array teardown. Queries for older ABIs retain their exact
layouts, capability masks and failure-output behaviour.

C/Rust/Go and Rust-consuming-Go clients pass byte, alias, resource/element lifetime
and error controls. Two C clients pass strict address/undefined/leak checks,
including nested replacement/growth and cleanup that consumes the caller's
external credit. Complete local suites pass 3,240 C and 1,093 Rust checks; all 15
artifact, 32 import and 15 CI-helper tests and package-lifecycle clients pass.
Original corpus sources and behavioural oracles are unchanged. Existing native
package ABI 1.0/1.1 and package lifecycle ABI 1.2 paths remain verified.

Implementation `ee2a0ffd` is published on main with hosted acceptance pending
[run 38081916057](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38081916057).
This is the runtime prerequisite for mutable body-input transport. The existing native borrowed-array contract remains
read-only. Mutable Sindarin body adapters, live callback visibility, Rust
parameter alias partitions/rebind semantics, broader public type contracts,
independent SDK artifacts and whole corpus/platform acceptance remain required.

Evidence: [array replacement validation](rust-parity-evidence/array-replacement-validation.json).

## Mixed floating compound arithmetic and comparisons: local validation, 2026-10-10

Rust accepts C-supported mixed float/integer compound operations, computes at the
actual C operand width and converts once at storage. By-value/reference parameters,
fields, indexes, captures, synchronized storage and globals retain their semantics.
Three original rejection sources have hash-checked positive promotion records;
their source bytes and old diagnostic oracles remain preserved.

A raw C comparison around a scalar floating compound expression stores its
boolean result in the target. Rust now preserves that ordering rather than first
rounding/truncating and storing the arithmetic result. Checked strict-comparison
helpers keep their argument boundary. Lambda by-value parameters with a calling
RHS keep C's isolated statement-expression behaviour. Comparison controls observe
stored values through reference-record and native C resource aliases, call counts,
cleanup and helper-name collisions. Eighteen independent fixed-output cases cover
all O0/O1/O2 and default/checked/unchecked identities, including default O2's
unchecked selection. The original false/zero reproducer passes nine further
fixed-output C/Rust controls.

Complete local suites pass 3,240 C and 1,093 Rust checks without failures/skips.
All 171 arithmetic pairs, 18 independent comparison oracles, native C/Rust
address/undefined/float-cast sanitizer controls, eight artifact/coverage helper
tests and formatting pass. The platform workflow runs the complete 15-test helper suite.
Existing generated Rust snapshots, original sources and behavioural oracles are
unchanged. Implementation `aac3cae8` is published on main; complete hosted acceptance
is pending [run 38081042679](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38081042679).
Wider integral/bitwise compound-expression contexts, C-undefined conversions and
whole original-corpus/platform/package/SDK acceptance remain required work.

Evidence: [mixed arithmetic validation](rust-parity-evidence/mixed-floating-compound-validation.json)
and [comparison repair](rust-parity-evidence/compound-comparison-validation.json).

## Owned body array exports: hosted acceptance, 2026-10-10

Main `9af4f467` passes all 34 jobs in complete unified Linux/macOS/Windows CI.
The retained coverage summary verifies 26 groups and 292 gate executions with one
recorded compiler per platform. This supersedes the earlier pending array entry.

[Accepted array-export run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38078893550).
Evidence: [hosted acceptance](rust-parity-evidence/owned-body-arrays-main-ci-green.json).

## Owned body array exports: local validation, 2026-10-10

Independent C/RS Sindarin libraries now export owned `str[]` results through the
existing ABI 1.1 managed-value transport. Generated body facades preserve nil
versus empty arrays/elements, arbitrary string bytes and consumer mutation.
Rust package bodies use the nullable array representation consistently even when
no nil literal occurs in their source. Package building round-trips compiler
model/C-source byte payloads instead of requiring UTF-8 language string data.

C/RS providers pass direct and status protocols. Sanitized C clients retain
individual elements, release their array, shut down a global-backed library twice,
and verify element bytes remain valid. Closed calls preserve caller outputs.
C/Rust applications consume source-built and relocated prebuilt libraries after
body sources and producer caches are removed. Managed array inputs, wider
record/interface exports, modular Rust sidecars and complete SDK migration remain
required work; this is one step toward the full package architecture.

Local full suites pass 3,240 C and 1,090 Rust checks. All 32 import tests, 15 artifact
tests and 15 CI helpers pass. Strict address/undefined/leak checks include both
stateless and global-backed C/RS array exports. Original sources and behavioural
oracles remain unchanged. Hosted acceptance of the array batch is pending publication.

Evidence: [owned body arrays](rust-parity-evidence/sindarin-array-body-validation.json).

## Package lifecycle: hosted acceptance, 2026-10-10

Main `583a863a` passes complete unified Linux/macOS/Windows CI, accepting ABI 1.2
package controls, C/Rust/Go lifecycle clients and ordinary C/RS library global
initialization/cleanup. This supersedes the earlier pending entry below.

[Accepted lifecycle run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38056825974).
Evidence: [hosted acceptance](rust-parity-evidence/package-lifecycle-main-ci-green.json).

## Shared package lifecycle and implementation globals: local validation, 2026-10-10

ABI 1.2 adds canonical C package controls and owned call credits. Initialization
runs once outside the lock; concurrent callers wait, same-thread initialization
can reenter, and failures remain sticky. Shutdown blocks new calls, drains active
calls and runs cleanup once. Active same-thread shutdown returns BUSY rather than
deadlocking; closed calls preserve outputs. Retain/release and call credits keep
controls alive until the last operation ends. C/Rust/Go clients and strict
address/undefined/leak sanitizers pass concurrency/reentry/failure/alias checks.
ABI 1.0/1.1 layouts, query masks and behaviours remain verified independently.

Independent C/RS libraries now initialize ordinary globals before export calls.
Static/default storage is seeded before deferred expressions, preserving C's
later-literal visibility; deferred initializers retain source order. Rust global
cells hold an optional value so shutdown can drop owned state without leaking
static LazyLock payloads. Initializer panic rolls back initialized Rust slots.
Cleanup follows reverse declaration order, with automatic process cleanup and
explicit initialize/shutdown exports recorded in artifact metadata. Returned owned
strings remain valid after shutdown, later status calls preserve outputs, and
relocated prebuilt global libraries work without implementation source/caches.

Package-global thread/handle captures, modular RS native sidecars, managed body
exports and full SDK artifact migration remain unfinished. Unsupported lifecycle
cases are diagnosed and receive no completion credit.

Local full suites pass 3,240 C and 1,090 Rust checks, 31 imports, 14 artifact tests
and 15 CI helpers. Earlier ABI clients, C/Rust/Go lifecycle clients, runtime
sanitizers and C/RS library cleanup sanitizers pass. The unified catalog keeps
26 groups and now covers 292 platform gate executions. Hosted acceptance of this
integrated lifecycle batch is pending publication.

Evidence: [package lifecycle validation](rust-parity-evidence/package-lifecycle-validation.json).

## Function libraries: hosted acceptance, 2026-10-10

Main `89c23570` passes complete unified Linux/macOS/Windows CI, accepting the
independent stateless body libraries, typed exports and nullable ownership repairs.
This supersedes the earlier pending entry below; full goal acceptance remains open.

[Accepted function-library run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38053669489).
Evidence: [hosted acceptance](rust-parity-evidence/function-library-main-ci-green.json).

## Independent Sindarin function libraries: local validation, 2026-10-10

`language: SN` build units compile original implementation bodies for the package's
C/RS runtime; omitted runtime inherits the application's target. `--build-package`
builds reusable archives with generated typed export facades, selected runtime,
implementation export contracts and dependency hashes. The public native API is
checked against actual implementation signatures before emission. C symbols are
package-scoped and Rust crate identities stay distinct. Applications consume the
compiled package through shared C-ABI adapters, including fixed C libraries from
Rust and fixed RS libraries from C. No competing application entry is emitted.

Tests cover direct/status wire protocols, imported private helpers, nullable and
empty strings, independent result lifetimes, inheritance and relocated prebuilt
reuse after all implementation sources/caches are removed. Standalone C clients
consume C/RS body exports, with strict address/undefined/leak sanitizer checks.
GO implementation bodies, mismatched signatures, application main functions and
implementation globals fail clearly. Full package initialization/shutdown,
managed array/record body exports, generic public contracts, modular RS native
sidecars and unchanged SDK facade migration remain required work.

This exposed absolute import normalization dropping the leading separator;
package entry compilation now preserves absolute/UNC roots. C borrowed-string
returns now use existing nil-aware duplication. The native regression also found
an owned temporary leaking during nil comparison; equality/inequality now acquire
one temporary owner and release it after the comparison. Nil/empty distinctions,
caller isolation and once-only effects pass nine optimization/arithmetic profiles
and debug sanitizers. All original sources and execution/code-generation oracles
are unchanged. The native inventory rises to 386.

Complete local suites pass 3,240 C and 1,090 Rust checks. All 30 import tests,
13 artifact tests and 15 CI helper tests pass. Focused status/export and sanitizer
checks pass. Exact-revision hosted acceptance is pending publication; this does
not complete the full package/SDK architecture or Rust parity goal.

Evidence: [Sindarin body library validation](rust-parity-evidence/sindarin-library-validation.json).

## Prebuilt consumption: hosted acceptance, 2026-10-10

Main `94e8a31f` passes complete Linux/macOS/Windows unified CI, accepting the sealed
prebuilt loader, ABI runtime guards, C provider hygiene and package body-boundary
diagnostics. This supersedes that batch's earlier pending acceptance notes below.

[Accepted prebuilt run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38050952649).
Evidence: [hosted acceptance](rust-parity-evidence/prebuilt-main-ci-green.json).

## Verified prebuilt native consumption: local validation, 2026-10-10

Manifests can select a sealed `native.assembly` descriptor. C/Rust applications
consume relocated C/Rust/Go archives after original backing sources/modules and
producer build caches are removed. Go/archive/symbol tools and a Rust backing
compiler are unnecessary for C consumers. Public Sindarin declaration bytes,
binding/ownership/type contracts, platform/architecture/pointer width,
build/export/initialization inventories and archive hashes are verified before
linking. Incompatibility fails without source rebuilds or fallback implementations.

Consumers select their current canonical C runtime rather than producer paths.
Adapters query its ABI version/capabilities and scalar/pointer widths before
foreign calls. ABI 1.0 transport remains supported. Runtime rejection tests prove
unsupported versions/widths fail before the backing function runs. Multiple Go
units inside one prebuilt aggregate archive work under the existing graph model;
combining prebuilt Go runtimes with another Go package requires an aggregate
source rebuild and is explicitly diagnosed. Complete source-independent multi-Go
composition and independent Sindarin bodies/SDK artifacts remain required work.

The corruption matrix rejects descriptor/schema, package identity, ABI/platform,
public declarations, provider types, ownership, exports, initialization, archive
hash/path and malformed descriptor errors. A C provider hygiene repair removes
backing-name macros before rendering adapter locals and calls the explicit private
symbol: an ordinary backing function named `value` no longer shadows its call
with the generated local `value` variable. Original backing sources are preserved.

The runtime boundary also rejects foreign-runtime package method bodies and
globals that previously escaped the free-function body check. They require real
independent package compilation rather than silent application-target emission.
Matching C package/application callers retain their tested source path.

Local suites pass 3,240 C and 1,089 Rust checks. All 28 import tests, 12 artifact
tests, 15 CI helper tests and five strict provider/runtime-guard sanitizer tests
pass. Existing source/oracle inventories remain intact. This prebuilt scalar/string
path is independent of the pending borrowed-array batch and uses the already
accepted transport/provider foundation. Hosted acceptance is pending publication.

Evidence: [prebuilt consumption validation](rust-parity-evidence/prebuilt-native-consumption-validation.json).

## Scalar conversion prerequisites: hosted acceptance, 2026-10-10

Character-conversion main `f805570a`
([run 38046184242](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38046184242)),
finite-double main `5d295716`
([run 38047206995](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38047206995)),
and signed-postfix main `7c3b132f`
([run 38048129178](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38048129178))
pass complete Linux/macOS/Windows unified CI. These runs supersede the earlier
pending descriptions for those increments. The borrowed-array batch is published
on main `04f04549`, with exact-revision
[run 38048978802](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38048978802)
still pending. Full goal acceptance remains unfinished.

Evidence: [accepted scalar increments](rust-parity-evidence/scalar-increments-main-ci-green.json).

The borrowed-array main `04f04549` has subsequently passed complete unified
Linux/macOS/Windows CI
([run 38048978802](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38048978802)).
This supersedes the pending note above and that increment's earlier local entry.
Evidence: [borrowed-array hosted acceptance](rust-parity-evidence/borrowed-string-array-main-ci-green.json).

## Borrowed string-array package inputs: local validation, 2026-10-10

ABI 1.1 native declarations now accept read-only borrowed `str[]` inputs through
generated consumer/provider adapters in C, Rust and Go. Nil/empty arrays and
elements, non-UTF-8 bytes through the first NUL, repeated-argument aliases and
distinct empty array identities are preserved. C temporary views install string
copy/release hooks, so backing `sn_array_copy` returns an independent owned value.
All acquired transport credits/views are released after successful calls and
after invalid input or backing status failures. Status output pointers remain
unchanged on failure. ABI 1.0 and non-borrowed array input contracts are diagnosed.

The integration regression found C string-array literals calling `strdup(NULL)`.
Literal element copying now uses the existing nil-aware `sn_strdup` helper.
The focused C/Rust regression observes nil versus empty elements, independent
copy ownership and mutation/cleanup under native debug sanitizers. Two generated-C
snapshots record the helper change; existing Sindarin programs and execution
oracles remain unchanged.

Complete local suites pass 3,240 C and 1,089 Rust checks. All 24 native import
tests, 11 native artifact tests and 15 CI helper tests pass. Strict provider
address/undefined/leak sanitizer clients cover aliases, raw byte data, element
lifetimes after input release, wrong array/element kinds, invalid booleans, null
outputs, backing errors and preserved failure outputs. The native inventory is
385. Broader array/record/interface/callback contracts, independent Sindarin
package bodies, complete SDK artifact imports and full parity acceptance remain
required. Publication is held while the two feature CI slots are occupied;
exact-revision hosted acceptance is pending.

Evidence: [borrowed string-array validation](rust-parity-evidence/borrowed-string-array-validation.json).

## String-array result adapters: hosted acceptance, 2026-10-10

Main `e5763d3b` passes complete Linux/macOS/Windows unified CI, including the
pinned canonical SDK managed TextFile/readLines implementation and strict
array-provider lifetime gates. This supersedes that batch's earlier pending
publication/acceptance notes below.

[Accepted string-array run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38045852403).
Evidence: [hosted acceptance](rust-parity-evidence/string-array-main-ci-green.json).

## Signed value-parameter postfix operations: local validation, 2026-10-10

The parameter-mutation preparation pass now marks `int`, `long` and `int32`
default-qualified parameters for the already implemented postfix lowering.
Previously the signed compound-assignment marker existed, but `++` and `--`
were rejected before reaching it. Named functions, instance/static methods,
old/new values, caller isolation and safe signed limits pass nine C/Rust mode
comparisons and four native/debug profiles. The existing shadow-initializer
guard, arithmetic lowering and C runtime remain unchanged.

Both original signed increment/decrement rejection fixtures and diagnostics are
byte-identical; promotion records require eighteen successful C/Rust profile
comparisons. The native inventory rises to 384. Complete local suites pass 3,240
C and 1,088 Rust checks and all 15 CI helper checks pass. Twenty-four additional
O2 boundary executions agree between C/Rust with checked/unchecked CLI flags;
the evidence records these observations separately and grants no parity credit
to undefined C operations or an assumed overflow-checking contract.

This batch is locally validated and unpublished while feature CI slots are
occupied. Full original-corpus mode/platform acceptance, wider parameter and
record mutation, package artifacts, SDK adapters and shared ABI work remain
required. Evidence: [signed parameter postfix validation](rust-parity-evidence/signed-parameter-postfix-validation.json).

## Finite double integer conversions: local validation, 2026-10-10

Rust accepts `double.toInt()` and `double.toLong()` for the existing C-defined
finite range, truncating toward zero and evaluating the receiver once. Controls
include fractions on both sides of zero, negative zero, exact large integers,
the largest binary64 integer below the positive signed limit and the inclusive
negative limit. An independent C cast agrees in nine optimization/arithmetic
profiles and four native/debug profiles. Three C profiles also pass undefined
behaviour and float-cast-overflow sanitizers.

NaN, infinities and out-of-range casts are recorded separately as C-undefined
controls; their runtime outputs receive no parity credit. The historical
double-to-integer rejected source and diagnostic remain unchanged, with a sealed
promotion record proving C/Rust execution in nine profiles. All 1,087 current
Rust checks and 15 CI helper checks pass. The preceding character batch retains
the complete passing 3,240-check C baseline. No C runtime behaviour is changed.

Publication is held while the independent string-array main `e5763d3b`
([run 38045852403](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38045852403))
and character main `f805570a`
([run 38046184242](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/38046184242))
occupy both pending CI slots. Exact-revision hosted acceptance of this batch is
pending. Full package/SDK/ABI and original-corpus platform/mode acceptance remain
required.

Evidence: [double conversion validation](rust-parity-evidence/double-conversion-validation.json).

## Historical rejection inventory: refreshed characterization, 2026-10-10

The 118 current rgen rejection fixtures were recompiled through C at `-O0 -g`
and executed under the normal debug sanitizers, then checked against current
Rust source emission. Sixty run successfully in C while Rust still rejects them;
39 fail C compilation, 15 fail C runtime/sanitizers, and four are accepted by
both at this local checkpoint. This inventory is preparation for semantic
review, not evidence of parity or of universally defined C behaviour. C failure
and undefined cases remain separate from Rust implementation gaps. The review
retains every original program and diagnostic oracle.

Evidence: [refreshed rejection inventory](rust-parity-evidence/negative-admission-review-20261010.json).

## Integer and byte character conversions: local validation, 2026-10-10

Rust accepts the C-supported `int.toChar()` and `byte.toChar()` methods, retaining
the low byte and evaluating the receiver once. Existing character widening and
string helpers preserve the host C signedness, NUL and high-byte semantics.
An independent C reference checks all 256 byte values, negative/wrapping inputs,
both signed integer limits and effectful receivers. Nine C/Rust optimization and
arithmetic comparisons and four native profiles, including debug sanitizers,
pass without changing the C runtime or existing language source.

The original rejected `primitive_conversion_int_to_char` program and its
diagnostic oracle remain byte-identical. Its promotion record now requires
successful C/Rust execution in nine profiles rather than treating rejection as
parity. The native inventory increases to 382; the complete local suites pass
3,240 C and 1,086 Rust checks with zero failures/skips, and all 15 CI helper tests
pass. Double-to-integer conversions and the remaining package/SDK/shared-ABI and
full-corpus acceptance requirements remain open. This batch is independent of
the pending string-array implementation; exact-revision hosted acceptance is
pending publication.

Evidence: [character conversion validation](rust-parity-evidence/character-conversion-validation.json).

## Generated owned string-array results: local validation, 2026-10-10

ABI 1.1 native declarations returning `str[]` now drive generated C/Rust/Go
provider exports and C/Rust consumer adapters. Providers preserve nil/empty
arrays/elements, copy byte-oriented strings into shared managed values and release
original owned backing storage. Consumers construct owning C-runtime string arrays
with cleanup/copy hooks and release transport credits. ABI 1.0 array contracts and
array inputs retain explicit diagnostics; complete record/method/SDK adapters,
managed element graphs and language copy-hook handling remain required work.

The integration regression exposed a C double free: printing a borrowed string
array element incorrectly applied temporary-string cleanup. ASAN identified both
frees in generated main; the ownership classifier now recognises live array-index
owners. A C/Rust regression prints repeatedly, mutates and destroys the same array,
including debug sanitizers. Native inventory rises to 381 with original fixtures
and oracles retained. Complete SDK artifact imports/full Rust parity remain open.

Evidence: [string-array adapter validation](rust-parity-evidence/string-array-adapter-validation.json).

Resumed validation passes 3,240 C and 1,085 Rust checks, 22 native import tests,
10 native artifact tests, 14 runtime/manifest tests and 15 CI helper tests.
Status array providers preserve outputs on backing errors and reject null output
pointers; successful nil/empty arrays are covered through C/Rust consumers.
C transport failures release partial arrays and backing storage before returning
the error. Strict provider, shared-runtime and SDK address/undefined/leak checks
pass, along with all 24 canonical SDK checks. Original source/oracle inventories
remain intact. Exact-revision hosted acceptance is pending publication.

The initial parallel local C invocation lost one generated executable because
both suite runners clean the same temporary-directory prefix on startup. The
isolated C rerun passes every check; CI already runs the suites in separate jobs.
This diagnosis is retained in the validation evidence.

## Runtime prerequisites: hosted acceptance reconciled, 2026-10-10

Complete Linux/macOS/Windows unified CI is green on managed-array main
`6ea76011` ([run 37997020187](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37997020187)),
typed-resource main `23f52899`
([run 37995031547](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37995031547)),
and unsigned-conversion main `47d06f31`
([run 37993627668](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37993627668)).
These accepted runs supersede the pending publication/acceptance notes below.
The SDK managed TextFile/readLines increment is published at `f7e7727` and the
compiler workflow now pins that exact SDK revision. Full SDK artifact migration,
independent Sindarin package bodies and remaining Rust parity are still open.

## Managed-value runtime arrays: local validation, 2026-10-09

ABI 1.1 negotiates managed-value arrays and typed-resource capabilities while
ABI 1.0 retains its existing query mask/layouts. Arrays own element credits;
get returns an owned element, copy creates independent slots retaining their
values and retain aliases one array. Slot replacement acquires/publishes the new
owner before releasing the old, supporting reentrant cleanup that resizes the array.
POD/managed kind mismatches and bounds failures preserve outputs. Higher-level
language copy hooks, managed graph cycles and complete generated adapters remain
separate goal requirements; this slot-copy operation does not imply deep copying.

C/Rust/Go clients pass lifetime/copy/alias checks. C address/undefined sanitizers
exercise reentrant replacement and original ABI 1.0 clients remain covered. Local
SDK C readLines bridges copy canonical C output into managed strings; C/Rust/Go
clients verify retrieved lines survive both array and file release. All 24 existing
SDK checks and its sanitizer check pass, with original Sindarin/C sources unchanged.

Local suites pass 3,240 C and 1,084 Rust checks, nineteen package imports, nine
artifact tests and fifteen CI helpers. Publication and SDK updates are held until
typed-resource prerequisite CI succeeds. Full record/interface/SDK artifacts and
Rust parity remain unfinished.

Evidence: [managed-value array validation](rust-parity-evidence/managed-value-array-validation.json).

## Typed runtime resources: local validation, 2026-10-09

The C runtime can adopt resources with a copied package/type/ABI identity and
reject incompatible live handles before publishing payload pointers. Identity
views borrow the owner's lifetime; retain preserves aliases and final release
invokes cleanup exactly once. Generic resources and existing public ABI layouts,
status values and version remain unchanged. Typed entry points are prerequisites
for generated managed-resource adapters, not complete record/interface support.

C/Rust/Go runtime clients cover copied identities, mismatches, unchanged failure
outputs, aliases, thread/reentry lifecycle and nil. The SDK TextFile C module has
local typed-handle factory/path/readLine/dispose/isOpen bridges using the existing
C implementation. All 24 SDK legacy/native checks and its sanitizer check pass
locally, including new C/Rust/Go handle clients. SDK publication is held until this
runtime prerequisite passes integrated CI; compiler-generated SDK artifact imports,
full SDK migration and wider managed ABI/parity remain outstanding.

Local suites pass 3,240 C and 1,084 Rust checks. Hosted acceptance is pending.
Evidence: [typed-resource validation](rust-parity-evidence/typed-resource-validation.json).

## Go package graph aggregation: hosted acceptance, 2026-10-09

Main `5cc8fc8b` passes complete Linux/macOS/Windows unified CI, including multi-Go
module/package graphs, original caller paths and strict graph archive sanitizers.
This supersedes earlier pending acceptance below.

[Accepted aggregation batch](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37992420228).

## Unsigned-to-signed primitive conversions: local validation, 2026-10-09

Rust now accepts existing `uint.toInt()` and `uint.toLong()` methods, using the
same 64-bit narrowing/wrapping behaviour as supported C targets. Boundaries cover
zero, representative values, the signed limit, the unsigned high half and maximum,
plus an effectful receiver evaluated once. No Sindarin syntax or C implementation
changes are required.

Two historical rejection fixtures now pass unchanged through C/Rust in nine
profiles. Their original source and rejection oracles remain byte-identical;
explicit parity records seal those files and provide positive output/exit evidence.
The runner executes promoted cases instead of skipping them or claiming matching
rejection as support. Guard tests reject altered originals and C compilation failure.
The closure-body case now returns its original output (`1`). New semantic coverage
raises the native inventory to 380 without removing original fixtures.

Local suites pass 3,240 C and 1,084 Rust checks, promoted/boundary cases under the
ASAN compiler, and fifteen CI helper checks. Aggregation CI remains pending at
publication; this conversion batch is independent and uses the second permitted
slot. Hosted acceptance is pending. Double/character conversions, wider managed
ABI contracts, SDK artifacts and remaining full parity requirements remain open.

Evidence: [unsigned conversion validation](rust-parity-evidence/unsigned-conversion-validation.json).

## Go package graph aggregation: local validation, 2026-10-09

Multiple generated-provider Go libraries now compile through one bridge/runtime
archive consumed by C/Rust Sindarin applications. Normal Go module resolution runs
in generated bridge files, preserving original manifests/source and shared module
instances. Conflicting replacements and duplicate module roots are diagnosed;
legacy handwritten main archives remain valid individually and cannot be silently
combined. A package with several GO build units also builds one independent
archive, referenced once by its unit metadata/link plan.

A shared-counter regression observes one dependency instance from both application
targets. String regressions preserve borrowed input/owned result lifetimes and nil
across multiple libraries. Standalone C clients link multi-unit Go packages through
one archive; original single-Go/C/Rust paths remain covered. Aggregate artifacts
are immutable and carry input/dependency/runtime/toolchain metadata, but do not yet
provide complete source-independent package consumption or transitive cgo cache
reuse. SDK artifacts, wider managed ABI contracts and all remaining Rust parity
requirements remain open. Local suites pass 3,240 C and 1,083 Rust checks,
nineteen normal/ASAN imports, nine artifact tests, fourteen package-contract
tests and thirteen CI helpers. Strict provider/graph address, undefined-behaviour
and leak sanitizer clients pass; the existing Linux provider gate now includes
multi-unit Go archives without changing the 26 groups or 288 executions.
Hosted acceptance of this batch is pending.

Evidence: [Go graph validation](rust-parity-evidence/go-package-graph-validation.json).

## Generated provider exports: hosted acceptance, 2026-10-09

Main `95c780f6` passes complete Linux/macOS/Windows unified CI, including generated
providers, original mixed callers and strict Linux provider lifetime sanitizers.
This supersedes the earlier pending acceptance below.

[Accepted provider batch](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37989867688).

## Generated native provider exports: local validation, 2026-10-09

Optional binding `function` metadata now resolves typed native declarations and
builds generated C-ABI exports for ordinary C, Rust and Go backing functions.
Standalone package builds and application imports share this path. C prototypes
are enforced during source compilation; Rust retains its original crate/module
structure; Go imports the original library and rebases relative module replacements.
Original backing source and manifests are preserved. Supported contracts currently
cover plain scalars, borrowed strings, owned string results and abort/status errors.

C backing function link names and Rust crate identities include package/build
identity. A regression previously called the first C implementation from both
packages; it now preserves each package's output. Duplicate public wire exports
across imported packages are diagnosed. Complete automatic export-name allocation
and wider namespace/graph planning remain outstanding.

String-only views reject ABI buffers before backing code can assume termination.
Nil/empty/non-UTF-8 values, result lifetime, failure outputs, scalar conversions,
void returns and call-frame panic containment have integration regressions. Go's
legacy `panic(nil)` is contained; Rust panic hooks retain existing behaviour.
Independent ABI clients consume all three generated provider languages. These
remain partial native artifacts; complete Sindarin/SDK artifacts, managed
record/interface/array/callback contracts, Go aggregation and full parity remain
required by the goal. Local suites pass 3,240 C and 1,083 Rust checks, fifteen
normal/ASAN import tests, eight artifact tests, fourteen package-contract tests
and thirteen CI helper tests. The shared-runtime clients and strict
provider lifetime sanitizer clients pass. A new Linux sanitizer gate keeps the
26 groups and brings required executions to 288. Hosted acceptance for this
provider batch is pending.

[Provider contract](native-package-imports.md#generated-provider-exports).
Evidence: [provider validation](rust-parity-evidence/native-provider-validation.json).

## Native import platform correction: hosted acceptance, 2026-10-09

Corrected main `8c367892` passes complete Linux/macOS/Windows unified CI,
including all three core gates and validated artifacts. This lifts the feature
publication hold and supersedes the earlier pending/frozen descriptions below.

[Accepted correction](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37986936909).

## Native import platform correction: local validation, 2026-10-09

Main `a84c3346` fails the macOS and Windows core jobs in run `37984504278`.
Feature publication is frozen. Windows uses `PATH_MAX` (260) for the entire C
library argument list; this correction grows that list from actual arguments and
preserves all dependencies. Native toolchain framework/option pairs are converted
to single driver options before separate link pragmas can reinterpret their
arguments as library names. Diagnostic tests use canonical, forward-slash package
paths, matching the compiler on macOS and Windows.

A twenty-package C application fails with the original linker buffer and passes
with the correction. Paired framework options and missing option arguments have
focused regressions; the unchanged mixed-language tests exercise actual macOS
framework linking in hosted CI. Original Sindarin source/oracles are preserved.
Eight native import tests pass locally through normal and ASAN compilers; all
1,616 ASAN unit tests pass. Complete local suites pass 3,240 C and 1,083 Rust
checks; fourteen package-contract and thirteen CI helper tests also pass.
Complete hosted acceptance remains pending. Provider
export generation stays local until corrected integrated CI is green.

Evidence: [native platform correction](rust-parity-evidence/native-import-platform-correction.json).

## Generated native package imports: local validation, 2026-10-09

Resolved native declarations now drive generated C consumer adapters and automatic
original-language archive linking for C/Rust Sindarin applications. Scalar values,
borrowed string inputs, owned string results and explicit status failures cross the
shared C ABI. Native-only C/RS/GO declaration packages can differ from the consumer;
unsupported body/type/ownership contracts retain diagnostics. Ordinary imports and
legacy source directives remain their existing paths. Mixed per-application
@source/native-artifact backing is rejected to prevent silent symbol shadowing.

Six integration tests pass through the normal and ASAN compilers. A real Rust
Sindarin app combines C/Rust/Go native packages, portable request-line code and C
SDK TextFile resources/arrays. SDK calls still use the legacy compatibility path.
Local suites pass 3,240 C checks, 1,083 Rust checks, 1,616 ASAN units, fourteen
package-contract tests and thirteen CI helper tests. A native-import core gate
retains 26 groups and increases required executions to 287. Hosted acceptance is
pending; independent SDK artifact imports, complete package bodies/type descriptors,
provider export shims, graph aggregation and full Rust parity remain required work.

[Contract/limits](native-package-imports.md).
Evidence: [native import validation](rust-parity-evidence/native-import-validation.json).

## SDK oracle correction: hosted acceptance, 2026-10-09

Corrected compiler main `0e463a1d` passes full Linux/macOS/Windows unified CI,
including pinned SDK native clients and the exact Windows text oracle. SDK
correction `52bd4e4` also passes all four SDK jobs. This lifts the publication
freeze and supersedes the earlier pending/frozen descriptions below.

[Accepted compiler correction](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37981073019).

## SDK Windows oracle correction: local validation, 2026-10-09

Compiler run `37978810233` passes the Windows native artifact and SDK native
C/Rust/Go clients, then fails its SDK legacy oracle. Git's Windows checkout
already contains CRLF in the expected file; applying CRT translation again
produces CRCRLF. SDK correction `52bd4e4` converts either LF or CRLF checkout bytes
to Windows CRT bytes exactly once. All other bytes remain significant, and no
original SDK source/expected fixture is rewritten.

Two regressions verify both checkout forms and preservation of non-UTF-8, NUL
and isolated CR bytes. The SDK's oracle job and full Linux/macOS/Windows standard
CI pass. All 24 native/legacy SDK checks pass locally with accepted compiler main.
The compiler workflow now pins the correction. Full native integration acceptance
is pending, and feature publication remains frozen until corrected integrated
compiler main is green. The package adapter implementation remains local/unpublished.

[Accepted SDK correction](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/37980671870).
Evidence: [SDK oracle correction](rust-parity-evidence/sdk-oracle-correction.json).

## Independent TextFile SDK module: published; integration CI pending, 2026-10-09

SDK main `723b3e2` now builds TextFile native backing independently with its
existing C implementation. Its package-owned record/constructor and compatibility
layout checks preserve the reference-count prefix, existing fields and unchanged
Sindarin declarations. Atomic retain/release, explicit/final close and shared-
runtime path transport are available without a Rust/Go SDK logic port.

The SDK's complete standard Linux/macOS/Windows CI passes using the released
compiler. Compiler dependency `d3327688` also passes complete unified platform CI.
Twenty-four local integration checks cover independent C/Rust/Go clients, GC-safe
ownership, strings/arrays, field layout, errors/exits, aliases, pointer sizeof and
all nine-mode unchanged SDK callers. Actual C module/runtime resource sanitizers
and thirteen CI helper tests pass. The two new required integration gates pin the
SDK revision in compiler CI; 26 test groups now require 284 executions. Hosted
acceptance of the native integration gates is pending.

[SDK standard acceptance](https://github.com/SindarinSDK/sindarin-pkg-sdk/actions/runs/37978097898).
[Compiler dependency acceptance](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37975706163).
[Native module and contracts](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/723b3e2f59cad24504d8f83b23c132032fe641cc/docs/native/textfile.md).
Evidence: [SDK TextFile integration](rust-parity-evidence/sdk-textfile-integration.json).

This proves one independently built SDK C resource module, not complete SDK package
artifacts or generated Sindarin import/export adapters. The wider SDK migration,
complete package compilation/linking, shared ABI record/interface/callback/error
contracts, mixed-package Sindarin application and remaining Rust parity gaps remain
required full-goal work. The earlier local/pending descriptions below are historical.

## SDK field and lifetime preparation: local validation, 2026-10-09

A reusable C TextFile module has been prepared in the SDK using its original C
implementation and a package-owned layout/constructor. Existing Sindarin imports,
fields and methods retain the generated compatibility path. The SDK pilot is
locally verified with independent C/Rust clients, strings/arrays, exact field
layout, shared identity, explicit/final close and sanitizers. It remains unpublished
until compiler integration is accepted; full SDK/package adapters remain unfinished.

This exposed two compiler gaps. Native C field reads/assignments ignored physical
C aliases. The fix annotates native fields only, preserving ordinary serialization
aliases. Rust native handle fields now provide C string accessors with owned copies
and lifetime-preserving reads/stores. Compound field updates use the canonical
C getter/setter and existing arithmetic lowering, preserving shared native storage.
A frozen regression covers scalar/string mutation, aliases and pointer sizeof.

Independent C artifact builds now use the normal compiler's GNU feature macro and
reject implicit function declarations. A real strdup/free backing regression
prevents the pointer truncation exposed by the SDK pilot. Local suites pass 3,240
C checks and 1,083 Rust checks, 1,616 ASAN units, the nine-mode native field
comparison under the ASAN compiler, seven native artifact tests and thirteen CI
helper tests. The native fixture inventory grows to 379; existing fixture/oracle
contracts remain unchanged. Hosted acceptance is pending.

Evidence: [SDK field access validation](rust-parity-evidence/sdk-field-access-validation.json).
Generated package adapters, complete SDK decoupling and all full-goal requirements
remain outstanding.

## Native artifact builder: hosted acceptance, 2026-10-09

Main `f73853d6` passes complete unified Linux/macOS/Windows CI, including native
C/Rust/Go artifact builds, C/Rust consumers and cache controls. This accepts the
native backing stage; complete package compilation and generated adapters remain
unimplemented requirements. The earlier pending entry below is superseded.

[Accepted native artifact run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37970177022).

## Native backing artifact builds: local validation, 2026-10-09

The compiler now provides `--native-plan` and `--build-native`. An optional staged
Python 3 driver builds original C sources, Rust crate roots and Go bridge modules
with their own toolchains. It emits archives plus declaration/binding/ownership,
platform/runtime, initialization and native-link metadata. Export presence is
verified; incomplete artifacts explicitly remain `complete_package: false`.

C and Rust consumers link all three native backing languages, including a Go
export returning a string allocated by the shared C runtime. Conservative C/Rust
cache reuse checks metadata/archive bytes and compiler-discovered dependencies;
external header changes and corruption trigger rebuilding. Published generations
are immutable. Go inputs/link flags are recorded, but native Go artifact reuse and
multi-Go bridge aggregation remain pending. Incompatible host targets are rejected.

Local complete suites pass 3,240 C checks and 1,082 Rust checks. Six actual native
artifact tests pass normally and with the compiler under AddressSanitizer; all
1,616 sanitizer unit checks, fourteen existing package tests, four shared ABI
clients and thirteen CI helper tests pass. The new core gate retains 26 groups
and increases hosted validation to 280 gate executions. Existing source/oracle
contracts remain unchanged. Hosted acceptance of this batch is pending.

Usage and limits: [native backing artifacts](native-artifacts.md).
Evidence: [native artifact validation](rust-parity-evidence/native-artifact-validation.json).

Complete Sindarin package bodies, signature/type/layout verification, generated
export/import adapters, SDK decoupling, package graph linking and full Rust parity
remain required. Application compilation still diagnoses native-bearing manifests
as needing the incomplete package/adapters pipeline. No Go Sindarin target is claimed.

## Windows correction: hosted acceptance, 2026-10-09

Corrected main `d563f1b3` passes complete unified Linux/macOS/Windows CI. The
manifest byte-preservation controls and compiler profile initialization repair are
accepted; this lifts the feature publication freeze. The earlier pending/frozen
entry below describes the failing revision and is superseded by this acceptance.

[Accepted correction run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37967502055).

## Windows validation correction: local verification, 2026-10-09

Run `37964376134` fails the Windows C and Rust groups. The C unit suite compares
LF fixture text with actual bytes written by the Windows text stream as CRLF.
The rejected-manifest test now captures the file's bytes before the edit and
requires the same bytes afterward. A binary CRLF control requires exact unchanged
bytes on every platform. No original Sindarin source or runtime oracle is changed.

The Rust/native suite records nine C `-O0 -g` compilation failures reporting a
profile/debug conflict despite no `-p` flag. `compiler_init` omitted profile_build
initialization. Compiler option storage is now initialized before argument parsing,
including a false profile default and valid early-cleanup state. A regression
prefills options with three nonzero patterns; without the initialization fix it
deterministically reproduces the same conflict. All nine formerly failing sources
compile and execute with their exact existing byte oracles after the repair.

Local complete suites pass 3,240 C checks and 1,082 Rust checks. The 1,616-check
unit suite passes under AddressSanitizer; fourteen compiler package-contract tests
and thirteen CI helper tests pass. The 26-group/277-execution required CI catalog
is unchanged. Evidence is recorded in
[Windows validation correction](rust-parity-evidence/windows-driver-correction.json).

Both roots are combined into one correction batch. Feature publication remains
frozen until complete Linux/macOS/Windows CI on corrected integrated main passes.
The native artifact builder remains local and unpublished; native manifest
acceptance and the full Rust/shared-runtime/SDK goal remain unfinished.

## Native build/binding metadata: local validation, 2026-10-09

Native package manifests now declare shared ABI version, API declaration modules,
C/RS/GO build inputs, Rust crate roots, Go module directories, C-callable symbols,
parameter/result ownership, borrowed-result owners and explicit failure policies.
The reader extracts an owned JSON plan without dynamically owned data in recursive
package-manager configuration frames. Structural validation rejects malformed,
duplicate or unsupported contracts and unresolved build references.

Dependency add/update edits the original YAML document, preserving native plans
and unrelated root/dependency extensions. Sequence replacement preserves unrelated
aliases of the original dependency list. Node IDs are refreshed across libyaml
allocations. Invalid contracts and scalar-only native-config serialization are
rejected before replacing the destination.

Local validation passes 3,239 C checks and 1,082 Rust checks, all 1,615 unit tests
under AddressSanitizer, fourteen compiler package contract tests (42 compiler
invocations and eight executions), the same compiler contract under AddressSanitizer,
and thirteen CI helper tests. The existing package contract gate runs these new
checks on all three platforms; the 26-group/277-execution CI inventory is retained.
Existing Sindarin fixtures and output oracles remain unchanged.

The schema and implementation limits are documented in
[native package metadata](native-package-manifest.md); evidence is in
[native manifest validation](rust-parity-evidence/native-manifest-validation.json).
Hosted acceptance of this batch is pending.

This implements metadata reading/preservation and honest diagnostics. Native
artifact build execution, declaration/type validation, generated adapters, independent
SDK/package artifacts and all remaining full-goal obligations are unfinished.
Native-bearing manifests report the unavailable artifact/adapter pipeline during
application compilation instead of silently ignoring their declared backing.

## Shared C ABI foundation: hosted acceptance, 2026-10-09

Main `c0167613` passes complete unified Linux/macOS/Windows CI, including C/Rust/Go
clients and native Go archive consumption from Rust on each platform, and the
actual C implementation's address/undefined sanitizer lifetime gate on Linux.
This accepts the transport foundation, not complete generated package integration
or Rust parity. The historical pending entry below is superseded.

[Accepted shared-ABI run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37961423741).

## Shared C runtime ABI foundation: local validation, 2026-10-09

The staged C runtime archive now exports versioned value-transport operations.
Negotiation checks version, capabilities, layout size and host widths. Opaque
credits distinguish nil/empty strings and binary buffers, support borrowed views,
plain-value C arrays with copy/alias semantics, and resource destructor callbacks.
String concatenation and array allocation/growth/copy reuse the existing C runtime.
Reference credits are atomic; mutable payloads still require caller synchronization.

C, Rust and Go clients consume the same C archive. A native Go archive exports
C-ABI functions returning shared strings/resources; a Rust client consumes those
exports and performs final Go-resource cleanup on a worker thread. Registry tokens
retain Go state without retaining a Go object pointer in C. Cleanup runs exactly
once and reenters runtime services. Go builds use the stronger cgo pointer check.

Local validation passes all four clients, the actual C implementation under
AddressSanitizer/UndefinedBehaviorSanitizer (160,000 concurrent credit operations),
3,237 C checks, 1,082 Rust checks and thirteen CI helper tests. Existing fixtures
and oracles remain unchanged. The new ABI gates retain all existing coverage and
increase hosted acceptance to 277 executions across the same 26 groups. Go 1.26.0
is configured only for core-group clients; the sanitizer gate remains Linux-only.

The contract and limits are in [the shared runtime ABI reference](runtime-abi.md).
Evidence is in [ABI foundation validation](rust-parity-evidence/runtime-abi-validation.json).
Hosted acceptance of this batch is pending.

This is a transport foundation, not complete Rust parity or generated Sindarin
interop. Managed elements, public record/interface layouts, general callable/error
contracts, independent package artifacts, SDK decoupling, build/binding metadata,
automatic adapters and the full mixed-package Sindarin application remain required.
Existing generated Rust services have not yet migrated to the shared ABI.

## Package runtime contract: hosted acceptance, 2026-10-09

Main `39645c97` passes complete unified CI, including the newly required package
runtime contract gate on Linux, macOS and Windows. This accepts runtime manifest
selection/preservation and import diagnostics; all package/runtime/SDK architecture
and remaining parity requirements remain in force. The older pending entry below
is superseded by this acceptance.

[Accepted package-runtime run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37958281690).

## Package runtime manifest contract: local validation, 2026-10-09

The manifest now parses and serializes explicit `runtime: C / RS / GO`, retaining
omission as target inheritance and preserving runtime on dependency add/update.
Direct YAML mapping traversal isolates nested extension metadata from root fields
and dependency entries. Malformed/duplicate runtime declarations produce errors.

Compilation checks each imported source's owning manifest, including transitive,
namespaced, relative and canonicalized Unix symlink imports, before source/model
emission or linking. The application's manifest does not override its CLI target.
Matching/inherited packages retain existing source compilation and C sidecar
behaviour. Unsupported cross-runtime and GO dependencies fail explicitly instead
of being silently compiled for the consumer's target.

This establishes package selection metadata and diagnostics. Independent package
artifacts, native build/binding metadata, shared ABI, generated adapters and Go
native bridges remain unimplemented requirements. Full Rust parity is unfinished.
The new compiler-level package contract gate runs on all three hosted platforms;
only Windows symlink creation has an explicit host-privilege limitation. Existing
CI gates remain required: 26 test groups now cover 273 gate executions.

Local validation passes 3,237 C checks, 1,082 Rust checks, all eleven actual-
compiler package contract tests (34 compilations and eight executions), the same
contract under AddressSanitizer, and thirteen CI helper tests. Existing source
fixtures and behavioural oracles remain unchanged. Evidence is recorded in
[package runtime validation](rust-parity-evidence/package-runtime-validation.json).
Hosted acceptance of this implementation batch is pending.

## Accepted main before package runtime work, 2026-10-09

Callable integration `27ac5010` and the architecture/goal documents through
`40670069` pass their complete unified CI runs. The historical pending and frozen
entries below describe earlier revisions. The shared architecture and new goal
are persisted; implementation has resumed from accepted main. No outstanding main
CI remained when this batch started.

[Accepted main run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37946012526).

## Callable integration on accepted main: local verification, 2026-10-09

The preserved callable work is merged with accepted main `8ad475b9`, retaining
both the compiler lifetime repair and the pinned-library CI bootstrap. The
nullable comparison helper refreshes right-side metadata before processing the
left operand. Thirty-six actual-compiler checks with an ASAN JSON-lifetime guard
cover the original macOS failure, both nil operand orders, standalone nullable
transport and effectful replacement across all nine modes.

A further unchanged control exposed a C runtime failure when the assignment's
right-hand side replaces the same callable through a static method's reference
parameter. C captured the prior owner before executing that side effect. The
assignment now acquires the result first, then captures and releases the current
destination owner. All nine C/Rust comparisons require exact `final\n` output;
managed-owner sanitizers also cover the repair. This preserves the established
right-hand-side-first contract.

The required interface/callable matrix now has 25 frozen sources and 225 mode
comparisons, with 207 independent output oracles. The closure ownership gate
retains both record-reference fixtures and all three callable regressions: 120
executions, including 30 Linux sanitizer cases. Complete C/Rust suites, the
621-case callback matrix and CI helper checks are verified in the linked record.
Existing source/oracle contracts and generation goldens remain unchanged; the
previous uninitialized-callable limitation source remains preserved in the
archive and its runtime-failure test. Hosted acceptance of this integrated batch
is pending. Native interface ABI and all remaining documented ownership,
concurrency, SDK and language obligations remain full-goal work.

Evidence: [integrated callable validation](rust-parity-evidence/callable-integrated-validation.json).

## Integrated main acceptance and callable integration, 2026-10-09

Main `8ad475b9` passes all 34 unified CI jobs, including all 26 test groups and
270 gate executions across Linux, macOS and Windows. The nil-model lifetime
repair and direct pinned-library bootstrap are accepted. No main runs remain
queued or running at this checkpoint. The earlier pending/frozen descriptions
below are historical and superseded by this acceptance.

The preserved callable batch is being integrated onto that accepted revision.
Its nullable comparison lowering refreshes right-side type metadata after
replacement, retaining the compiler lifetime fix. Hosted acceptance of the
callable integration is pending. Native interface ABI and all other documented
full-goal obligations remain required work.

[Accepted main run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37845945158).

## Linux dependency bootstrap correction, 2026-10-08

Run `37832718573` verifies the nil-model repair on macOS: its build and every
macOS test group pass, including the complete Rust suite and callback matrix.
The Windows groups pass as well. Linux fails during bootstrap after the native
library archive download is reset. The released compiler's installer reports a
warning but returns success; compilation then lacks `json-c/json.h`. No Linux
compiler bundle is produced, causing ten downstream artifact-download failures.
These do not represent ten failed runtime test groups.

CI now checks out the same pinned library revision on every platform and invokes
the Unix project-library installer directly. Download retries and timeouts are
bounded, installer errors propagate, and actual compiler headers/static libraries
are checked before compilation. This removes the release-compiler prerequisite
from Unix CI bootstrap. Thirteen CI helper tests pass locally, including recovery
from two real local HTTP connection interruptions, rejection of installer failure
and rejection of missing build inputs. Shell syntax and workflow validation pass.
Full hosted acceptance remains pending; feature publication stays frozen until
corrected integrated main has green Linux/macOS/Windows CI.

## Native nil compiler crash: verified model lifetime repair, 2026-10-08

Main `b267f35f` fails two macOS gates on the unchanged native nil-result source.
The callback gate records three compiler SIGSEGVs, with the actual-process stack
in `json_string_property_equals` called from `rust_closure_walk`. The ordinary
Rust suite also fails compiling that source. The repeated eighteen-case gate
passes, so its clean result alone does not establish compiler stability.

The comparison validator borrows both operand type objects. Replacing the right
nil operand's type releases its old JSON object, leaving the cached right type
dangling before the second comparison. An ASAN-poison guard around JSON model
lookups deterministically reproduces this in the actual compiler on Linux and
records the free at the type replacement. Refreshing the borrowed right type
before the second comparison fixes the lifetime. Both the unchanged source and
a new left/right nil-comparison control pass all eighteen guarded compilations
and exact runtime oracles after the repair.

The new frozen control extends the required callback matrix to 69 sources and
621 C/Rust comparisons, and the fixed-output native suite to 378 sources. The
existing eighteen-case ordinary/traced compiler gate remains mandatory. Existing
sources and oracles are unchanged. Full local suites and matrix results are
recorded in the linked evidence; hosted acceptance is pending. This correction
is isolated from the unpublished callable transport batch. Feature publication
remains frozen until full Linux/macOS/Windows CI on corrected main is green.
Native interface ABI and the remaining full-goal obligations are unfinished.

Evidence: [nil model lifetime repair](rust-parity-evidence/native-nil-model-lifetime-validation.json).

## Callable replacement and nullable transport: local verification, 2026-10-08

An unchanged local control crashes the C executable in all nine modes when a
callable variable is reassigned a named function; Rust succeeds. C initialization
already worked, so the earlier initializer warning was stale. Reassignment now
creates its owned closure prefix and retains borrowed function values before
releasing the previous owner. Self-assignment and aliases remain valid, including
captured string storage after the original owner is cleared.

Rust nullable callable storage is now independent of native callback declarations.
Contextual nil transport covers function/lambda returns, arguments, static and
instance methods, record fields, array literals and assignments, default locals,
sized callable arrays and nested recursive captures. Two new hashed regressions
extend the required matrix to 24 sources and 216 mode comparisons, with 198
independent output contracts. Existing sources, positive runtime oracles and
existing generation goldens remain unchanged. The former uninitialized-callable
limitation source moves unchanged to an emission/runtime-failure test; its old
compiler diagnostic is archived. The invalid nil invocation is not credited as
a positive C/Rust parity case.

Local results and compiler/report hashes are recorded in the linked evidence.
The closure ownership gate retains its original record-reference sources and
adds both callable regressions: 96 executions, including 24 sanitizer cases on
Linux. The CI catalog retains 26 groups and 270 gate executions. Hosted
acceptance of this batch is pending; no backend completion is claimed.

Main follow-up run `37687229537` has no failing completed jobs, but one Linux
job remains in toolchain setup. Older main runs also have live Linux setup jobs;
their previously observed Windows stream and macOS compiler failures are
retained. Infrastructure commit `b267f35f` is published on main with setup deadlines and
without unnecessary Linux prerequisite downloads. Its independent unified run
`37702160175` uses the second validation slot; the Linux build already passes
setup and completes. The callable batch remains local until a slot clears. The intermittent macOS compiler crash and native interface
ABI remain required work.

Evidence: [callable ownership validation](rust-parity-evidence/callable-ownership-current-validation.json).


## Integrated interface and CI correction acceptance, 2026-10-07

Corrective main `45fc52be` passes complete Linux/macOS/Windows compiler CI.
The final coverage artifact verifies all 26 groups and 270 gate executions,
with one compiler checksum per platform and the exact integrated revision.
The Windows lifetime audit passes with its producer-specific raw stream oracle;
macOS passes all 612 callback comparisons and all eighteen repeated nil compiler
cases with tracing and malloc scribbling enabled. This accepts the integrated
interface storage and audit increments and lifts the required-CI publication
freeze. It does not explain the earlier intermittent macOS compiler SIGSEGVs,
which remain open investigation work. No full backend completion is claimed.
The follow-up compiler stability gate exercises both ordinary and traced
compilation (nine mode cases each) because tracing can change the conditions
of the intermittent fault. All eighteen cases pass locally; hosted acceptance
of this follow-up remains pending.

Evidence: [hosted acceptance and remaining limits](rust-parity-evidence/interface-storage-main-ci-green.json),
[workflow run](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37681576979).

## Actual-process compiler crash tracing and original corpus audit, 2026-10-07

The Windows core gate now passes on corrective run `37677690009`. The macOS
native nil compiler crash recurred in both the full callback gate and the
repeated regression. Its LLDB diagnostic invocation succeeded, so that log does
not explain the original fault. An opt-in `SN_COMPILER_BACKTRACE=1` handler now
records the stack inside the actual faulting compiler process on macOS/Linux,
while preserving the fatal signal. Callback CI and the repeated compiler gate
enable it; normal compiler invocation retains its default signal behaviour.
The local forced-SIGSEGV contract preserves exit status -11 with tracing either
on or off. The unchanged native nil source passes its eighteen local repeated
cases and all nine C/Rust mode comparisons with tracing enabled. Mac root cause
and hosted acceptance remain pending; feature publication remains frozen.

A fresh original-program audit executes all 1,365 tagged integration/exploratory
programs through Rust with their unchanged C-harness contracts at O0/debug on
Linux. All pass, with zero skips. All 1,155 tagged output-oracle files match.
One historical fixture differs from the tag: the current closure-array control
adds synchronization. The audit stages and compiles the exact original bytes
separately, leaving the synchronized current file intact. This execution result
does not establish defined concurrent-race semantics or all-mode raw parity.
An initial custom executable basename violated the `test_main_args` argv[0]
contract; that invalid harness result is retained and the case passes using its
standard test filename. No program or expected output is changed for this audit.

Evidence: [crash trace contract](rust-parity-evidence/compiler-crash-trace-validation.json),
[original corpus result and limits](rust-parity-evidence/original-corpus-current-validation.json).

## CI correction and native nil compiler investigation, 2026-10-07

The required Windows interface lifetime audit failed because it applied the CRT
stdout newline contract to the injected Rust stderr diagnostic. Rust `eprintln!`
uses LF. The corrected audit retains exact CRLF Sindarin stdout on Windows and
requires exact `metadata_entries:0\n` stderr, zero exit status and unchanged
sources. It now writes raw observations before raising an audit error. Ten CI
helper tests pass, including rejection of metadata leaks, nonzero exits and
incorrect stream bytes. All twelve Linux lifetime audits pass; an emitted-Rust
Windows CRT stream simulation confirms the mixed newline producers. Actual
hosted Windows acceptance is still pending.

Run `37667533109` also records three macOS compiler SIGSEGVs during code
generation for the unchanged native nil-result fixture. C succeeds in each
case. The same callback gate on `b08ccc38` passed, but that does not explain or
close the intermittent compiler crash. GCC ASAN and allocator-perturbed release
probes plus optimized Clang ASAN/UBSAN probes have not reproduced it locally.
A new required 18-case repeated compiler regression uses the frozen source and
oracle, with malloc scribbling on macOS and crash/debugger evidence retained on
failure. Failures remain failures; diagnostic reruns receive no parity credit.

This is a corrective batch. Feature publication remains frozen until complete
Linux/macOS/Windows CI on corrected main is green. The catalog retains all prior
checks and now requires 270 gate executions across 26 groups. The unexplained
macOS crash remains a full-goal obligation.

Evidence: [CI correction and remaining investigation](rust-parity-evidence/interface-ci-correction-validation.json).

## Interface C storage and closure scopes: local verification, 2026-10-07

The recovered interface implementation now uses the selected C compiler for
zero-sized function, method and closure scope storage. C closure bodies copy
captured value records into invocation-local slots; Rust now preserves that
identity for escaping and nested captures as well. Assignments retain the
existing destination identity and evaluate the source value first.

The required matrix passes all 198 comparisons across 22 frozen sources under
both GCC and Clang on ARM64 Linux, covering nine optimization/arithmetic modes.
It includes both unchanged original interface programs. Existing sources and
oracles are preserved. Four compiler-sensitive empty-local controls retain their
historical literal oracle plus one explicit C-supported alternative, with exact
C/Rust output equality still mandatory. Eighteen new storage probes additionally
compare actual compiler-selected identity directly against C.

All 505 Rust generation tests, 119 rejection tests, 377 fixed-output native
fixtures and twelve provenance lifetime audits pass locally. Complete C suites
pass. Interface matrix and lifetime gates are added to all three CI platforms;
the catalog retains every previous gate and now requires 267 executions across
26 groups. Hosted acceptance of this increment is pending. Base main `d928d615`
has green three-platform CI: [run 37657799360](https://github.com/SindarinSDK/sindarin-compiler/actions/runs/37657799360).


The first hosted run passed all 198 Linux comparisons but the source verifier
rejected the new mixed-scope fixture: the pre-commit hook formatted two struct
literals after its hash had been frozen. The follow-up pins the committed bytes,
checks all frozen sources before compilation, and verifies the hosted Linux
report successfully. Existing source programs and output expectations are unchanged.

Native opaque interface parameters remain rejected. A fresh C-valid field
read/write and identity probe passes C in all nine modes while Rust rejects its
native signature. Native ABI and retained storage are next implementation work;
this increment does not establish full goal completion or corpus-wide acceptance.

Evidence: [current validation](rust-parity-evidence/interface-storage-current-validation.json),
[implementation and limits](rust-structural-interfaces.md).

## Value-record reference capture snapshots: local verification, 2026-10-07

Rust previously rejected the unchanged C regression
`test_lambda_capture_record_ref_snapshot` as a borrowed capture. Its C model
specifies `struct_copy`: the closure owns a snapshot copied from the referenced
value record. Rust now admits that existing ownership contract for supported
auto-copy plain value records, retaining the separate guards for shared mutable
captures, reference-record handles and user copy hooks.

The original source is copied byte-for-byte into a Rust generation/runtime
fixture. A second source checks nested strings and arrays, forwarding, escaping
scope owners, caller mutations after capture and nested closure snapshots.
Both complete local suites pass without failures or skips, with existing
goldens unchanged. The frozen ownership gate passes 36 literal C/Rust oracle
executions in all nine optimization/arithmetic modes and six sanitizer runs
per backend on Linux. This gate is required by three-platform compiler CI.
Hosted acceptance is pending. This closes a newer C regression source's Rust
rejection; it does not change the 1365-source original corpus denominator or
establish full backend completion.

Evidence: [local validation](rust-parity-evidence/record-reference-snapshot-validation.json).

## Interface recovery on current backend: unpublished research, 2026-10-07

The earlier `b7e0686c` interface implementation is recovered onto current code,
with the intervening callback and ownership paths preserved. Both original
interface sources pass all 18 GCC C/Rust mode comparisons. Complete Rust suites
and all 12 metadata lifetime audits pass locally, with existing goldens unchanged.

Clang independently reproduces the portability failure: empty source locals
compare equal in C and distinct in Rust. Two preserved controls fail all nine
modes, yielding 18 failures in the 36-case focused Clang matrix. This work is
unpublished, receives no accepted parity credit, and requires a real storage
repair plus native ABI and ownership composition before hosted acceptance.

Evidence: [current recovery](rust-parity-evidence/interface-recovery-current-validation.json),
[implementation and limits](rust-structural-interfaces.md).

## Static callback temporary cleanup: local verification, 2026-10-07

Current main `16cf5b7f` fails Linux Compiler CI because the unchanged
`test_fn_ref_static_method_arg` leaks a 32-byte closure header. Static-call
projection already marks function argument temporaries for cleanup, but its C
template ignored those annotations. The template now creates scoped
`sn_auto_fn` owners and passes them to the callee. Stored and returned function
handles retain their independent credits; escaping captured callbacks remain
usable after the call.

The original failure reproduces on this aarch64 worker when LeakSanitizer
ignores dead register/stack roots. Both unchanged original and new retained,
copy, capture and multiple-argument controls pass 36 literal-oracle executions
across C/Rust and all nine optimization/arithmetic modes, plus six C
ASAN/UBSAN/leak executions. Complete C and Rust suites pass with no failures or
skips and existing goldens unchanged. Compiler CI now requires this ownership
gate on all three platforms, with sanitizers on Linux. Hosted acceptance is
pending; interfaces and the complete backend objective remain unfinished.

Evidence: [local validation](rust-parity-evidence/static-callback-ownership-validation.json).

## Native callback increment: local acceptance, 2026-10-07

The current Rust increment uses typed ABI thunks and C-owned closure headers with identity-preserving credits. It covers nil, scalar/pointer/string/value-record signatures, canonical primitive/character/string/record/nested arrays, actual native pointer aliases, reentry, private record and array snapshots, retained callback identity and joined-thread transport. The 68-source permanent gate passes 612 mode comparisons; 204 C and 204 Rust/native-C sanitizer cases pass. Both complete suites pass with 364 native fixtures, no failures or skips, and existing goldens unchanged.

This work is prepared against `9da559c3` and is not published. Its parent’s six compiler/runtime jobs and three platform artifacts are independently accepted: 65 reports and 5,961 cases per platform. All 98 required runtime commands pass (93 executed in the combined run and five reused from exact-byte checks), and independent verification confirms 66 reports and 6,573 cases against 8,907 frozen repository inputs. Broader qualifiers, threaded reentry/concurrent buffers, additional owners/layouts and the rest of the full backend goal remain open. See [native callback contracts and limits](rust-native-callbacks.md) and [local validation](rust-parity-evidence/native-callbacks-validation.json).

## Record capture copy source: local verification, 2026-10-07

A managed value record captured from an `as ref` parameter already supplies a record pointer. The C closure constructor previously took that pointer’s address and copied the stack slot as a record. The unchanged regression source triggers a stack-buffer-overflow at `-O0` and an undefined memory read at `-O1` and `-O2` under sanitizers. Both C model projections now mark pointer sources so the shared constructor copies the referenced record. Existing owned snapshot semantics and cleanup remain intact.

The new C integration fixture checks snapshots after original scalar/string mutation, after source scope exit through a forwarded parameter, and through nested closures. All nine optimization/arithmetic runs and three address/undefined/leak sanitizer runs pass with exact output. Both full local suites pass without failures or skips; existing goldens are unchanged. This small increment is prepared against `c5b6d3dd` and has not been published. Ordinary borrowed managed-record captures in Rust, native callbacks, and wider array wire formats remain unfinished.

Evidence: [original failure](rust-parity-evidence/record-ref-capture-copy-before.json), [fixed executions](rust-parity-evidence/record-ref-capture-copy-after.json), [validation and limits](rust-parity-evidence/record-ref-capture-copy-validation.json).

## Borrowed scalar captures: published as `d0b44ecf`, 2026-10-07

The published increment preserves the caller’s scalar storage in escaping and nested closures. It covers all scalar kinds, aliased references, qualified factory copies, globals, thread transport, record fields and nested fields, owner-preserving record assignments, native scalar mutation, and native record value arguments/results. The original borrowed-capture source is promoted byte-for-byte from an error fixture. C closure capture fields now hold and dereference caller pointers, and copied function handles retain their closure owners.

The permanent gate passes all 21 sources and 189 optimization/arithmetic comparisons, with 63 sanitizer executions per backend. Both complete local suites pass without failures or skips. All 89 required commands pass after repairing a fixed-count packaging mistake. The strict verifier confirms 65 reports and 5,961 cases against 6,925 frozen inputs. All 1,365 original corpus sources are unchanged. The discarded packaging run is retained separately. This increment is on main as `d0b44ecf`. Windows setup then failed before building because the bootstrap compiler rejected GitHub’s certificate. Repair `c5b6d3dd` uses an authenticated pinned dependency checkout. Its compiler and runtime jobs pass on all three platforms. Each Linux, macOS and Windows runtime artifact independently verifies 65 reports and 5,961 cases, with all six hosted suite logs checked.

Native record references combined with captured fields remain rejected: source fields use shared owner storage while C requires a persistent compatible record layout and may retain the actual record address. Temporary copy-back is insufficient. Native closure callbacks also remain rejected. These are unresolved parity gaps, alongside borrowed method-self captures, interface identity, native string aliases and wider native array formats and lifetimes.

## Physical value-record receivers, 2026-10-06 — published as `adaf1351`

The prepared Rust change gives plain scalar-field record arrays a stable owned
header and raw buffer with C's minimum capacity and doubling policy. Clones
retain capacity; removal and clearing retain initialized physical slots.
Indexed calls check the source index before evaluating arguments, then pass a
raw slot pointer into a generated method helper. Ordinary and closure calls,
effectful indexes, nested calls and recursion use that receiver representation.

Nil arrays, concatenation, slices and explicit sized-array defaults use the same
storage. A nil concat operand copies the other operand with its original spare
capacity. Foreach caches the original header and length, reloads its current
buffer for each Copy value, and releases cell borrows before the body. This
preserves reversal, growth and private parameter rebinding during iteration.
C explicit plain-record sized defaults now assign the record to a temporary
before pushing it; per-element effects and zero-size behavior remain intact.

Twenty-two permanent fixtures pin source hashes and literal outputs for **198**
comparisons and **66 C + 66 Rust** sanitizer executions. All **86** combined
acceptance commands pass with frozen inputs, including the complete C/Rust
suites, exactly **278** native fixtures and strict **64 reports / 5772 cases**.
All **1365** original source hashes and existing goldens remain unchanged; the
new fixtures are formatted and independently hashed. The change is published as `adaf1351`. Its compiler and runtime jobs passed on all three platforms; each runtime artifact independently verified 64 reports and 5,772 cases. Its new gate runs on all three platforms, with C/Rust sanitizers on Linux.

The discarded Vec prototype and nil-concat implementation both caused Rust
use-after-free failures under ASAN; neither receives parity credit. Borrowed
method-self captures remain a verified gap: C's lambda observes later receiver
mutation, while Rust rejects the capture. Managed record fields, native layouts,
interface identity, wider lifetime/qualification/thread cases and the complete
backend goal remain unfinished. C stays the default target. The physical receiver increment has full hosted acceptance; the broader backend goal remains open.

Evidence: [validation](rust-parity-evidence/physical-receiver-validation.json),
[oracles](rust-parity-evidence/physical-receiver-oracles.json),
[comparisons](rust-parity-evidence/physical-receiver-after.json),
[C lifetimes](rust-parity-evidence/physical-receiver-c-lifetimes.json),
[Rust lifetimes](rust-parity-evidence/physical-receiver-rust-lifetimes.json),
[review failures and remaining gap](rust-parity-evidence/physical-review-findings.json).

## Windows native test compilation timeout, 2026-10-06

An earlier Windows runtime job failed when `scalar_alias_partitions_5` reached
its native harness's 60-second compilation limit. The same fixture passed later
in that job and on the following revision. Native compilation now uses the
existing generated-Rust test budget: at least 120 seconds for Rust on Windows.
C compilation and program execution retain their limits; larger caller limits
remain effective. Timeouts still fail, with no retries or oracle changes.

Eight focused routing checks pass, and the unchanged fixture passes against the
current main compiler on Linux. Exact-revision hosted checks are required after
publication. This is a harness reliability repair and earns no feature-parity
credit. [Evidence](rust-parity-evidence/windows-native-compile-timeout.json).

## Owned closure array rebinding, 2026-10-06

`as val` closure array assignment now replaces the per-call copied vector owner
through its borrowed cell instead of trying to replace the reference itself.
Caller arrays remain intact. Subsequent mutations, returned arrays, independent
formals and captures taken before/after rebinding retain their ownership rules.
The RHS executes before the short mutation borrow; tuple binding prevents local
helper names from shadowing source parameter names during assignment.

Record-element stores now hold a guard of the original closure-array owner for
the final store, with RHS and raw index effects evaluated before that guard.
Deferred native handle arguments remain borrowed across argument binding and
call entry, preserving native reference credits and cleanup.

Eleven literal-oracle fixtures cover **99** C/R comparisons across all nine
optimization/arithmetic modes, including nested/string/record/native elements,
snapshot captures, two independent as-val formals, self/RHS effects and hygiene.
Linux ASAN/UBSAN/leak checks cover **33** executions. The native helper source is
frozen and independently hashed; exact output pins native counts and zero live
objects after cleanup. All **73** combined acceptance commands pass with frozen inputs, including
complete C/Rust suites, exact **198** native fixtures and **58 reports / 5043
cases**. Existing test sources and goldens and all **1365** original corpus
hashes remain unchanged. The change is prepared against repaired main
`5d19a3af`; all six repair jobs and retained 57 reports / 4944 cases on each
platform are now verified. This increment is ready for publication.
This increment requires its own exact-revision hosted acceptance after push.

The array assignment-result expression crashes the C baseline and is excluded;
default array rebinding and string default/as-val rebinding also have C lifetime
failures and receive no parity credit. They remain required repair work.
Remaining interfaces/native C wire/empty physical storage, physical value-record
receivers, qualifications/custom copies, wider callbacks/globals/escaping/thread
lifetimes and evaluation contexts remain required. C stays the default target;
rejections, skips and unsafe C probes do not count as parity or goal completion.

Evidence: [validation](rust-parity-evidence/array-rebinding-validation.json),
[oracles](rust-parity-evidence/array-rebinding-oracles.json),
[comparisons](rust-parity-evidence/array-rebinding-after.json),
[C lifetimes](rust-parity-evidence/array-rebinding-c-lifetimes.json).


## Closure string parameter ownership, prepared 2026-10-06

Default and `as val` string closure parameters now support local replacement
and compound append with caller contents preserved. C keeps the incoming
pointer borrowed until replacement, tracks only private replacements in a
cleanup slot and copies assignment-expression returns before that slot exits.
Exact AST parameter facts and the current lambda's own parameter context select
these slots. Locals, shadowed bindings and outer captured parameter names use
their existing ownership paths. Rust mutable value formals return independent
assignment values; captures made before and after replacement remain snapshots.

Eight unchanged-source literal controls pass **72** mode comparisons and **24**
Linux ASAN/UBSAN/leak runs. All **75** combined acceptance commands pass with frozen
inputs, including complete C/Rust suites, **59 reports / 5115 cases**, exact
**206** native fixtures, **501** Rust and **107** C generation goldens and
**121** admission-error fixtures. Existing source/goldens and all **1365**
original corpus hashes remain unchanged. The increment is validated against array main
`02a67749`, preserving both changes. Own exact-revision platform acceptance
remains required after publication. Its exact-revision hosted acceptance remains required.

Native physical string aliases remain a demonstrated gap. The same source string
passed to two closure formals yields one C buffer address but separate Rust
buffers. The no-mutation control already differs on existing main; the rebinding
control differs on this candidate. All nine modes report C `true/true`, Rust
`false/true` with compile/run status zero. Six C sanitizer runs pass. These
controls receive no parity credit; this ownership increment does not solve their
native storage contract. Interfaces/native wire/empty physical storage, physical
value-record receivers and remaining qualifiers and lifetime/evaluation contexts
still prevent full completion.

Evidence: [validation](rust-parity-evidence/string-rebinding-validation.json),
[oracles](rust-parity-evidence/string-rebinding-oracles.json),
[comparisons](rust-parity-evidence/string-rebinding-after.json),
[C lifetimes](rust-parity-evidence/string-rebinding-c-lifetimes.json),
[native alias gap](rust-parity-evidence/string-native-alias-gap.json).

## Private closure array bindings, prepared 2026-10-06

Default array formals now preserve the caller's actual storage for element and
method mutations until rebinding. A rebind replaces only that invocation's formal
with private owned storage; other aliases retain their previous owner. C tracks
private replacements in cleanup slots instead of freeing borrowed caller arrays,
and copies array assignment-result returns before local cleanup. Rust uses a
hygienic borrowed/owned enum with mapped `Ref`/`RefMut` guards in programs that
require default-formal rebinding. Dynamic alias dispatch compares actual owner
cell addresses; the implementation adds no unsafe Rust storage casts.

Fifteen literal controls pass **135** mode comparisons and **45** Linux
ASAN/UBSAN/leak runs. They cover self/nil/return assignment, forwarding,
snapshots before/after rebinding, mixed default/as-val formals, nested arrays,
dynamic partitions, real native array header identities/reference counts/zero
live objects, once-only RHS effects and the C wire `sizeof` result. RHS borrow
guards end before a hygienic temporary borrows the formal for replacement.
All **77** combined acceptance commands pass with frozen inputs, including complete
C/Rust suites, **60 reports / 5250 cases**, exact **221** native fixtures,
**501** Rust/**107** C goldens and **121** admission-error fixtures.
Existing sources/goldens and all **1365** original corpus hashes remain unchanged.

This candidate is validated against array/string main `f8421976`. Its own
complete exact-revision hosted verification remains required after publication. Ordinary primitive native array parameters are
still rejected by Rust, and native string physical aliases remain different.
An authored named-function array-forwarding probe crashes C and gets no credit;
a captured scalar reference-forwarding control requires a separate repair.
Interfaces/native wire/empty physical identity, physical value-record receivers
and remaining qualifiers and lifetime/evaluation contexts prevent completion.

Evidence: [validation](rust-parity-evidence/private-array-validation.json),
[oracles](rust-parity-evidence/private-array-oracles.json),
[comparisons](rust-parity-evidence/private-array-after.json),
[C lifetimes](rust-parity-evidence/private-array-c-lifetimes.json).

## Native primitive array headers, prepared 2026-10-06

Native boundaries now admit arrays of int/long/int32/uint/uint32/byte/bool/float/
double using actual C headers, buffers and ownership callbacks. A program with a
primitive native array boundary uses canonical C-backed storage for compatible
primitive arrays, preserving repeated argument header identity, native element
writes/growth and native-owned returns. Rust does not lend a Rust allocation to C
reallocation or substitute a copied header. Hygienic allocator helpers report C
element width/alignment and verify it against Rust before allocation. Array
support is owned by the native plan even in programs with no native handles.

Canonical arrays provide the byte-encoding view used by existing code. Byte views
read the logical count of bytes from the actual C object representation, including
native returns whose static byte[] type retains an int-sized C header. The prior
width-assertion panic is retained without credit; the unchanged fixture now passes
all nine modes and the complete byte-encoding gate. Other element views keep their
matching-width requirement.

Thirteen frozen native controls and helper hashes pass **117** mode comparisons
and **39** Linux ASAN/UBSAN/leak runs. All **79** combined acceptance commands pass
with frozen inputs, complete C/Rust suites, exact **234** native fixtures,
**61 reports / 5367 cases**, **501** Rust/**107** C goldens, **121** admission-error
cases and **four** native diagnostic cases. Existing positive sources/goldens and
all **1365** original corpus hashes remain unchanged. Two byte-identical negative
sources now expect missing-symbol link failures after their array types become
admitted; both C and Rust link-failure logs are retained. They receive no
executable parity credit; native-return positives have actual C definitions.

The change is validated against private-array main `aa771e9e`; its own complete
exact-revision hosted acceptance remains required after publication. Native
char/string/record/nested array formats, qualifications/callback and escaping
lifetime contexts, native string physical aliases, interfaces/native wire/empty
physical identity and physical value-record receivers still prevent completion.

Evidence: [validation](rust-parity-evidence/native-primitive-arrays-validation.json),
[oracles](rust-parity-evidence/native-primitive-arrays-oracles.json),
[comparisons](rust-parity-evidence/native-primitive-arrays-after.json),
[C lifetimes](rust-parity-evidence/native-primitive-arrays-c-lifetimes.json),
[diagnostics](rust-parity-evidence/native-primitive-arrays-diagnostics.json).

## Scalar snapshot reference forwarding, prepared 2026-10-06

Captured scalar values forwarded by reference now mutate a private copy for each
invocation, preserving the captured snapshot and caller's original. Mutation
arguments refer to the local place instead of a cloned read temporary. Ordinary
scalar reference callees in closure-reference programs use the existing borrowed
reference protocol before alias preparation, so repeated arguments preserve
actual aliases without mismatching an `Rc<Cell>` with a scalar snapshot local.

Eleven new native-suite controls and one byte-identical promoted admission-error
source pass **108** C/R comparisons and **36** Linux ASAN/UBSAN/leak runs. Coverage
includes repeated same/distinct references, ordinary/native/closure calls,
characters, booleans, bytes and floats, nested capture epochs and mixed caller
references. All **81** frozen combined acceptance commands pass, complete C/Rust suites,
**62 reports / 5475 cases**, exact **245** native fixtures, **502** Rust generation
positives and **120** admission-error cases. Existing positive source/goldens and
all **1365** original corpus hashes remain unchanged. The promoted source's bytes
are identical; its original nine-mode zero-status C/R proof is retained.

The candidate is validated against primitive/private-array/string main
`aea96d33`; its own complete exact-revision hosted acceptance remains required
after publication. A shadowed-capture control fails to compile in C and
receives no credit; lexical capture repair remains separate work. Native string
physical aliases, primitive native array parameters, interfaces/native wire/empty
physical identity, physical value-record receivers and remaining qualifiers and
lifetime/evaluation contexts still prevent full completion.

Evidence: [validation](rust-parity-evidence/scalar-snapshot-reference-validation.json),
[oracles](rust-parity-evidence/scalar-snapshot-reference-oracles.json),
[promotion](rust-parity-evidence/scalar-snapshot-reference-promotion.json),
[comparisons](rust-parity-evidence/scalar-snapshot-reference-after.json),
[C lifetimes](rust-parity-evidence/scalar-snapshot-reference-c-lifetimes.json).

## Lexical closure capture scopes, 2026-10-06

Both projections discover locals in source order and restore visible names at
block, branch and loop exits. Ordinary initializers read the prior visible
binding; only typed recursive lambda initializers bind themselves early, matching
type checking. A nested or later same-name local no longer suppresses an outer
capture. C suffix blocks scope late declarations, and initializer temporaries
evaluate captured-name RHS values before introducing the shadowing local, including
nested body declarations. The raw helper names cannot collide with source names
that receive the compiler's `__sn__` prefix. Heap cleanup and recursive identity
remain intact. The snapshot-reference repair on main now composes with shadowing.

Eleven frozen literal controls pass **99** mode comparisons and **33** Linux
ASAN/UBSAN/leak runs: block/later/iterator/while shadowing, heap values, nested
captures, recursion, scalar/heap/nested initializers and reference forwarding.
All **83** combined acceptance commands pass with frozen inputs and complete
C/Rust suites, exact **256** native fixtures, **63 reports / 5574 cases**,
**502** Rust/**107** C goldens and **120** admission-error cases. Existing
source/goldens and all **1365** original corpus hashes remain unchanged.
The change is validated against snapshot main `ccbff0e1`; its own full
exact-revision platform acceptance remains required after publication.

Interfaces/native wire/empty physical identity, physical value-record receivers,
native string aliases, remaining native array formats and qualifiers/callback/
lifetime/evaluation contexts still prevent completion. The physical receiver
control remains C-valid with sanitizer evidence while Rust still panics; no
snapshot/reindexed receiver or rejected case receives parity credit.

Evidence: [validation](rust-parity-evidence/capture-scope-validation.json),
[oracles](rust-parity-evidence/capture-scope-oracles.json),
[comparisons](rust-parity-evidence/capture-scope-after.json),
[C lifetimes](rust-parity-evidence/capture-scope-c-lifetimes.json).

## Scalar closure RHS sequencing: CI repair, 2026-10-06

macOS runtime job 112380383034 and Windows job 112380383080 for `a69da434` failed the unchanged scalar
value-parameter RHS control in all nine modes. C printed `false/true` where Rust
printed `true/true`: the C assignment embedded an effectful function call in an
unsequenced arithmetic expression. Linux GCC had produced the expected `55`,
while macOS Clang exposed the earlier-value result. The recorded failure receives
no parity credit; neither the source nor its literal output oracle is changed.

C now sequences a call-containing RHS of a current lambda's own numeric value
parameter before reading/updating its local mutation place. The RHS executes once
and the target variable address is formed after the RHS. Explicit reference
formals and captured outer-parameter bindings retain their previous paths;
lexical AST parameter facts and the current lambda's parameter list select the
repair. No OS condition or expected-output normalization is used.

The unchanged failure fixture passes all nine modes with GCC and project-local
Clang. All **71** final acceptance commands pass with frozen inputs, including the
complete C/Rust suites, exact **187** native fixture requirement and all
**57 reports / 4944 cases**. Existing C/Rust/model generation goldens and test
sources remain unchanged after a template EOF newline regression was found
and corrected. All **1365** original corpus source hashes are verified.
Repaired main's exact-revision hosted acceptance remains required.

All six repaired-main jobs and retained platform reports are now green and
independently verified: [hosted proof](rust-parity-evidence/closure-rhs-ci-repair-main-ci-green.json).
Array rebinding is validated against this accepted main. The full parity objective remains
incomplete, including interfaces/native physical storage and lifetime contexts.

Current main `2b74dc28` also inherited the same macOS failure in job
112397865327; the collected log identifies only the nine RHS cases as failed.
The repair is based on current main, preserving the record increment.

Evidence: [failure](rust-parity-evidence/closure-rhs-ci-repair-failure.json),
[GCC/Clang proof](rust-parity-evidence/closure-rhs-ci-repair-targeted-proof.json),
[complete validation](rust-parity-evidence/closure-rhs-ci-repair-validation.json).

## Owned record closure parameters, 2026-10-06

Supported auto-copy value records declared `as val` in closure signatures now
use independent per-call owners and mutable local bindings. Scalar/string/array
changes stay private to the copy; returned records and escaping capture snapshots
retain independent owners. Variable, field, array-element and temporary arguments
are covered by unchanged C/R source controls.

Nested mutable receivers project the original owned record field storage. Method
arguments are evaluated before receiver locking, and cached reference arguments
retain their borrowing roles. Existing array alias specialization now admits
stable nested receivers rooted in owned record parameters. Receiver-field aliases
and duplicate/dynamic array-formal partitions specialize before argument caching;
selected leaves form their borrows once. Earlier clone-only receiver updates and
RefCell alias panics are preserved as failed attempts, never counted as parity.

Seventeen literal-oracle controls check **153** C/R comparisons across all nine
optimization/arithmetic modes, with **51** Linux ASAN/UBSAN/leak executions.
All **71** final acceptance commands pass, including the complete C/Rust suites
and the exact count-checked **187** native fixtures. Frozen evidence verifies
**57 reports / 4944 cases**, including all prior **56 / 4791**. Existing source
and generation expectations remain unchanged, and all **1365** original corpus
source hashes are verified. Exact-revision Linux/macOS/Windows hosted acceptance
remains required.

The separate record assignment-result expression crashes the C baseline, and
string/array rebinding baseline probes also crash C; they receive no C-valid
parity credit. A default nested forwarding helper failed C ABI compilation;
its explicit-reference sibling is the admitted source. An insert probe used
reversed argument roles; the corrected source follows `insert(value, index)`.
The wrong source and false assertions are excluded from accepted fixtures.

Remaining qualified record/reference/native/custom-copy contexts, owned parameter
rebinding/lifetimes, structural interfaces and C wire ABI/empty storage, physical
value-record receivers, and broader callback/global/escaping/thread/evaluation
compositions remain required work. C remains the default target. Corpus coverage
is a separate metric from full-goal completion; guards/skips/rejections do not
count as parity.

Evidence: [validation](rust-parity-evidence/owned-record-parameter-validation.json),
[oracles](rust-parity-evidence/owned-record-parameter-oracles.json),
[comparisons](rust-parity-evidence/owned-record-parameter-after.json),
[C lifetimes](rust-parity-evidence/owned-record-parameter-c-lifetimes.json).

## Scalar closure value parameters, 2026-10-06

Default and explicit `as val` scalar closure parameters now have mutable local
bindings for each invocation. Mutations and reference forwarding affect the
local copy and leave the caller's value intact. Type/operator checks remain
active; the private lexical facts admit numeric mutation through the existing
checked/wrapping/floating lowering. Nested capture snapshots and lexical
shadowing retain their existing ownership and binding identity.

Two original rejection fixtures are promoted with their source bytes unchanged.
The gate supplies literal outputs for **16** programs in **144** comparisons
across all nine optimization/arithmetic modes, with **48** Linux ASAN/UBSAN/leak
runs. The float/postfix case pins `16777220` and an effectful RHS case pins `55`,
matching authored raw output controls; initial incorrect assertions receive no
credit. The separate floating-modulo negative is C-invalid and remains excluded
from parity. Its diagnostic now reaches the specific unsupported operator
instead of the former closure-parameter mutation guard.

All **69** final acceptance commands pass with frozen inputs. The complete
C/Rust suites pass, including the exact count-checked **170** native fixtures.
Evidence verifies **56 reports / 4791 cases**, all prior **55 / 4647** included.
Rust generation has **501** positives and **121** negatives with no failures
or skips. All **1365** original corpus source hashes remain unchanged; no
existing generation goldens change. Exact revision hosted acceptance remains
required on Linux/macOS/Windows. Source/operator support outside these verified cases, owned parameter
rebinding, remaining qualifiers, structural interfaces/native ABI and empty
storage, physical value-record receivers and broader native/global/callback/
escaping/thread lifetime/evaluation compositions remain required work. C stays
the default target. Original corpus coverage is a separate measure from goal
completion; rejected or skipped features receive no completion credit.

Evidence: [validation](rust-parity-evidence/value-parameter-validation.json),
[oracles](rust-parity-evidence/value-parameter-oracles.json),
[comparisons](rust-parity-evidence/value-parameter-after.json),
[C lifetimes](rust-parity-evidence/value-parameter-c-lifetimes.json),
[promotion](rust-parity-evidence/value-parameter-promotion.json).

## Scalar closure references and reference evaluation, 2026-10-06

Rust scalar `as ref` closure parameters now share the caller's storage. A
private wrapper reads/writes through short borrows of local storage or through
the existing shared record field owner. Named functions and instance/static
methods in these programs use the same place protocol so dynamic aliases remain
aliases. Stable field arguments preserve their variable owner. Deferred sibling
arguments execute once, and owned strings retain their source owner.

Native forwarding reuses one guard per distinct scalar place and passes raw
pointers through the existing C ABI adapter. Pointer grouping compares actual
borrowed cell/shared field ownership, including two reference-record variables
that alias the same field. Character references retain the C char adapter's
existing grouping and conversion. A C-valid duplicate-native-reference probe
initially panicked in Rust; the final regression includes aliased/distinct int
and char arguments and aliased shared fields. That failure receives no credit.

The shared checker carries lambda memory qualifiers into function types. C
indirect calls now cast qualified scalar formals as pointers and pass addresses;
resolved method qualifiers also annotate C arguments. The stable-place validator
recognizes both field expression forms and requires a variable-rooted owner.
Single-file C lambda signatures render the same reference ABI as modular output.

Reference compound mutation reads the prior scalar value before an effectful
RHS, preserving the observed C order. An unchanged C-valid standalone control
prints `4/4` where the previous Rust backend printed `5/5`; GCC and Clang also
agree on the larger reference controls. Two existing generated-code goldens
change only to pin the prior-value read. Their original source bytes are retained
and both targets satisfy authored runtime oracles in all nine modes.

Eight former Rust rejection fixtures are promoted with source bytes unchanged.
Rejection is not parity. The frozen reference gate checks **26** programs against
literal raw-output/status oracles in **234** optimization/arithmetic comparisons.
Linux ASAN/UBSAN/leak checks cover **78** executions. All **67** final acceptance
commands pass, including the complete C/Rust suites
and exact count-checked **156** native fixtures. Frozen evidence verifies
**55 reports / 4647 cases**, all prior **54 / 4413** included. Source generation
passes **499** positives and **123** negatives with no failures/skips. Exact
revision Linux/macOS/Windows hosted acceptance remains required.

The complete goal still requires structural interface storage/native ABI and
platform-dependent empty identity, physical value-record receivers, remaining
qualified signatures and parameter rebinding, and broader native/global/callback/
escaping/thread ownership and evaluation compositions. These unverified contexts
receive no completion credit. C remains the default target. Original corpus
coverage remains **1363 / 1365 (99.85%)**, a corpus measure rather than full-goal
completion. All 1365 corpus source hashes remain unchanged.

Evidence: [validation](rust-parity-evidence/closure-reference-validation.json),
[oracles](rust-parity-evidence/closure-reference-oracles.json),
[comparisons](rust-parity-evidence/closure-reference-after.json),
[C lifetimes](rust-parity-evidence/closure-reference-c-lifetimes.json),
[source promotion](rust-parity-evidence/closure-reference-promotion.json).

The prerequisite `fde9e5e6061dc57a4707182b3dbe28d22c628170` has all six CI jobs
and **54 reports / 4413 cases** verified on each platform.
[Qualified-array hosted proof](rust-parity-evidence/qualified-array-main-ci-green.json).
Its prerequisite `3f3610d25167732e9563c6d215326d757c8b670f` also has six green
jobs and **53 / 4350** independently verified on each platform.
[Captured-method hosted proof](rust-parity-evidence/captured-method-main-ci-green.json).

## Qualified closure array parameters, 2026-10-06

Rust now accepts array closure parameters declared `as val` and creates an
independent array owner at each invocation before executing the body. Default
array parameters retain caller-visible mutation. Two as-val formals supplied
with the same array receive independent copies. Nested arrays, returned arrays,
escaping captures and string elements use existing owned copy/cleanup rules.
Function signature compatibility now includes parameter memory qualifiers;
generated local owner names avoid all user identifiers.

Seven frozen controls pass **63** independently specified C/Rust comparisons
across all nine optimization/arithmetic modes. The unchanged parent C compiler
passes **21** source/optimization controls while Rust rejects those signatures.
Linux ASAN/UBSAN/leak checks pass **21** runs. C remains the default target.

The complete C/Rust suites and all **65** acceptance commands pass with zero
failures/skips. Local evidence verifies **54 reports / 4413 cases**, including
all prior **53 / 4350**. The required CI command passes with **140** native
fixtures; it passes again after correcting the CI display label. Exact-revision
Linux/macOS/Windows hosted acceptance remains required. Original corpus coverage remains **1363 / 1365
(99.85%)**, a corpus measure rather than full-goal completion.

Broader qualified signatures, closure parameter rebinding, record-array member
stores and nested field-array methods, value-record physical receivers,
structural interfaces/native ABI and platform-dependent empty identity remain
required work. A scalar `as ref` control crashes in the parent C backend and
receives no parity credit. The record-array probes are preserved separately;
no rejected or failing control is counted as successful parity.

Evidence: [validation](rust-parity-evidence/qualified-array-validation.json),
[parent baseline](rust-parity-evidence/qualified-array-before.json),
[comparisons](rust-parity-evidence/qualified-array-after.json),
[independent oracles](rust-parity-evidence/qualified-array-oracles.json),
[C lifetimes](rust-parity-evidence/qualified-array-c-lifetimes.json).

## Captured value-record methods and borrowed field printing, 2026-10-06

Methods on captured value records now use the existing mutable owned snapshot
machinery. Scalar changes remain private to each invocation; the capture's
private array state persists across calls. Nested value receivers and independent
owned string/array returns use the same ownership rules. Character/string read
methods on captured fields retain their previous behavior and generated goldens.

C `print`/`println` no longer frees a borrowed string member read through a live
variable/member/index owner. Previously a field print could free the containing
record's string and cause a double free at owner cleanup. Owned receiver/keeper
copies retain their cleanup. This repair changes ownership, not printed bytes.

Five frozen controls pass **45** independent C/Rust comparisons across all nine
optimization/arithmetic modes. Linux ASAN/UBSAN/leak checks pass **15** runs.
Parent C passes **9** unchanged source/optimization controls; the other **6**
parent executions fail their field-string lifetime contract and receive no
before-parity credit. Final successful comparisons require both targets to
compile/run with the independently authored output and zero stderr/status.

Complete C/Rust suites pass without failures/skips, including **133** native
positives and the exact count-checked CI command. Frozen acceptance verifies
**53 reports / 4350 cases**, including all prior **52 / 4305**. Existing C/Rust/model
expectations and all **1365** original source hashes remain unchanged. Original
corpus coverage remains **1363 / 1365 (99.85%)**, not goal completion.
Exact-revision Linux/macOS/Windows hosted acceptance remains required.

The prerequisite `3e45fe317e5d03f3cefca9b285e4ac4fd9f8ebaa` has all six CI jobs
green and **52 reports / 4305 cases** independently verified on each platform.
[Hosted proof](rust-parity-evidence/reference-receiver-main-ci-green.json).

Value-record receiver/array physical overlap, structural interface storage/native
ABI and empty identity, qualified closure signatures, rebinding, named array
adapters and broader mutation/lifetime/evaluation combinations remain work. The
separately preserved captured string-reassignment diagnostic is not counted as
verified parity. C remains the default target.

Evidence: [validation](rust-parity-evidence/captured-method-validation.json),
[parent baseline](rust-parity-evidence/captured-method-before.json),
[repaired comparisons](rust-parity-evidence/captured-method-after.json),
[independent oracles](rust-parity-evidence/captured-method-oracles.json),
[C lifetimes](rust-parity-evidence/captured-method-c-lifetimes.json).

## Reference-record receiver and native closure ownership, 2026-10-06

Indexed reference-record method receivers now borrow the selected handle under
a short array read guard, then release that guard before entering the method.
Reversal/removal does not redirect the receiver to a different heap object, and
no additional reference credit is introduced. Value-record physical receivers
remain separate required work; their live removed slots cannot be replaced by a
snapshot or a fresh bounds-checked index lookup.

Native reference objects and arrays are admitted in default closure signatures.
Object parameters borrow their owners; native array reads borrow the original C
header. Copied/returned arrays and escaping captures retain independent owners.
Native methods mark array arguments/parameters as borrowed headers, lifted
constructor temporaries retain those annotations, and nested reads extract the
canonical pointer under a short guard after evaluating cached indices. Method
argument bindings are processed by the same native-read lowering.

Five frozen controls pass **45** independent C/Rust comparisons in all nine
optimization/arithmetic modes, including exact native reference counts, no live
object leaks, shared header identity, original reference-object identity through
reversal/removal, nested arrays, copied/returned arrays, escaping snapshots and
constructor temporary lifetimes. The unchanged parent C backend passes all
**15** source/optimization controls. Linux ASAN/UBSAN/leak checks pass **15** runs.

Complete C/Rust suites pass without failures or skips, including **128** native
positives. Final local acceptance verifies **52 reports / 4305 cases**, all prior
**51 / 4260** included. The exact count-checked CI command is part of the local
acceptance matrix, and CI requires **128** native fixtures. A whitespace-only
expression wrapper regression was caught by unchanged generated-source goldens;
correct EOF trimming restores those goldens without changing expectations.
Exact-revision Linux/macOS/Windows hosted acceptance remains required.

All **1365** original source hashes remain unchanged; original corpus coverage
remains **1363 / 1365 (99.85%)**, rather than full-goal completion. Structural
interface storage/native ABI and platform-dependent empty storage, value-record
receiver/array overlap, qualified closure signatures, parameter rebinding, named
array-function adapters, captured value-record methods and wider lifetime/effect
compositions remain work. C remains the default target; no C production code
changes are included.

Evidence: [validation](rust-parity-evidence/reference-receiver-validation.json),
[parent baseline](rust-parity-evidence/reference-receiver-before.json),
[repaired comparisons](rust-parity-evidence/reference-receiver-after.json),
[independent oracles](rust-parity-evidence/reference-receiver-oracles.json),
[C lifetimes](rust-parity-evidence/reference-receiver-c-lifetimes.json).

The prerequisite CI repair `3e6b0ea5ed267a812980bdff57219e163450884b` is fully
verified: all six compiler/runtime jobs pass, and every platform independently
verifies **51 reports / 4260 cases**. Earlier `552de7ac`/`67854e22` runtime jobs
had stale fixture-count guards; their failed jobs are not green acceptance.
[Repair proof](rust-parity-evidence/native-count-repair-proof.json),
[hosted acceptance](rust-parity-evidence/native-count-repair-main-ci-green.json).

## Dynamic array aliases through instance/static methods, 2026-10-06

Instance and static methods now share the runtime alias-partition dispatch used
by named free functions. Shared array arguments are classified before mutable
projections, including calls with static duplicate arguments and different
array element types. Scalar argument callbacks finish before array guards.
Stored record receivers retain their original array storage; argument bindings
keep shared cells until the chosen partition forms its exclusive references.
A bound call result ends argument borrow temporaries before local cells leave
scope. Named `as val` parameters keep independent entry-time snapshots.

Eight frozen controls pass **72** independent C/Rust comparisons across all
nine optimization/arithmetic modes. They include all **15** four-formal
partitions for both recursive method families, persistent reference/stored
receiver mutations, scalar effects, mixed integer/string arrays, static
duplicates and independent value-parameter copies. All eight unchanged sources
pass parent C in **24** source/optimization runs; all 24 parent Rust executions
fail. Linux ASAN/UBSAN/leak checks pass **24** runs.

Complete C/Rust suites pass without failures or skips, including **123** native
positives. Frozen local acceptance verifies **51 reports / 4260 cases**, including
all prior **50 / 4188**. Exact-revision Linux/macOS/Windows hosted acceptance
remains required. All **1365** original sources remain unchanged; original
corpus coverage remains **1363 / 1365 (99.85%)**, which is not goal completion.

Structural interface storage/native ABI, qualified closure signatures, parameter
rebinding, named array-function adapters, snapshot value-record method captures
and broader receiver/evaluation/ownership compositions remain work. A separately
preserved computed-receiver probe fails C compilation because its closure
parameter is hoisted out of scope; it receives no parity credit. The snapshot
value-record capture probe passes C and is still rejected by Rust. C remains
the default target, and this increment changes no C production code.

Evidence: [validation](rust-parity-evidence/method-alias-validation.json),
[parent baseline](rust-parity-evidence/method-alias-before.json),
[repaired comparisons](rust-parity-evidence/method-alias-after.json),
[independent oracles](rust-parity-evidence/method-alias-oracles.json),
[C lifetimes](rust-parity-evidence/method-alias-c-lifetimes.json).

## Closure arrays forwarded through dynamic alias partitions, 2026-10-06

Named free-function calls now classify all shared closure-array arguments by
runtime cell identity before forming mutable references. Each identity partition
uses the existing coalesced specialization, preserving caller-visible mutations
without unsafe overlapping references or semantic array copies. Static duplicate
arguments can coexist with dynamically aliased formals. Different array element
types are classified independently; scalar argument effects occur once before
the final borrow. Ordinary named `as val` parameters retain entry-time independent
snapshots even when neighboring arguments alias the same caller array.

The eight frozen controls cover every **5 / 15 / 52** identity partition for
three/four/five arguments, recursive forwarding, mixed integer/string arrays,
static duplicates, argument effects and private value-parameter copies. The new
all-platform gate checks **72** independent C/Rust comparisons across all nine
optimization/arithmetic modes. Parent C baseline and Linux ASAN/UBSAN/leak
checks each cover **24** source/optimization runs. Temporary helper allocation
resumes after its last reserved suffix while continuing to check each new name
against the entire model; this removes the repeated occupied-name scans exposed
by the five-parameter control. Existing generated-source goldens remain unchanged.

Final local acceptance covers the complete C/Rust suites, **115** native positives,
and **50 reports / 4188 cases**, without failures or skips. All **1365** original
source hashes remain unchanged; original corpus coverage remains **1363 / 1365
(99.85%)**, which is corpus coverage rather than full-goal completion.
Exact-revision Linux/macOS/Windows hosted acceptance remains required.

Broader method receiver alias/ownership compositions, qualified closure
signatures, parameter rebinding, named function array adapters, additional
ownership/evaluation compositions and structural interface storage/native ABI
remain required work. A separately preserved mutable snapshot-capture rejection
and its false authored C assertions are excluded from parity credit. C remains
the default target; this increment changes no C production code.

Evidence: [validation](rust-parity-evidence/array-alias-partition-validation.json),
[parent baseline](rust-parity-evidence/array-alias-partition-before.json),
[repaired comparisons](rust-parity-evidence/array-alias-partition-after.json),
[independent oracles](rust-parity-evidence/array-alias-partition-oracles.json),
[C lifetimes](rust-parity-evidence/array-alias-partition-c-lifetimes.json).

## Closure array mutation methods, 2026-10-06

Closure default-array parameters and private captured snapshots now support
reverse, clear, insert and remove through the same short-lived cell guards used
by push/pop. Arguments and raw nested indices are evaluated before taking the
array guard; resolved bounds are cached before projecting the mutable row.
Nested operations mutate the stored row rather than a cloned read.

Eight frozen controls pass **72** independent C/Rust output comparisons in all
nine optimization/arithmetic modes. All eight sources pass the unchanged parent
C backend in **24** source/optimization runs. Linux ASAN/UBSAN/leak checks pass
**24** runs, covering strings, alias visibility, private snapshots, nested rows,
callback-bearing indices/arguments and typed floating-variable insertion.
The separately preserved integer-to-float insertion probe fails the shared
frontend; a literal float probe with an incorrect assertion is not counted as
an independent pass. No expected runtime output was weakened to accept it.

Complete C/Rust suites pass without failures or skips, including **107** native
positives. Frozen local acceptance verifies **49 reports / 4116 cases**, including
all prior **48 / 4044**. CI requires the new all-platform gate and Linux sanitizer
report. All six exact-revision compiler/runtime CI jobs pass on Linux, macOS
and Windows at `4317f250065c5cceb55f51c38303cb7360cd56e2`. Each hosted
platform independently verifies all **49 reports / 4116 cases**.
[Hosted proof](rust-parity-evidence/array-method-main-ci-green.json).

All **1365** original source hashes remain unchanged. This increment adds
composition coverage rather than closing another original corpus failure.
Interface storage/native ABI, qualified closure signatures, parameter rebinding,
named function array adapters, dynamic aliases through methods/static calls and the other
full-goal ownership/evaluation requirements remain work.

Evidence: [validation](rust-parity-evidence/array-method-validation.json),
[parent baseline](rust-parity-evidence/array-method-before.json),
[repaired comparisons](rust-parity-evidence/array-method-after.json),
[independent oracles](rust-parity-evidence/array-method-oracles.json),
[C lifetimes](rust-parity-evidence/array-method-c-lifetimes.json).

## Shared closure array parameters, 2026-10-06

Default array parameters now carry shared access to the caller's array instead
of a caller-side Vec snapshot. Repeated arguments share the same cell; selected
rows use one cached index and safe disjoint element borrows. Temporary arrays
have a scope owner. Escaping captures and returned arrays keep independent owned
snapshots. Indexed record stores retain their array guard, and record methods
mutate the stored receiver after argument callbacks finish. Named-function
forwarding covers static aliases and two dynamically aliased array formals.

The unchanged original exploratory array-mutation program and twenty controls
pass **189** independent C/Rust output comparisons in all nine modes. All controls
also pass the unchanged parent C backend in **63** source/optimization runs.
Linux ASAN/UBSAN/leak checks pass **63** runs. A generated-owner name collision was
reproduced and repaired with model-wide unique names. Only two Rust golden lines
change for the byte encoder's array parameter representation; its source and
runtime oracle remain unchanged.

Both complete suites pass without failures or skips, including **99** native
positives. Frozen local acceptance verifies **48 reports / 4044 cases**, including
the prior **47 / 3855**. The new gate runs on all three CI platforms, with Linux
sanitizers mandatory. All six exact-revision jobs passed for c8315346;
all three platform artifacts independently verify **48 / 4044**, including the
189 closure comparisons and Linux's 63 sanitizer runs.
[Hosted proof](rust-parity-evidence/closure-array-main-ci-green.json).

All **1365** original source hashes remain unchanged. Local original coverage
is **1363 / 1365 (99.85%)**; this is corpus coverage, not full-goal completion.
Both original interface programs remain. Broader array methods, qualified
signatures, parameter rebinding, named function array adapters, three-or-more
forwarded dynamic aliases, additional evaluation/ownership compositions and
native interface ABI support remain required work.

Evidence: [validation](rust-parity-evidence/closure-array-validation.json),
[parent baseline](rust-parity-evidence/closure-array-before.json),
[repaired comparisons](rust-parity-evidence/closure-array-after.json),
[independent oracles](rust-parity-evidence/closure-array-oracles.json),
[C lifetimes](rust-parity-evidence/closure-array-c-lifetimes.json),
[name collision](rust-parity-evidence/closure-array-hygiene-before.json).

## Borrowed array return ownership, 2026-10-06

Returning an array parameter now creates an independent owned result in C.
Lambda body generation saves and restores each lambda's borrowed parameter
context, including nested lambdas; owned local/as-val return transfers retain
their existing ownership path. Borrowed array expressions in expression-body
lambdas copy at the return boundary. Rust also copies ordinary borrowed array
parameters rather than emitting a mutable reference as an owned result.

Three controls cover integer/string elements, named functions,
statement/expression lambdas, nested arrays and parameter scopes, escaping
callables, owned local transfers and empty arrays. Before repair, all nine C
source/optimization runs fail ASAN with use-after-free; Rust also fails to compile
the named borrowed-parameter returns. These are repaired defects, with no
pre-existing C-positive coverage claimed. The fixed compiler passes all **27**
independent raw-output comparisons in O0/O1/O2 and default/checked/unchecked
arithmetic, plus **9** Linux ASAN/UBSAN/leak executions.

Both complete suites pass with zero failures/skips, including **79** native
positives. Frozen local acceptance verifies **47 reports / 3855 cases**, including
all prior **46 / 3828**. The new gate runs on all three CI platforms, with Linux
sanitizers mandatory. All six exact-revision jobs passed for 487bae5a;
all three platform artifacts verify **47 / 3855** and Linux verifies the nine
return-lifetime sanitizer cases.
[Hosted proof](rust-parity-evidence/array-return-main-ci-green.json).

All **1365** original source hashes remain unchanged. Original corpus coverage
remains **1362 / 1365 (99.78%)** on the verified parent; this repair adds regression
controls rather than closing an original Rust failure. General closure array
mutation/alias/qualifier composition, the two interface programs, native
interface representation and the other full-goal requirements remain work.

Evidence: [validation](rust-parity-evidence/array-return-validation.json),
[baseline](rust-parity-evidence/array-return-before.json),
[repaired comparisons](rust-parity-evidence/array-return-after.json),
[frozen oracles](rust-parity-evidence/array-return-oracles.json),
[before-fix sanitizers](rust-parity-evidence/array-return-c-lifetimes-before.json),
[repaired lifetimes](rust-parity-evidence/array-return-c-lifetimes.json).

## Managed iterator protocol, 2026-10-05

Rust now accepts supported managed element, iterator and iterable types through
its ordinary record and scope lowering. Strings, arrays, value records and
reference records are covered, including generic maps, nested loops, empty
inputs, early return, break/continue, element mutation and receiver mutation.
The two original iterator programs are unchanged, as are all **1365** original
corpus source hashes.

Ownership testing exposed three C defects: reference iterator objects were not
released, temporary collections had no scope owner, and owned arrays used only
for `.length` leaked. C now destroys owned iterator/collection temporaries in
reverse declaration order and releases an owned array after reading its length.
Borrowed collections retain their original storage and caller-visible receiver
mutation. Returned loop elements remain alive after the iterator is destroyed.

The original seven controls pass all **63** cases under the unchanged C baseline;
Rust rejected all 63. The additional value-temporary control exposed a C address
of rvalue compilation bug, so its nine repaired cases are not claimed as
pre-existing C-positive coverage. All **72** frozen output comparisons now pass
under GCC and Clang at O0/O1/O2 in default/checked/unchecked arithmetic. Linux
ASAN/UBSAN/leak checks pass all **24** source/optimization cases; the recorded
before-fix leak includes reference iterators on normal and early exits.

Both complete local suites pass with zero failures/skips, including **76** native
positives. Two former iterator rejections are preserved as positive fixtures.
The frozen compiler passes **46 reports / 3828 cases**, including independent
reverification of the prior **45 / 3756**. CI requires the new all-platform gate,
the native fixture count, and the Linux sanitizer report.

Original corpus coverage is **1362 / 1365 (99.78%)** on integrated main. All six
exact-revision jobs passed for 7b5b02e69a04ef5d41ec271bcfa99df1226e217e;
Linux, macOS and Windows artifacts independently verify **46 / 3828**, and
Linux verifies **24** sanitizer cases. Windows runner acquisition failures were
recovered by retrying the same revision after the runtime job completed.
[Hosted proof](rust-parity-evidence/managed-iterators-main-ci-green.json). The remaining original failures are both interface programs and array
mutation through closure parameters; foreign interfaces, qualifier/ownership
composition and other full-goal requirements remain implementation work.

Evidence: [validation](rust-parity-evidence/managed-iterators-validation.json),
[unchanged C baseline](rust-parity-evidence/managed-iterators-before.json),
[temporary baseline](rust-parity-evidence/managed-iterators-temporary-before.json),
[frozen output contracts](rust-parity-evidence/managed-iterators-oracles.json),
[sanitized C lifetimes](rust-parity-evidence/managed-iterators-c-lifetimes.json),
[before-fix leaks](rust-parity-evidence/managed-iterators-reference-leaks-before.json).

## Interface CI recovery, 2026-10-05

The interface increment `b7e0686c` and evidence-retention follow-up `e3b99919`
failed macOS runtime CI. Clang gives two zero-byte empty locals the same address;
Rust gave them distinct identities. Four related fixtures disagree at all nine
optimization/arithmetic combinations. Linux acceptance does not prove these
platform-dependent storage relationships.

The implementation and tests remain preserved in `b7e0686c`. Main restores the
exact compiler, templates and acceptance controls of fully hosted-verified
`2602b3c5` while the empty-storage contract is repaired. Full local C/Rust suites
pass without failures/skips, including 70 native positives and all 27 frozen
zero-record comparisons. Recovery revision `5a58cd52` has all six compiler/runtime jobs green, with all
three 45-report/3756-case platform artifacts independently verified
([hosted proof](rust-parity-evidence/interface-recovery-main-ci-green.json)).
The accepted original corpus count returns to **1360 / 1365 (99.63%)**; interface
prototype results are not counted as accepted progress. The full goal remains
unchanged, including interfaces, foreign ABI, iterators and closure ownership.

Evidence: [recovery validation](rust-parity-evidence/interface-ci-recovery.json).

## Zero-initialized value records, 2026-10-05

This independent increment starts from verified main `afdcddef`. Rust now lowers
uninitialized value-record bindings to their C zero values before nullable and
ownership preparation. Nested records contain zero scalars and nil managed
fields; explicit record literals continue to apply source field defaults.
Controls cover integer/float/boolean/character fields, nested strings, arrays
and references, independent copies, returns, array transport, escaping closures
and repeated scope exits. Native record storage retains its existing lowering.

All **27** unchanged baseline cases compile and produce exact expected output
under C; Rust originally rejects all 27. After the fix, all 27 comparisons pass
at O0/O1/O2 in default, checked and unchecked arithmetic modes. Both complete
local suites pass with zero failures/skips, including **70** native positives.
All **45 reports / 3756 cases** pass on the same frozen compiler, with the prior
**44 / 3729** independently reverified. The mandatory platform gate includes the
new comparisons; its native fixture guard and label both require 70.

The **1365** original source hashes are unchanged. Corpus coverage remains
**1360 / 1365 (99.63%)**, with the same five interface/iterator/closure-array gaps.
This increment is a prerequisite for interface work; full interface ownership,
metadata cleanup and foreign boundaries still require implementation and proof.
Exact-revision Linux/macOS/Windows acceptance is recorded by the mandatory CI
workflows after publication.

Evidence: [validation](rust-parity-evidence/zero-record-defaults-validation.json),
[unchanged baseline](rust-parity-evidence/zero-record-defaults-before.json),
[raw-output contracts](rust-parity-evidence/zero-record-defaults-oracles.json).

## Array value parameters: published and hosted verified, 2026-10-05

This increment starts from published main
`6d608821834aacc714774133fe219b088fd11d54`. All six compiler/runtime jobs,
all three **43-report/3684-case** platform artifacts and hosted suite counts
are independently verified:
[hosted proof](rust-parity-evidence/pointer-slices-main-ci-green.json).
The serialization parent `b5efa4c7` is also fully verified:
[hosted proof](rust-parity-evidence/serialization-main-ci-green.json).
The implementation is published at `f40dd12af209c1c86d974c09b44d0f97b2f98421`.
Runtime CI initially failed on all three platforms because its native-positive
guard still required 64 fixtures after this increment added three. The correction
is published at `afdcddeff0e1aa06bac3738bd68b1b3535fc0a01`. All six exact-revision
compiler/runtime jobs pass, and all three **44-report / 3729-case** platform
artifacts and suite counts are independently verified:
[hosted proof](rust-parity-evidence/array-values-main-ci-green.json).

Ordinary function and instance/static method `as val` array parameters now own
independent entry copies. Duplicate value inputs remain independent, while mixed
default/value inputs preserve caller-visible mutation through the default input.
Controls cover strings, nested arrays, owned value records, reference records,
function elements, nil/empty identity, returns, forwarding, fields, joined
workers, escaping snapshot captures and argument timing before entry copying.
Methods participate in thread-owner transport and copied parameters are not
rewritten as receiver fields. Nested push retains the defined C value-before-index
order. The C backend's original parameter-sharing bug is repaired to the
[documented contract](arrays.md); historical wrong output is not the oracle.
C closure-array retain/release and owned comparison cleanup are prerequisites.

Complete C/Rust suites pass with zero failures/skips, including **67** native
positives and the existing **four** admission-error fixtures. All **44 reports /
3729 cases** pass, including independently reverified previous **43 / 3684**.
The mandatory platform gate adds **45 frozen comparisons**. Its unchanged
baseline has **45 C compile successes**, **18 successful C executions**, **27 C
signal failures**, and **36 Rust compile failures**; only the **nine** original
closure-array pairs satisfy the new contract. The original array-value C runs
successfully but mutates its caller contrary to the specified copy semantics.

All **90 sanitizer-configured executions** satisfy independent raw-output
oracles. An actual command/object audit confirms ASAN/UBSAN instrumentation in
**45 generated C main objects**. Their prebuilt runtime archive is recorded
without claiming instrumentation; all **45 Rust cases** are pure Rust and are
not sanitizer-instrumented. Raw output/argv, native order, diagnostics and
Windows text helper logic also pass.

All **1365 original source hashes** remain unchanged. Original coverage is
**1137 integration + 223 exploratory = 1360 / 1365 (99.63%)**, with **five
compilation gaps**, zero runtime failures and zero skips. The newer integration
control is separate. This percentage measures original corpus coverage, not full
goal completion. Interfaces/iterators, mutation of closure array parameters,
qualified closure/capture compositions, broader effectful/indexed aliases,
heap-owning record reference parameters, SDK/native arrays and foreign callbacks,
native disposal/global/thread lifetimes and exact hosted acceptance remain required.

Evidence: [implementation and boundaries](rust-array-values.md),
[validation and remaining gaps](rust-parity-evidence/array-values-validation.json),
[45 frozen comparisons](rust-parity-evidence/array-values-oracles.json),
[previous 3684-case preservation](rust-parity-evidence/array-values-preservation.json),
[90 sanitizer-configured executions](rust-parity-evidence/array-values-asan.json),
[actual C object audit](rust-parity-evidence/array-values-asan-object-audit.json),
[unchanged original sources](rust-parity-evidence/array-values-source-preservation.json).

## Ordinary byte-pointer slices: local acceptance, 2026-10-05

This increment starts from published main
`b5efa4c73e41471fa7dab4783d933ad3260286a8`. All three compiler jobs pass and
the Linux 42-report/3639-case runtime artifact is independently verified.
macOS and Windows runtime jobs remain active. The preceding `8314a270` now has
all six jobs, all three 41-report/3522-case artifacts and hosted suite counts
independently verified:
[hosted proof](rust-parity-evidence/native-variadics-main-ci-green.json).
Publication and exact-revision hosted acceptance of this increment remain required.

Rust now evaluates ordinary byte-pointer slices and no-op `valueOf`, matching
the C runtime's byte offsets, owned copies, nil zero-fill, reversed/empty ranges
and negative offsets inside valid allocations. Results survive native source
mutation and disposal. A typed C adapter preserves actual char bound promotion.
Controls cover numeric bounds, source/private-name collisions, nullable array
identity, globals, record fields, ordinary returns and joined workers. Each
operand is evaluated once. C leaves independent argument order unspecified;
these controls do not claim a portable C evaluation order.

Complete C/Rust suites pass with zero failures/skips, including **64** native
positives and the existing **four** admission-error fixtures. All **43 reports /
3684 cases** pass, including independently reverified previous **42 / 3639**.
The new mandatory platform gate covers **45 frozen comparisons**. Its final
baseline has **45 C successes and 45 Rust compilation failures**. All **90
instrumented target executions** match frozen raw output with zero statuses and
empty sanitizer stderr. Actual **117 C object compiles** include ASAN/UBSAN;
all objects have ASAN sites, with applicable UBSAN sites in **81**. Every Rust
execution is tied to its own generated C objects or recorded as pure Rust. Rust
pointer-copy instrumentation is not claimed. An additional **18 comparisons**
pass with explicitly signed and unsigned C char. Raw bytes/argv, native output
order, diagnostics and Windows helper logic also pass.

All **1365 original source hashes** remain unchanged. Original coverage is
**1137 integration + 222 exploratory = 1359 / 1365 (99.56%)**, with **six
compilation gaps**, zero runtime failures and zero skips. The newer integration
control is counted separately. This percentage measures original corpus
coverage, not full completion. Interfaces/iterators, array parameter/capture/
copy semantics, broader scalar/managed pointer storage, foreign callbacks,
SDK record/array transport, native disposal/alias ownership, global/thread
lifetimes and exact integrated hosted acceptance remain required. Separate
C-valid char/bool/int/float/double pointer-slice probes still reject in Rust;
they receive no parity credit.

Evidence: [implementation](rust-pointer-slices.md),
[validation and remaining gaps](rust-parity-evidence/pointer-slices-validation.json),
[45 frozen comparisons](rust-parity-evidence/pointer-slices-oracles.json),
[previous 3639-case preservation](rust-parity-evidence/pointer-slices-preservation.json),
[90 instrumented executions](rust-parity-evidence/pointer-slices-asan.json),
[actual compiler/object audit](rust-parity-evidence/pointer-slices-asan-object-audit.json),
[unchanged original sources](rust-parity-evidence/pointer-slices-source-preservation.json),
[remaining pointer storage probes](rust-parity-evidence/pointer-slices-storage-gaps.json).

## Serialization vtables and generated methods, 2026-10-05

This increment builds on published main `8314a2706f3713ff765d7b27c5d49ebb4633b972`.
Its compiler jobs are green on all three platforms, and Linux/macOS runtime
41-report/3522-case artifacts are independently verified. Windows runtime remains
active. The preceding `8d29dd45` now has all six jobs, all three 40-report/3468-case
artifacts and full-suite counts independently verified:
[hosted proof](rust-parity-evidence/native-callables-main-ci-green.json).
Resulting exact-revision hosted acceptance remains required.

Rust now emits `@serializable` encode/decode/array traversal and calls concrete C
Encoder/Decoder vtables through typed shims. Opaque serialization objects share
one Rust owner for the actual C allocation; known children retain their parents,
Encoder end invalidates the child owner, and result uses the C finalization cache.
Generated field reads preserve order and aliases. Decoder locals avoid source
field collisions. Threaded record storage is wrapped/read correctly, and mixed
native arrays do not invent Encoder/Decoder C retain callbacks. No Send claim or
general serialized-record wire-ABI admission is made.

All complete C/Rust suites pass with zero failures/skips, including **62** native
positives and the existing **four** admission-error fixtures. All **42 reports /
3639 cases** pass; previous **41 / 3522** are independently verified with this
compiler. The new **117-case** gate is mandatory on Linux/macOS/Windows. Thirteen
unchanged sources cover the eight original serialization/threaded-return programs
and five direct vtable, name/array, cleanup/nil/return, mixed-native-array and
joined-record-thread controls. All **234 instrumented executions** match frozen
oracles, return zero and have empty sanitizer stderr. Actual **603 C object
compiles** include ASAN/UBSAN, with ASAN sites in all objects and applicable UBSAN
sites in **585**. Every Rust case is associated with its exact C objects; Rust
memory instrumentation is not claimed.

The final-source baseline has **117 C successes and 117 Rust compile failures**.
All **1365 original source hashes** are unchanged. Original coverage is **1135
integration + 221 exploratory = 1356 / 1365 (99.34%)**, with **nine compilation
gaps**, zero runtime failures and zero skips. The newer integration control is
counted separately. Raw bytes/argv, native output order, diagnostics and Windows
helper logic also pass. This percentage measures the original corpus, not full
completion. Interfaces/iterators, pointer slices, remaining array semantics,
foreign callbacks/SDK/native ownership, opaque-handle and global/thread lifetime
composition and exact hosted acceptance remain required.

Evidence: [serialization approach](rust-serialization.md),
[validation and remaining gaps](rust-parity-evidence/serialization-validation.json),
[117 frozen comparisons](rust-parity-evidence/serialization-oracles.json),
[234 instrumented executions](rust-parity-evidence/serialization-asan.json),
[actual compiler/object audit](rust-parity-evidence/serialization-asan-object-audit.json),
[final-source baseline](rust-parity-evidence/serialization-before.json),
[full C suite](rust-parity-evidence/serialization-full-c.log),
[full Rust suite](rust-parity-evidence/serialization-full-rust.log).

## Native variadic adapters: local acceptance, 2026-10-05

This increment starts from published main
`8d29dd4597332146bbdcd290f22fd0a33a4d48d2`. All compiler jobs and the Linux/macOS
runtime artifacts for that revision pass; Windows runtime remains active.
The preceding `ea1098d2` now has all six exact-revision CI jobs, all three
39-report/3405-case artifacts and hosted full-suite counts independently verified:
[hosted proof](rust-parity-evidence/native-managed-records-main-ci-green.json).
Publication and hosted acceptance of this increment remain required.

Resolved direct native variadic calls now use hygienic, deduplicated C adapters
with fixed signatures. Rust sends typed arguments; C performs the actual
variadic call and target argument promotions. Native C bodies keep their original
calls. Existing native ABI wrappers preserve fixed parameter qualifications,
reference aliasing, managed results and record storage. An owned `as val` record
is transferred once: the adapter does not repeat the real callee's cleanup.

Two prerequisite repairs also retain language behavior. Generated C variadic
body definitions now include the ellipsis already present in their declarations.
Private C initializers and callable definitions are linked even when no public
Rust ABI wrapper is required. The unused-native control pins initialization
before source main. These controls failed C compilation on the baseline and are
reported separately from previously C-valid parity successes.

Complete C/Rust suites pass with zero failures/skips; native positives are **57**,
with **four** remaining admission-error fixtures. All **41 reports / 3522 cases**
pass, including independently reverified previous **40 / 3468**. The new
**54-case** gate is mandatory on Linux/macOS/Windows. Controls cover scalar and
pointer tails, actual promotions (including float rounding and high-bit char),
imported aliases, globals, closures, shared C/Rust helpers, joined threads,
duplicate scalar/char/record references, managed string/byte-array results and
owned record transfers. All **108 instrumented target executions** pass frozen
output oracles with zero statuses and empty sanitizer stderr. All 225 actual C
compilation commands include ASAN/UBSAN; all objects contain ASAN sites, with
158 containing applicable UBSAN sites. Each Rust run is tied to its exact C
objects. Rust memory instrumentation is not claimed. Additional **18** promotion
comparisons pass with explicitly signed and unsigned C char.

The exact final baseline has **36 C successes**, **18 C compilation failures**
from the variadic-body prerequisite, and **54 Rust compilation failures**.
All **1365 original source hashes** remain unchanged. Original coverage is
**1127 integration + 221 exploratory = 1348 / 1365 (98.8%)**, with **17
compilation gaps**, zero runtime failures and zero skips. The newer integration
control is counted separately. The unchanged comprehensive interop original is
the newly passing source. Raw byte/argv, native print/flush ordering, diagnostics
and Windows helper logic also pass.

Full completion still requires every remaining original gap, interfaces and
iterators, serialization and broader SDK/native ownership families, callbacks
crossing Rust/C boundaries, indirect variadic callable values, array and global/
thread lifetime edges, exact integrated platform acceptance and a full completion
audit. This percentage measures the original corpus, not the full goal.

Evidence: [validation and remaining gaps](rust-parity-evidence/native-variadics-validation.json),
[54 frozen comparisons](rust-parity-evidence/native-variadics-oracles.json),
[previous 3468-case preservation](rust-parity-evidence/native-variadics-preservation.json),
[108 instrumented executions](rust-parity-evidence/native-variadics-asan.json),
[actual compiler/object audit](rust-parity-evidence/native-variadics-asan-object-audit.json),
[final-source baseline](rust-parity-evidence/native-variadics-before.json),
[full C suite](rust-parity-evidence/native-variadics-full-c.log),
[full Rust suite](rust-parity-evidence/native-variadics-full-rust.log).

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
