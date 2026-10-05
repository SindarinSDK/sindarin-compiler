# Structural interfaces: implementation in progress

This worktree contains an unpublished prototype based on main
`2602b3c5e7860296206826032e5f137a12aa33df`. Full interface parity is not established.
The current published corpus coverage remains the value reported in the main
completion ledger; passing prototype programs do not advance that count.

The shared front end checks structural satisfaction. Interface parameters in C
carry a borrowed opaque pointer; equality observes the identity of source storage.
Rust now transports a private storage descriptor and tracks typed relationships
between storage slots using actual C field offsets. This adds no dynamic dispatch
or shared satisfaction changes. Native interface ABI transport remains required.

Field metadata caches its storage key during construction and refreshes nested
relationships after replacement. Identity lookup can therefore preserve a
borrowed argument without reacquiring its payload lock. Readonly aggregate
transport retains the caller's storage origin; owning copies clear that origin.
Compiler-private origin state is excluded from normal value equality and reflection.

Nineteen sources pass 171 independent raw-output comparisons over O0/O1/O2
and default, checked and unchecked arithmetic. They include the two unchanged
original interface tests, value/reference/capture/temporary controls,
serialization handles, inline parent/field aliases, readonly borrowed parameters,
distinct empty locals, zero-stride empty arrays, retained interface values across
replacement, destination assignment and copying, array copies/slices/growth,
field/captured arrays and uninitialized records. Existing C closure snapshot
semantics are preserved; identity reads borrow the snapshot's storage.

Provenance edges belong to shared lifetime tokens. The final owner removes its
edges, including edges created before moves and reallocations. Owning copies get
new tokens; readonly shared transports retain their token. This state is private
and excluded from source value equality, reflection and native C record layouts.
Twelve mandatory emitted-Rust audits require zero remaining metadata entries
after 1,000 scope exits, array growth, escaping captures and returned arrays, at
all three optimization levels. Complete local C and Rust suites pass with zero
failures/skips. All 1365 original source hashes remain unchanged.

The mandatory gate is `make test-rust-parity-interfaces`; its source hashes and
raw output contracts are checked by `scripts/check_rust_interface_oracles.py`.
Lifetime audits run through `scripts/check_rust_interface_lifetimes.py`. Platform
CI also requires exactly 87 positive native fixtures without skips.

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
and destination slots. Empty source values receive private Rust storage while
array provenance uses the C element stride, including zero stride. Record-array
literals reserve C's minimum initial capacity, and tested mutations refresh their
storage relationships.

The ordinary interface increment does not establish full interface interoperability.
The C-valid native-boundary probe passes six independent checks at O0/O1/O2;
Rust still rejects its interface parameters. Native ABI transport must provide
real C-compatible source storage and preserve returns, mutations and callback
lifetimes. Rejection is not parity credit. Effectful receivers, qualified/global
ownership, concurrency and SDK/foreign boundaries remain full-goal work.

Self-to-interface conversion in the attempted method probe is rejected by the
shared front end for both targets; it is not a passing parity case. The separate
named-function initializer probe exposes a C closure representation failure and
remains repair work. Qualified parameters, escaping/captured identities,
concurrency, native SDK interfaces and foreign callbacks remain in the full goal.

Local records are retained under `.sn/interface-owned-metadata-main-validation.json`,
`.sn/interface-native-boundary-before.json`,
`.sn/interface-c-layout-audit.json`,
`.sn/interface-identity-layout-audit.json` and their referenced reports. Source
admission, complete C/Rust suites, mandatory platform gates, current-main
composition and Linux/macOS/Windows hosted evidence are required before publishing
this implementation as accepted.
