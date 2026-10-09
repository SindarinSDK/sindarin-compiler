# Rust completion goal

Complete the Sindarin Rust target to verified feature and behavioural parity with
C, using the shared C runtime and mixed-language package architecture defined in
`docs/runtime-target-architecture.md`. Preserve the existing language contract,
public SDK API, legacy package behaviour and C as the default target.

## Required outcomes

1. **Define and implement the shared ABI.** Establish a versioned, documented C
   ABI for the shared Sindarin runtime and package exports. Keep the canonical
   runtime implementation in C and make it consumable from generated Rust and
   Go-native dependency bridges. Specify scalar widths, nil, strings/buffers,
   arrays, records/interfaces, allocation, ownership, aliases, callbacks,
   reentry, errors, initialization and resource cleanup. Preserve observable
   language behaviour, including evaluation order, identity and sizeof.

2. **Keep one C-backed SDK implementation.** Decouple the actual SDK package from
   per-application generated C records and constructors sufficiently to build
   reusable package artifacts. Preserve existing SDK imports, types, methods,
   visible fields, layouts where exposed, outputs and lifecycle behaviour.
   Provide compatibility adapters for existing C applications. Do not require
   separate Rust and Go SDK implementation ports.

3. **Build packages independently.** Implement package `runtime: C / RS / GO`
   selection and explicit native build/binding metadata. Pure Sindarin packages
   with no fixed runtime inherit the application target. Legacy manifests and C
   source/include/link directives retain their established behaviour. Artifacts
   carry public declarations, symbols/types, ABI and ownership information,
   runtime/toolchain and platform compatibility, transitive dependencies and
   initialization requirements. Include generic/reflection specialization data
   where needed, caching and useful compatibility diagnostics.

4. **Generate package adapters and link Rust applications.** Generate export and
   import adapters from declared package contracts. Build native backing code
   with its original language's toolchain; do not translate C or Go backing code
   into Rust. Automatically connect compatible package artifacts to the Rust
   application and shared runtime. Reject incomplete or unsupported contracts
   clearly instead of silently selecting a different implementation.

5. **Prove mixed-language consumption.** Build and execute a Rust-targeted
   Sindarin application using all of: the C-backed SDK; a pure Sindarin package
   such as HTTP compiled for Rust; a Rust-backed package; and a Go-native package
   exposed through a C-callable bridge. Exercise representative SDK resources,
   strings/arrays, ownership, aliases, callbacks, errors and cleanup, not only
   scalar hello-world calls. Preserve each backing implementation's language.
   Respect Go bridge/runtime aggregation and platform/toolchain requirements.

6. **Close all remaining Rust parity gaps.** Repair documented C-supported/Rust-
   rejected cases and remaining ownership, aliasing, interfaces, closures,
   concurrency, numeric, match, reflection, copy-hook, native/SDK and language
   gaps. Revalidate historical inventories against current code. Preserve
   original source programs and existing behavioural oracles; add focused
   regressions for repairs. Record C compilation/runtime failures and undefined
   cases separately; do not count matching failures or rejection as parity.

## Go scope for this goal

Include consuming Go-native package implementations from a Rust application
through generated C-ABI adapters and supported Go library build modes. The full
Sindarin-to-Go compiler backend remains a later goal. Until it exists, a GO
package needing Go compilation of Sindarin implementation bodies must produce
an explicit unsupported-target diagnostic; declarations plus native Go backing
can use the Go-native bridge path. Do not advertise a functioning Go target.

## Development workflow

- Resume from current origin/main and preserve previously accepted repairs and
  useful parity work. Implement small, coherent batches with relevant local
  regression, ABI/lifetime and unchanged-source C/Rust checks.
- Commit and push validated batches directly to main; no PR is required.
- Preserve a distinct CI run for each main push. Later pushes must not
  automatically cancel earlier main validation. Superseded revisions of one PR
  may cancel their earlier PR checks.
- Allow at most two independent feature batches awaiting complete required CI.
  Hold dependent changes until their dependency is validated. Continue useful
  local work when both publication slots are occupied.
- Monitor every outstanding CI run. Any required failure freezes feature pushes.
  Diagnose the failing revision and current main, repair the root cause and keep
  a focused regression. Combine known fixes into one correction batch; allow
  only one correction batch awaiting validation. Publish only corrections until
  complete Linux/macOS/Windows CI on corrected integrated main is green.
- Existing required failures take priority when resuming. Run complete local
  suites at meaningful milestones and final acceptance. Keep the completion
  ledger current, distinguishing local validation, pending CI and hosted
  acceptance. Follow repository instructions and standing worker permissions.

## Completion evidence

Finish only when every required outcome above is implemented and verified:

- All 1,365 original corpus programs pass through Rust with preserved source and
  behavioural contracts. C/Rust verification covers supported optimization and
  arithmetic modes and platform acceptance; the earlier Linux/O0 audit alone
  does not prove final parity.
- Every documented parity gap is resolved, with current evidence for outputs,
  exits, evaluation order, mutation, identity, aliases and lifetimes.
- The mixed-language Rust application, independent package artifacts, generated
  adapters, shared runtime and C-backed SDK pass behavioural and lifecycle tests
  on supported Linux/macOS/Windows configurations.
- Complete C/Rust suites and required ABI/lifetime/sanitizer checks pass without
  unexplained failures or skips. Existing C applications and packages retain
  their tested behaviour and default-target compatibility.
- Complete unified CI is green on final integrated main. Runtime/SDK ABI,
  package schema/builds, limitations and usage documentation and the completion
  ledger accurately describe implemented, accepted capabilities.

No Rust-only SDK replacement, blanket rewriting of foreign implementations,
reduced fixture inventory or narrower passing subset satisfies this goal.
