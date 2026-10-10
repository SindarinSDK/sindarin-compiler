# Native backing artifacts

Status: the compiler can inspect native plans and build independent native backing
archives. Complete Sindarin package compilation, type descriptors and SDK
decoupling remain required work in the
[Rust completion goal](rust-completion-goal.md). These artifacts explicitly carry
`complete_package: false`. Supported native declaration imports consume them
through [generated consumer adapters](native-package-imports.md), including verified
prebuilt consumption without original backing sources; complete Sindarin
package bodies and SDK artifact imports remain incomplete.

## Commands

```sh
sn --native-plan path/to/sn.yaml --target rust -o plan.json
sn --build-native path/to/sn.yaml --target rust -o .sn/build/native
```

`--native-plan` validates the [native manifest contract](native-package-manifest.md)
and outputs JSON. Omitted package runtime inherits `--target`; explicit runtime
remains the package's selection. This command does not require foreign toolchains.

`--build-native` invokes the installed Python 3 build driver and original C, Rust
and Go tools. Python is required for this optional package command; existing
application compilation retains its current requirements. `SN_PYTHON` can select
the Python executable. Successful output is JSON identifying `assembly.json` and
whether a cached artifact was reused. `-O0/-O1/-O2` and checked/unchecked selection
are recorded in the plan/cache context; native tool optimization follows the level.

