# Generated native package imports

Status: implemented consumer adapters for resolved scalar and string function
contracts. Complete independent Sindarin package bodies, SDK artifact imports,
managed record/interface/array/callback contracts and generated provider shims
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

Native backing must currently provide the declared C-callable wire exports.
Consumer adapters are generated; automatic provider export shims for ordinary
Rust/Go functions remain later work. The compiler rejects unsupported types,
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

## Linking and validation

The compiler passes archive paths and discovered native library options to the
C and Rust linker plans. Ordinary library names retain legacy translation and
configuration overrides. The shared runtime resolves the generated value adapters.
More than one Go archive in a final graph requires aggregate bridge planning and
currently produces a diagnostic rather than linking incompatible Go runtimes.

Run `python3 tests/package/native_imports.py` with the pinned SDK integration
checkout available. Tests build C/Rust/Go native packages, use generated adapters
from real C/Rust Sindarin callers, validate owned/borrowed string lifetime and
error exits, and reject ownership mismatches. A Rust application also uses SDK
TextFile resources/arrays with all three backing languages and a portable Sindarin
request-line module. The SDK call path in that test remains its existing C
compatibility path; it does not prove an independent SDK artifact import.

Full corpus/mode/platform parity and complete mixed-package SDK acceptance remain
outstanding. Passing these fixtures does not satisfy the entire Rust completion goal.
