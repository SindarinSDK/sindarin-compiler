# Runtime and mixed-language package architecture

Status: architectural direction with implemented capabilities, updated 2026-10-11.

Local ABI 1.8 work adds canonical reference-record arrays with copied leaf type
identity, native slot hooks and unique owned transfer. Generated native C/RS/GO
providers and C/RS Sindarin bodies consume the same C headers. The default SDK
stager now includes all public XML methods, giving seventeen modules and
34 C/Sindarin units. SDK XML correction `0b71e84` has complete hosted acceptance
at run 38164835929; this compiler expansion remains local until its compiler
dependency run 38162607031 is accepted. Wider contracts and final platform/corpus
proof remain required.

The [completion ledger](rust-parity-progress.md) distinguishes local validation,
required hosted acceptance and remaining work. C remains the default application
target; the full Sindarin Go backend remains outside this goal.

The shared [C ABI](runtime-abi.md) implements value transport, typed resources,
initialization/call leases/shutdown, replacement with live aliases and reentrant
cleanup, canonical byte/string arrays and typed scalar/string arrays through
rank 32 (ABI 1.0–1.7). C/Rust/Go native provider units and C/Rust Sindarin bodies
use these transports. Canonical record bodies preserve incoming borrowed credits,
owned returns and parameter rebinding. The integrated record/opaque ownership
and thirteen-module SDK stage have complete hosted compiler acceptance at
`757d9d0c`, verified by 373 gates across Linux/macOS/Windows. Go-native dependencies
use original Go implementations and generated C-callable bridges; a GO package
requiring compilation of Sindarin bodies produces the unsupported-backend diagnostic.

Local integration extends the reusable C-backed SDK stage to sixteen modules,
32 C/Sindarin units. All public Sindarin declarations are preserved, and backing
C sources are removed for sealed consumption. Sixteen original callers pass 576
source/sealed C/Rust mode cases, plus 72 stdin controls (648 total). StringBuilder,
Process and Json retain original public operations and canonical C storage;
Process also repairs a reproduced POSIX dual-pipe deadlock. Json copy hooks retain
the original typed-nil/null-wrapper behavior. Generated ordinary `as val` and
`copyOf()` adapters use declared C copy functions, preserving independent native
storage, source aliases and temporary-producer cleanup. The SDK correction and
this wider compiler integration require complete hosted acceptance. Compiler
CI 38162607031 failed macOS record-guard traversal and Windows SDK C link length;
a combined correction is pending and feature publication remains frozen. SDK
resource correction 2152da2 and XML correction 0b71e84 have hosted SDK acceptance.

The optional record copy contract, managed-field and public-copy-method support,
native resource lifetimes, original caller outputs and nil behavior have focused
normal/sanitizer evidence. Parsing a manifest or rejecting unsupported contracts
is not parity evidence. Whole SDK packaging (including wider encoding/network
modules), package-exported qualifiers and callbacks, complete value record/interface
and generic contracts, dependency lifecycle graphs and final 1,365-source
platform/mode/oracle proof remain required work. The implemented lifecycle
controls do not yet establish complete dependency initialization/shutdown graphs.

## Decision

Sindarin applications choose a compilation target. Packages are independently
buildable units and may choose a different implementation runtime. A shared,
versioned C ABI connects the language runtime, package exports and generated
adapters. Native implementations keep their original language.

The canonical shared Sindarin runtime is implemented in C for now and must be
consumable from C, Rust and Go. This does not require three SDK implementations.
The actual SDK package can retain its C implementation behind the common ABI.
Target-specific bindings adapt values and calls; they do not duplicate SDK logic.

C remains the default target. Existing Sindarin syntax, public SDK API and C
behaviour must be preserved throughout implementation and migration.

## Terms

| Term | Meaning |
| --- | --- |
| Application target | Language/toolchain used to compile application Sindarin code: C, Rust or eventually Go |
| Shared Sindarin runtime | Common services and value/ownership contract; its canonical implementation is C |
| Package `runtime` | Compilation runtime chosen for that package's own Sindarin implementation |
| Native backing language | Language of a package's interop source files; compiled with that language's toolchain |
| Package assembly | Compiled package artifacts plus public API, ABI and build metadata |
| SDK package | Higher-level Sindarin APIs and implementations, distinct from the language runtime |
| Native libraries package | External dependencies such as json-c, OpenSSL and zlib |

An assembly here is a native package artifact, such as a static archive or shared
library, with accompanying metadata. This does not specify a CLR/.NET assembly
format or a new universal bytecode.

Selecting Rust describes how the application's Sindarin code is compiled. A Go
interop dependency can still bring its Go runtime into that application's process.
That runtime is distinct from the shared Sindarin runtime.

## Compilation architecture