Each C build unit compiles its declared sources and creates a static archive. Each
RS unit compiles its declared crate root with Rust into a static library. GO
units build a Go main/bridge module with `c-archive`. Multiple generated-provider
GO units in one package produce one archive; their unit records reference that
same archive, which consumers link once. Bindings without `function`
supply their own C-callable exports. Bindings naming ordinary backing functions
resolve typed declarations and generate provider shims for supported scalar/string
contracts, preserving original backing source and toolchains. Missing exports and
C/Rust/Go implementation signature mismatches are diagnosed. See the
[provider contract](native-package-imports.md#generated-provider-exports).
Native build commands do not compile Sindarin implementation bodies or implement
a Sindarin Go target. `language: SN` units now compile Sindarin function bodies
through the selected C/RS backend, as described below.

Tool selectors are `SN_CC`, `SN_AR`, `SN_NM`, `SN_RUSTC` and `SN_GO`. C flags use
`SN_CFLAGS`; Rust flags use `SN_RUSTFLAGS`. Subprocesses receive argument arrays,
including paths containing spaces. Commands are not shell source. Native commands
reject combinations with application emission/debug or package maintenance flags.

## Artifacts and compatibility

A published generation contains archives and an `assembly.json` descriptor with:

- Package identity/version and resolved Sindarin implementation runtime.
- ABI version, host OS/architecture/pointer width and runtime archive identity.
- Exact public declaration source bytes and hashes, plus binding/ownership policies.
- Native build language, archive bytes/hash and declared export symbols.
- Link libraries, Rust-reported native static-library flags and discovered cgo flags.
- Toolchain versions/binary identities, build context and dependency hashes.
- Toolchain initialization requirements and Go aggregation requirements.

The builder checks native target settings against the compiler/runtime host.
Cross-platform artifact builds require coordinated toolchain/runtime planning and
currently produce an unsupported-target diagnostic instead of incorrect host
metadata. Artifacts remain native backing units; raw declarations are not yet a
verified type/layout/reflection or generic specialization contract.

The final package/link pipeline must select one compatible shared C runtime and
validate complete package ABI contracts. This builder does not infer custom
initialization/shutdown hooks, error wire signatures, foreign layouts, or callback
lifetimes from native source. Supported scalar/string native declaration imports use generated consumer
adapters. Unsupported complete package bodies and managed contracts retain
explicit diagnostics until those pipelines are implemented.

## Cache and publication

Cache context includes manifest/build policies, package input bytes, compiler/
driver identities, native tools, ABI header/runtime bytes, platform and environment
fingerprints. Environment values are hashed rather than serialized into artifacts.
C compiler dependency files and Rust dependency information account for discovered
source/header inputs, including headers outside the package. Cached archive and
metadata bytes are validated before reuse. Changes or corruption cause rebuilding.

Published generations are immutable and identified by their descriptor hash.
A new generation is prepared before atomically replacing the cache's current
pointer. Existing consumers retain their generation paths. Package/dependency
changes detected during a build prevent publication. Missing inputs, unavailable
toolchains, failed builds and missing exports produce diagnostics and no new
publication marker.

Go source/module inputs and cgo link dependencies are recorded. Native Go build
cache reuse is disabled pending complete cgo transitive-header capture; Go's own
compiler cache remains active. Source builds aggregate multiple generated-provider
Go units/packages into one bridge/runtime archive. Explicit sealed prebuilt
artifacts can be reused independently of that source-build cache.

The default output `.sn/build/native` is removed by normal build-cache cleanup.
Custom output directories are controlled by the caller.

## Sindarin implementation libraries

An implementation unit can compile original Sindarin code independently of the
application. Its package runtime selects C/RS; an omitted runtime inherits the
application target. Public declarations and bindings describe the ABI exports:

```yaml
name: example-library
runtime: C
native:
  abi: 1.0
  declarations: [src/api.sn]
  builds:
    - name: implementation
      language: SN
      entry: src/body.sn
      sources: [src/body.sn, src/helper.sn]
  bindings:
    - declaration: src/api.sn::echo
      function: echo
      symbol: example_echo_v1
      build: implementation
      convention: C
      failure: abort
      ownership: {parameters: {text: borrowed}, result: owned}
```

Here `api.sn` contains `native fn echo(text: str): str`, while `body.sn` contains
the original ordinary `fn echo(text: str): str` implementation. `--build-package`
builds the declared units into reusable archives. Generated facades and provider
exports adapt the implementation's values to the shared C ABI. Consumers import
the public API, link the compiled library and retain their own application target.
Public and implementation types/qualifiers must agree. C implementation symbols
are package-scoped; Rust implementations use distinct crate identities.

Scalar/string exports currently support direct and status wire protocols, nil
versus empty strings and non-UTF-8 bytes. Package-body emission has no application
entry; competing `main` symbols are rejected. Implementation/dependency source
hashes and selected runtime are recorded. These units can also be consumed as
sealed prebuilt artifacts after implementation sources are removed.

Full package initialization/shutdown, implementation globals, managed array/record
body exports, generic public APIs and complete native-backed SDK facade migration
remain required work. Unsupported contracts fail clearly. RS body emission that
needs native C sidecars still requires the modular library lifecycle extension;
native C/Go backing is kept in its original language. An SN unit selecting GO
reports the unimplemented Sindarin Go backend. This initial body-library path
does not satisfy the whole Rust completion goal.

## Consuming a prebuilt artifact

A distribution can retain its public `.sn` declarations and manifest while
shipping the immutable generation's archives and `assembly.json`. Original
C/Rust/Go backing source, backing headers, Go modules and producing toolchains are unnecessary
for consumption. Keep the build and binding entries as declarations of the
backing-language/export contract, then select the sealed descriptor:

```yaml
native:
  abi: 1.0
  assembly:
    path: dist/assembly.json
    sha256: <SHA-256 of the exact descriptor bytes>
  declarations: [src/api.sn]
  # Retain the original builds and bindings metadata.
```

The reader checks the descriptor seal, schema, package identity/version/runtime,
ABI, platform/architecture/pointer width, exact declaration bytes, resolved provider
types, ownership/binding contracts, build/export/initialization inventory and
archive hashes. Archives must stay within the descriptor's generation directory.
An incompatible or corrupt artifact fails explicitly; it does not trigger a
source rebuild. Applications still need their own C/Rust target toolchains,
headers explicitly referenced by public API pragmas, and declared external link
libraries.

The application links its current canonical C runtime. Producer paths, toolchain
fingerprints and runtime hashes remain provenance. ABI 1.0 transport remains
supported by ABI 1.1. Consumer adapters query the live runtime's version,
capabilities and scalar/pointer widths before making a foreign call. For explicit
`--build-native` selection, the JSON result includes the consumer `shared_runtime`
alongside `assembly`, `prebuilt: true` and `cache_hit: true`.

A prebuilt Go assembly can contain several units referencing one aggregate
runtime archive. Combining it with another Go assembly or source Go package
requires rebuilding one aggregate source graph and currently produces a clear
diagnostic. Complete source-independent multi-package Go composition remains
required work, along with full Sindarin package lifecycle and SDK artifact imports.

## Validation

`python3 tests/package/native_artifacts.py` builds original C/Rust/Go backing files,
then links their archives into C and Rust consumers. A Go export returns a string
allocated by the shared C runtime, which both consumers inspect and release. Tests
cover spaces in package paths, target inheritance, command conflicts, dependency
invalidation, immutable generations, corrupted archives/metadata, missing inputs,
missing exports and unavailable tools. The unified core gate runs these checks on
Linux, macOS and Windows.

This is not final SDK/mixed-Sindarin-package acceptance. Managed record/interface
contracts, generated adapters, portable package bodies, SDK resources and full
Rust/C parity remain outstanding.

Applications importing several generated-provider Go libraries use one aggregate
bridge archive. The Go toolchain builds original library modules and their shared
dependencies together, preserving one Go runtime. Incompatible module roots or
replacement directives are diagnosed; unrelated handwritten main archives cannot
be combined. Complete prebuilt/source-independent Go package consumption remains
part of the wider package artifact work.
