# Generated native package imports

Status: implemented consumer and provider adapters for resolved scalar and string
function and borrowed-input/owned-result string-array contracts. Complete independent Sindarin package bodies, SDK artifact imports,
managed record/interface/callback contracts and non-string array elements,
and wider package graph planning
remain required by the [Rust completion goal](rust-completion-goal.md).

A C- or Rust-targeted Sindarin application can import declaration modules from
packages using the [native manifest](native-package-manifest.md). The compiler
resolves each binding against its native declaration, validates the parameter and
result representation/ownership, builds the original backing language's artifacts,
generates a C ABI adapter and links the archives and shared C runtime automatically.
Application source uses existing Sindarin imports and calls.

Native-only declaration packages can select C, RS or GO independently of the
consumer. A fixed-runtime package containing Sindarin bodies still requires that
runtime's independent package compilation path; unsupported cross-runtime bodies
produce an explicit diagnostic. Ordinary portable Sindarin packages retain target
inheritance. This does not implement a Sindarin Go backend.

## Current contracts

- Plain fixed-width scalar inputs/results use `value` ownership.
- String inputs use `borrowed` ownership; adapters copy into shared C runtime
  values for the call and release those credits afterward.
- String results use `owned` ownership; adapters copy their byte view into the
  caller's established string representation and release the returned C credit.
- `failure: abort` exports use the declared direct wire return. `failure: status`
  exports return a status and place a non-void result in an output parameter.
  A failed status produces the same C diagnostic/exit through both app targets.

Native backing can provide declared C-callable wire exports, or select generated
provider exports with the optional `function` binding field described below. The compiler rejects unsupported types,
reference qualifiers, ownership policies and mismatched declaration/parameter
metadata. Native symbols must be callable C identifiers.

Public function declarations retain their Sindarin identity. Generated import
adapter names are attached to the resolved declaration and symbol table; backing
symbols and toolchains retain their original implementation language. Generated
C adapter headers expose legacy C-compatible call signatures to app code, while
adapter bodies use shared ABI values at the package boundary.

Native-bearing API modules cannot also compile per-application `@source` backing:
that can silently shadow the independent archive. Legacy manifests without native
metadata retain their established C source/include/link behaviour. Complete SDK
facades and wider package migration still need explicit artifact/type planning.

## Generated provider exports

A binding can name an ordinary backing-language function separately from its
exported wire symbol:

```yaml
- declaration: src/api.sn::echo
  build: backing
  symbol: example_echo_v1
  function: echo
  convention: C
  failure: status
  ownership: {parameters: {text: borrowed}, result: owned}
```

The existing Sindarin declaration is `native fn echo(text: str): str`. Both
`--build-native` and normal application imports resolve and type-check that
public declaration; no application is needed to build the package. The backing
function uses these representations:

| Contract | C backing | Rust backing | Go backing |
| --- | --- | --- | --- |
| 64-bit signed integer | `long long` | `i64` | `int64` |
| 64-bit unsigned integer | `uint64_t` | `u64` | `uint64` |
| 32-bit integers | `int32_t` / `uint32_t` | `i32` / `u32` | `int32` / `uint32` |
| byte / char | `unsigned char` / `char` | `u8` | `uint8` |
| boolean | `bool` | `bool` | `bool` |
| float / double | `float` / `double` | `f32` / `f64` | `float32` / `float64` |
| borrowed string input | `char *` | `Option<&[u8]>` | `*string` |
| owned string result | malloc-owned `char *` | `Option<Vec<u8>>` | `*string` |
| borrowed string-array input (ABI 1.1) | read-only `SnArray *` | `Option<&[Option<&[u8]>]>` | `[]*string` |
| owned string-array result (ABI 1.1) | owning `SnArray *` | `Option<Vec<Option<Vec<u8>>>>` | `[]*string` |

Nil uses NULL/None/nil; an empty value remains non-nil. Strings preserve non-UTF-8
bytes and follow existing C string semantics through the first NUL. Borrowed
inputs are read-only during the call and cannot be retained without an independent
copy. Providers copy results into the shared C runtime before releasing backing
storage. C results must use allocation compatible with `free`; generated code
releases them after the copy. Go results are copied synchronously before leaving
the Go call and no Go pointer becomes a retained C value.

For `failure: abort`, an ordinary function returns its declared result directly.
For `failure: status`, C returns `uint32_t` and receives a final result output
pointer; Rust returns `Result<T, u32>`; Go returns `(T, uint32)`. Void status
functions use no C result pointer, `Result<(), u32>` in Rust and only `uint32` in
Go. Zero means success. Outputs are published only after success and remain
unchanged after an invalid input, explicit error or contained panic. Backing code
keeps cleanup responsibility for any provisional result on its failure path.