```mermaid
flowchart TD
    A[Application source and sn.yaml] --> P[Resolve package graph and public APIs]
    P --> B[Build or load individual package artifacts]
    B --> C[C package exports]
    B --> R[Rust package exports]
    B --> G[Go package exports]
    C --> ABI[Shared versioned C ABI]
    R --> ABI
    G --> ABI
    ABI --> AD[Generate adapters for the application target]
    AD --> APP[Compile application Sindarin code]
    APP --> L[Link final binary and package dependencies]
    RT[Shared Sindarin runtime - C implementation] --> L
```

1. Resolve imports and dependency versions and load public Sindarin declarations.
2. Determine the application target and each package's compilation runtime.
3. Build or select compatible package artifacts using their own native toolchains.
4. Export package entry points through the common ABI and generate importing
   adapters for the application's target.
5. Compile the application and link its adapters, package artifacts and runtime.
6. Validate required runtime initialization, shutdown and callback registration.

Package initialization is distinct from application `main`; dependencies must not
produce competing executable entry points. Exported symbols are namespaced by
package identity, and dependency initialization follows a documented graph order.

## sn.yaml runtime field

The supported manifest values are `C`, `RS` and `GO`:

```yaml
name: sindarin-pkg-sdk
version: 1.0.0
runtime: C
```

```yaml
name: rust-backed-package
version: 1.0.0
runtime: RS
```

```yaml
name: go-backed-package
version: 1.0.0
runtime: GO
```

`runtime` chooses how that package's Sindarin implementation is compiled. It does
not change the final application's target and does not translate foreign source
files into the selected language. Native sources also need explicit build inputs
and binding metadata, defined in the [native manifest contract](native-package-manifest.md).
Its parser/preservation and [native backing artifact builds](native-artifacts.md)
are implemented. Complete Sindarin package compilation and wider adapter contracts remain work. For example, an RS package can still declare an explicit C dependency.

For a package consisting only of portable Sindarin source, an omitted runtime
inherits the application's target. Its artifact is therefore cached separately
for each target. An explicit runtime fixes the package artifact's implementation
runtime and consumers use generated ABI adapters.

Legacy manifests retain their current behaviour: C remains the default application
target, pure Sindarin imports follow the selected target, and existing C native
source/include/link directives remain C interop. Adding the new architecture must
not silently reinterpret or remove those directives.

The manifest parser validates and preserves `runtime`, including dependency add
and update operations. Omission is distinct from an explicit C selection. Runtime
values must match the spellings above; duplicate or malformed runtime declarations
produce errors. Nested extension metadata does not select a package runtime.

The compiler checks the manifests owning imported modules, including transitive,
relative and SDK imports, before code emission. Matching C/RS selections currently
use the existing source compilation path. Native-only packages with implemented scalar/string contracts can cross runtimes
through [generated consumer adapters](native-package-imports.md). Cross-runtime
Sindarin implementation bodies still require complete independent package
compilation; GO implementation bodies report the unsupported Go backend.
These checks also apply with `--no-install` and source/model emission. Native
build/binding metadata is structurally validated and retained when editing
dependencies; supported native declaration imports build their original backing language and
connect generated call adapters; unsupported contracts are diagnosed.

The application's CLI target remains independent of the runtime declared in its
own manifest; imports belonging to that same manifest follow the application
target. Existing native C directives keep their C backing language even for RS
packages. Native build-plan execution produces independent C/Rust/Go backing archives.
Complete Sindarin package artifacts, wider type/ownership verification and final
package-graph linking remain required implementation work. Supported native scalar
and string declarations generate both consumer adapters and optional provider exports.

## Mixed-package example

A Sindarin application targeting Rust imports these dependencies:

| Dependency | Build behaviour |
| --- | --- |
| SDK, `runtime: C` | Build its Sindarin facade/native C implementation as a C package artifact |
| HTTP, pure Sindarin and no fixed runtime | Compile its Sindarin implementation into a Rust package artifact |
| Package with Rust backing | Compile Rust backing code with Rust and expose the declared common-ABI exports |
| Package with Go backing | Build Go code and a C-callable export bridge with Go |

The final application is compiled as Rust and consumes the package artifacts
through generated adapters. C, Rust and Go backing implementations are not
rewritten as Rust. This design also permits a C application to consume RS/GO
package artifacts through the same boundary.

Go's `c-archive` and `c-shared` modes build a bridge main package and its imported
packages, exposing cgo-exported functions. Multiple generated-provider Go libraries now use one
aggregate bridge/runtime build, including several GO build units in one package. Individually buildable package units do not imply
that arbitrary independent Go runtime archives can be linked together. The generated bridge preserves original Go library sources and module
dependencies; conflicting module roots/replacements and incompatible handwritten
main archive combinations are diagnosed. Complete artifact planning must preserve
these toolchain constraints.

## Common runtime and package ABI

The common ABI must be explicit and versioned. It must define:

- Scalar widths, signedness, boolean/character representations and nil.
- Strings and buffers with their length, encoding and ownership rules.
- Array and record access, value-copy semantics, layout metadata where required,
  and opaque resource handles where internal layouts are private.
