# Native backing artifacts

Status: the compiler can inspect native plans and build independent native backing
archives. Complete Sindarin package compilation, type descriptors and SDK
decoupling remain required work in the
[Rust completion goal](rust-completion-goal.md). These artifacts explicitly carry
`complete_package: false`. Supported native declaration imports consume them
through [generated consumer adapters](native-package-imports.md); complete Sindarin
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
RS unit compiles its declared crate root with Rust into a static library. Each GO
unit builds a Go main/bridge module with `c-archive`. Bindings without `function`
supply their own C-callable exports. Bindings naming ordinary backing functions
resolve typed declarations and generate provider shims for supported scalar/string
contracts, preserving original backing source and toolchains. Missing exports and
C/Rust/Go implementation signature mismatches are diagnosed. See the
[provider contract](native-package-imports.md#generated-provider-exports).
Native build commands do not compile Sindarin implementation bodies or implement
a Sindarin Go target.

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

Go source/module inputs and cgo link dependencies are recorded. Native Go artifact
reuse is currently disabled pending complete cgo transitive-header capture and
bridge aggregation; Go's own compiler cache remains active. More than one GO unit
in one native plan is rejected with an aggregation diagnostic. Combining Go
backings across the final package graph remains required implementation work.

The default output `.sn/build/native` is removed by normal build-cache cleanup.
Custom output directories are controlled by the caller.

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