Rust export shims catch unwinding panics and Go shims recover call-frame panics,
including legacy `panic(nil)`, returning `SN_ABI_FOREIGN_ERROR` (6). Rust's normal
panic hook still runs. Abort-policy calls retain terminal failure; fatal process
termination and panics on unrelated Go goroutines cannot become call statuses.
Status providers reject Rust `panic=abort` flags.

C builds force-include generated backing prototypes into original source units,
so implementation signature mismatches fail compilation. Declared C backing
functions receive package/build-specific link names through the generated header;
ordinary source and calls are preserved while identically named functions in two
packages remain independent. Rust crate identities also include package identity. Rust builds the original
crate as an rlib, preserving its crate-relative modules, then builds a separate
C-callable export crate. Go builds a bridge main module importing the original
library; module requirements and relative replacements are preserved without
editing original go.mod/source files. Both export and consumer adapters are
compiler-generated; backing functions need no handwritten C ABI annotations.

Current provider functions must be public/root-level where the backing language
requires it; C functions must have external linkage. Declared wire symbols must
be unique across imported packages; collisions produce a diagnostic. Complete
automatic wire-symbol allocation and broader native namespace isolation remain
package-planning work. A generated C symbol must differ from its backing function. One build
unit currently selects generated or handwritten exports throughout. Qualified
methods, managed records/arrays/interfaces/callbacks and reference qualifiers need
further ABI work and are diagnosed. Artifact metadata retains resolved provider
signatures and generated export symbols; these remain partial native artifacts,
not complete independent Sindarin/SDK packages.

## Linking and validation

The compiler passes archive paths and discovered native library options to the
C and Rust linker plans. Ordinary library names retain legacy translation and
configuration overrides. The shared runtime resolves the generated value adapters.
Multiple generated-provider Go libraries now build one aggregate bridge/runtime
archive for the application. Each bridge file imports its original library module;
normal Go dependency resolution selects the combined module graph. Shared module
instances and initialization remain under the Go toolchain. Original source and
module manifests are preserved, and artifact metadata records the selected module
manifest, input hashes, exports and runtime/toolchain provenance.

Conflicting replacements or a module path mapped to different source roots are
diagnosed. Legacy handwritten Go main archives remain supported individually;
combining them requires an importable library/provider contract and produces a
clear diagnostic. Go graph artifacts are immutable and currently rebuilt rather
than reused until complete transitive cgo input capture is available. This still
does not implement a Sindarin Go backend.

Run `python3 tests/package/native_imports.py` with the pinned SDK integration
checkout available. Tests build C/Rust/Go native packages, use generated adapters
from real C/Rust Sindarin callers, validate owned/borrowed string lifetime and
error exits, and reject ownership mismatches. A Rust application also uses SDK
TextFile resources/arrays with all three backing languages and a portable Sindarin
request-line module. The SDK call path in that test remains its existing C
compatibility path; it does not prove an independent SDK artifact import.

The unified Linux sanitizer group also runs generated provider ABI clients with
address/undefined sanitizers and leak detection, covering all three backing
languages. It retains the existing platform matrix and C/Rust gates.

Full corpus/mode/platform parity and complete mixed-package SDK acceptance remain
outstanding. Passing these fixtures does not satisfy the entire Rust completion goal.

## Owned string-array results

Native declarations returning `str[]` can use `abi: 1.1` and `result: owned`.
Generated C providers accept an owned `SnArray *` with string element cleanup;
Rust providers return `Option<Vec<Option<Vec<u8>>>>`; Go providers return
`[]*string`. Nil and empty arrays, nil and empty elements and byte-oriented string
semantics remain distinct. Status providers wrap these results in the existing
C output-pointer / Rust Result / Go `(value, status)` protocol.

Provider adapters copy into managed C-runtime values and release backing storage.
Consumer adapters build an owning legacy `SnArray` with string copy/release hooks,
copy element bytes and release runtime credits. C/Rust callers can mutate results
with their existing array syntax. Arrays require ABI 1.1 explicitly; ABI 1.0
contracts are rejected. Non-string/record elements, language copy hooks and
complete SDK record/method adapters remain required work.

String-array inputs use `parameters: {name: borrowed}` and ABI 1.1. Adapters
preserve nil/empty arrays and elements, first-NUL byte-string semantics and
same-argument aliases, including distinct empty arrays. C/Rust/Go providers
receive a read-only view for the call; retaining input requires an independent
copy. Generated C views install string copy/release hooks, so `sn_array_copy`
produces an owning result. Error/panic paths release temporary C credits and
views before returning, and status errors preserve result output pointers.

A C ownership repair preserves string elements borrowed while printing through
a live variable/member/index owner. Printing no longer frees the array's element
before mutation or array cleanup; temporary owned strings retain their existing
cleanup path. Existing Sindarin source and output contracts are preserved.
