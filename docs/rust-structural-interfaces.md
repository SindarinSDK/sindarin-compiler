# Structural interfaces: implementation in progress

Structural interface parameters preserve C storage identity across value and
reference records, arrays, inline fields, copies, assignments and captures.
The shared front end still checks structural satisfaction; this representation
adds no dynamic dispatch or changes to the language contract.

The original two interface programs and twenty controls pass all 198 C/Rust
comparisons at O0/O1/O2 with default, checked and unchecked arithmetic, using
both GCC and Clang on ARM64 Linux. Of these, 180 comparisons also enforce
independent literal output contracts; eighteen examine compiler-selected
zero-sized storage directly against C. All existing source files and expected
outputs remain unchanged. Three new controls cover mixed scopes, escaping and
nested empty captures, and managed records beside empty storage.

The selected C compiler now owns zero-sized local and parameter storage through
small C scope support functions. Rust executes the source body inside that
scope and retains its cleanup and panic handling. Closure trampolines get
invocation-local capture identities: returning a nested closure does not retain
the creator's expired stack slot. Assignment preserves the destination's slot
while evaluating the right-hand side first.

Four preserved controls observe two distinct zero-sized C locals. Their frozen
GCC oracle is retained; a second explicit contract changes only the seventeenth
comparison when C allocates those locals together. Rust must match the actual
C execution byte for byte in every mode. These four controls run in the required
interface matrix rather than the general fixed-output native suite, which now
contains 377 fixtures. No source or historical oracle is rewritten.

`make test-rust-parity-interfaces` requires the complete 24-source matrix,
frozen source hashes, successful C/Rust execution, exact raw output equality,
and the independent contracts. Twelve further audits require zero provenance
metadata entries after scope exit, array growth, escaping captures and returned
arrays. Both gates are required on Linux, macOS and Windows in unified CI.
The earlier storage increment has hosted acceptance on main `45fc52be`.
The current 24-source extension passes locally; hosted acceptance is pending.

Evidence: [storage and closure validation](rust-parity-evidence/interface-storage-current-validation.json),
[earlier recovery](rust-parity-evidence/interface-recovery-current-validation.json).
Native interface ABI remains unfinished: C can pass a borrowed opaque pointer
into native code for field reads and writes, whereas Rust still rejects that
boundary. Retained native storage, native callbacks and ownership composition
require further implementation and evidence before full backend completion.

For executable compilation use `--target rust`; `SN_CC` selects the C compiler
for scope support. `--emit-rust` reports an error for programs requiring C scope
storage because one Rust source file cannot include the required C unit.
Programs without that support requirement retain ordinary Rust source emission.

## Metadata ownership

Field metadata caches its storage key and refreshes nested relationships after
replacement. Readonly aggregate transport retains its origin, while owning
copies receive independent metadata owners. Provenance edges belong to lifetime
tokens; the final owner removes its edges, including links created before moves
and reallocations. Compiler-private state is excluded from value equality,
reflection and native C record layouts.

## C layouts and identity requirements

Private layout descriptions derive C storage from ordered field types. Reference
record bodies include the generated C reference-count header; reference values
remain pointer-sized. Runtime-defined Encoder/Decoder bodies use their existing
canonical native ABI instead of a synthesized reference-count header.

An ARM64 Linux audit compares generated C headers against emitted Rust layout
descriptions using sizeof, alignment and offsetof. All six probed shapes agree:

| Shape | Body size | Alignment | First source field offset |
| --- | ---: | ---: | ---: |
| Empty value | 0 | 1 | — |
| Value with string | 8 | 8 | 0 |
| Reference with string | 16 | 8 | 8 |
| Value containing another value | 8 | 8 | 0 |
| Reference containing a value | 16 | 8 | 8 |
| Value with integer | 8 | 8 | 0 |

Shared model size/offset metadata cannot substitute for this evidence. For
example, its value-with-string metadata has size 16 and field offset 8, while the
actual generated C type has size 8 and offset 0. The private layouts ignore those
synthetic header offsets. This audit is Linux evidence; other platforms still
require their own acceptance.

The implementation preserves the checked relationships between copies, aliases
and destination slots. Empty source locals use C scope storage; array provenance uses the C element
stride, including zero stride. Record-array
literals reserve C's minimum initial capacity, and tested mutations refresh their
storage relationships.

The ordinary interface increment does not establish full interface interoperability.
The C-valid native-boundary probe passes six independent checks at O0/O1/O2;
Rust still rejects its interface parameters. Native ABI transport must provide
real C-compatible source storage and preserve returns, mutations and callback
lifetimes. Rejection is not parity credit. Effectful receivers, qualified/global
ownership, concurrency and SDK/foreign boundaries remain full-goal work.

Self-to-interface conversion in the attempted method probe is rejected by the
shared front end for both targets; it is not a passing parity case. Named callable initialization works on current main. Reassignment previously
stored a bare C function pointer instead of an owned closure and failed in all
nine modes. Reassignment now constructs the closure and retains borrowed
function owners before releasing the destination, including self-assignment.
The frozen regression also preserves captured strings after the source owner
is cleared. Qualified parameters, escaping/captured identities,
concurrency, native SDK interfaces and foreign callbacks remain in the full goal.

Local records are retained under `.sn/interface-owned-metadata-main-validation.json`,
`.sn/interface-native-boundary-before.json`,
`.sn/interface-c-layout-audit.json`,
`.sn/interface-identity-layout-audit.json` and their referenced reports. Source
admission, complete C/Rust suites, mandatory platform gates, current-main
composition and Linux/macOS/Windows hosted evidence are required for full backend completion.

## Nullable callable transport

Rust callable values can hold nil without a native callback declaration. Nil
acquires the receiving signature at declarations, assignments, returns, function
and method arguments, record fields and array literals. Default local callables
and sized callable arrays start as nil. Ordinary and recursive closures use the
same nullable owner; nested captures retain their callable independently.

The two added regressions require eighteen exact C/Rust comparisons and frozen
literal output contracts, including managed closure replacement and nil transport
through static/instance methods and recursive captures. The complete interface
matrix now requires 216 comparisons over 24 unchanged, hashed sources, with 198
independent output oracles. The closure ownership gate additionally exercises
both new regressions at every optimization level, including C ASAN/UBSAN/leak
checks and Rust ASAN on Linux.

The former Rust-only `closure_values_uninitialized` limitation source now emits
successfully and is checked as a runtime failure: calling its default-null value
traps. Its source bytes are preserved, and its old compiler diagnostic remains
in `docs/restoration/post-tag-fixtures`. This invalid call receives no positive
C/Rust parity credit. Existing generation goldens and positive runtime oracles
are unchanged. Foreign interface ABI and the other full-goal obligations above
remain unfinished.