- Allocation, release, retain and destruction entry points, including which
  allocator owns returned memory.
- Borrowed versus transferred values, aliases and observable identity/lifetimes.
- Callback signatures, registration, userdata ownership, reentry and thread rules.
- Errors and panic boundaries, initialization/shutdown and ABI compatibility.

No Rust String/Vec/trait object or Go string/slice/interface representation becomes
part of the common binary contract merely by casting it. Runtime adapters use the
specified ABI representations and operations. Go/Rust object state retained by
foreign code needs valid handles or the explicitly supported lifetime protocol.

Language-visible behaviour must remain consistent, including evaluation order,
copy hooks, arithmetic modes, cleanup, aliases, nil and existing `sizeof`/identity
contracts. Backend-private layout can vary only where it is not observable through
those contracts. Foreign layouts are explicit ABI commitments.

Rust/Go backend support code remains necessary to adapt their emitted code to this
runtime. It must implement the common contract rather than invent incompatible
ownership rules. A Go dependency's garbage collector is not a substitute for
Sindarin's deterministic resource-cleanup rules.

## Automatic adapter generation

Adapters are generated from the package's public Sindarin declarations and its
ABI/ownership/binding metadata. Generate both the package export adapters and the
consumer import adapters as required. The consumer does not hand-write wrappers
for each dependency.

For native exports, metadata maps declared entry points to their backing symbols
and specifies the lifetime/ownership information that signatures alone cannot
express. Automatic generation is not arbitrary reflection over Rust/Go source:
unsupported or incomplete contracts must produce clear errors. Custom integration
hooks may be required for complex foreign APIs; their implementation stays in the
package's original language.

Same-language direct calls can be a later optimization. Correctness must first be
established through the shared ABI so target selection remains independent of a
package's implementation runtime.

## Package artifact metadata and final linking

An artifact carries public symbols/types, binding and ownership metadata, package
identity/version, shared-runtime ABI version, implementation runtime/toolchain,
platform/architecture, native link dependencies and initialization requirements.
Generic definitions or specializations must also be available where application
instantiation requires them; a native archive alone cannot represent every generic
or reflection requirement.

Build-cache keys include those properties and the required optimization/arithmetic
settings. The final linker checks compatibility, selects one compatible shared
Sindarin runtime instance, resolves transitive native libraries and reports missing
symbols, ABI mismatches or unavailable toolchains. Prebuilt artifacts can avoid
rebuilding native dependencies, provided their compatibility metadata agrees.

Go bindings require cgo and a compatible C toolchain on supported platforms.
Cross-compilation must coordinate every native dependency's architecture/toolchain;
selecting an application target does not make foreign packages portable by itself.

## Current SDK and migration

The current SDK contains public `.sn` modules and C `.sn.c` sidecars. Its C sources
use generated records and constructors such as `__sn__TextFile__new()`. The current
Rust backend already compiles native C code and generates private interop adapters;
it has not rewritten the SDK in Rust.

The TextFile native module now has a package-owned C record/constructor and an
independent build path reusing its existing C implementation. Its unchanged
Sindarin declarations and generated C compatibility path are preserved; native
C/Rust/Go clients and SDK standard platform CI are verified. The pinned compiler
integration gates provide hosted acceptance for that pilot.

To produce complete reusable SDK/package artifacts, extend this decoupling from
per-application generated C declarations to the wider SDK and generated adapters. Define the runtime/SDK ABI and preserve
compatibility adapters for the current C API, layouts, allocation and release
behaviour. Existing native fields must be inventoried as API-visible; underscore
names are not permission to remove them.

Migration sequence:

1. Define and test the shared runtime/package ABI and artifact metadata.
2. Add the runtime manifest field and native build-plan metadata while preserving
   legacy manifest parsing, serialization and C builds.
3. Prove one independently built SDK resource module consumed by C and Rust apps,
   with automatically generated adapters and unchanged Sindarin callers.
4. Prove Rust-native and Go-native dependency bridges. This can use small Go
   binding clients before a Go Sindarin backend exists.
5. Extend independent package compilation, caching and final linking to the SDK
   and wider package graph, preserving generic/type and ownership contracts.
6. Complete the remaining Rust language parity gaps and full platform/mode audits.
7. Implement and verify the Go Sindarin backend later against the same contracts.

This architecture is a build/package/runtime capability. It preserves the current
C default and language contract and does not require maintaining three complete
SDK implementations.

## Technical references

- [Rust C interoperability](https://doc.rust-lang.org/nomicon/ffi.html)
- [Rust library linkage](https://doc.rust-lang.org/reference/linkage.html)
- [Go library build modes](https://pkg.go.dev/cmd/go#hdr-Build_modes)
- [Go C interoperability and pointer rules](https://pkg.go.dev/cmd/cgo)
- [Go callback-state handles](https://pkg.go.dev/runtime/cgo#Handle)
