# Native package build and binding metadata

Status: manifest validation, plan extraction and preserving dependency edits are
implemented. [Native backing archives](native-artifacts.md) can now be built with
`--build-native`. Complete package compilation and managed type/layout/provider export contracts
remain required work. [Generated native imports](native-package-imports.md) resolve
scalar/string declarations, build original backings and generate consumer adapters.
Unsupported bodies, types and ownership policies receive diagnostics, including
source/model emission and `--no-install`.

This is part of the [runtime/package architecture](runtime-target-architecture.md)
and [Rust completion goal](rust-completion-goal.md). It extends package build
configuration; it does not extend Sindarin language syntax or port native backing
logic into another language. Existing manifests and C source/include/link directives
retain their existing behaviour.

Build language `SN` identifies a Sindarin implementation unit. It requires an
`entry` listed in `sources`; the package's runtime selects C/RS, and omission
inherits the application target. GO implementation bodies produce an explicit
unsupported-backend diagnostic. `--build-package` aliases the artifact builder.
Public native declarations remain the typed ABI contract, with `function`
selecting the separately compiled implementation function. See
[Sindarin body libraries](native-artifacts.md#sindarin-implementation-libraries).

## Schema

Sindarin library bindings can name public static or instance methods as
`src/api.sn::Type.method`, with `function: Type.method` on an SN build. Static
methods keep their declared parameters; instance methods include a `self`
ownership parameter. Native as-ref receivers use the canonical record contract
below. Public declarations may retain their original Sindarin bodies: consumers
call the separately compiled export while the declaration file stays unchanged.
Reference-qualified/generic exports and Rust body record guards remain work.

A C build can declare `provides_sources: [src/io/module.sn.c]` for an existing
`@source` snippet included by that build's actual C sources. The builder verifies
the include dependency before publishing the archive. Consumers and Sindarin
facade builds retain the directive as provenance and link the owning archive;
they do not compile the snippet again. Unclaimed source directives keep their
existing diagnostics/legacy behavior. A prebuilt consumer can remove provided
implementation sources while retaining its unchanged public declarations.

ABI 1.5 additionally supports `native.types` contracts for package-owned,
unpacked `native struct ... as ref` records backed by C. Public declarations
retain their fields and aliases. An example storage contract is:

```yaml
native:
  abi: '1.5'
  declarations: [src/api.sn]
  types:
    - declaration: src/api.sn::Connection
      identity: example/Connection@1
      c_type: ExampleConnection
      header: include/connection.h
      create: example_connection_create
      retain: example_connection_retain
      release: example_connection_release
      refs: example_connection_refs
      owner: atomic
  # builds and bindings remain required
```

The C header must declare `ExampleConnection *create(void)`,
`ExampleConnection *retain(ExampleConnection *)`,
`void release(ExampleConnection *)`, and `int refs(ExampleConnection *)` under
the specified function names. Creation returns one owned credit; retain preserves
identity and returns an additional credit; release destroys storage and its
resources exactly once at zero credits. Retain/release/refs must accept nil.
`owner: atomic` promises thread-safe owner credits; mutable field and resource
operations retain their existing synchronization requirements. These functions
must be exported by the package archives and keep their public C symbols.

Generated code aliases the package's actual C type and delegates construction
and ownership to these functions. It checks the existing C declaration's size,
alignment, field offsets and field types, including `@alias` names. Public C
headers and their transitive package-local includes are sealed into artifact
metadata and validated on prebuilt consumption. Header-only lifetime functions
and a missing language declaration do not constitute an independent record
contract.

Record input ownership is `borrowed`; output ownership is `owned` or `borrowed`.
Borrowed results require `borrowed_from` naming a borrowed record parameter and
receive an independent credit before input guards are released. The wire format
uses shared runtime typed resources with the declared identity; consumers acquire
the actual record before releasing the wire credit. Nil remains nil.

Generated record providers currently require C backing. RS/GO/SN record providers,
value/packed/generic records, interfaces and the complete unchanged SDK facade
remain implementation work. This contract enables C SDK storage consumption; it
does not mark the full package or Rust completion goal achieved.

```yaml
name: mixed-native-package
runtime: RS
native:
  abi: '1.0'
  declarations:
    - src/api.sn
  builds:
    - name: c_io
      language: C
      sources: [native/io.c]
      include_dirs: [include]
      libraries: [z]
    - name: rs_codec
      language: RS
      sources: [native/lib.rs, native/value.rs]
      entry: native/lib.rs
    - name: go_codec
      language: GO
      sources: [native/go/main.go, native/go/go.mod]
      module: native/go
  bindings:
    - declaration: src/api.sn::open
      build: c_io
      symbol: native_open
      convention: C
      failure: status
      ownership:
        parameters: {path: borrowed}
        result: owned
    - declaration: src/api.sn::view
      build: rs_codec
      symbol: native_view
      convention: C
      failure: abort
      ownership:
        parameters: {self: borrowed}
        result: borrowed
        borrowed_from: self
    - declaration: src/api.sn::consume
      build: go_codec
      symbol: native_consume
      convention: C
      failure: status
      ownership:
        parameters: {input: owned, count: value}
        result: value
```

The top-level `runtime` selects the package's Sindarin implementation runtime.
Each build's `language` selects the original toolchain for its backing sources.
They are independent: RS Sindarin code may use C backing, and a package can declare
multiple native build units. Omitted `runtime` still inherits the application
target. `native.abi` selects the shared ABI contract, currently `1.0`, `1.1` or `1.5`.

`declarations` lists the Sindarin API modules supplying signatures and types.
Each binding identifies a declaration using `module/path.sn::name` (or the
qualified method name), the owning build unit, and its C-callable export symbol.
These strings must eventually resolve against the declaration graph; structural
manifest validation alone does not verify that the native implementation matches.

Build names must be nonempty and unique. Every build has a nonempty source-file
list. Paths are interpreted from the package manifest's directory by the artifact
builder. C builds may declare include directories and link libraries. RS builds
specify a crate-root `entry` present in their source list. GO builds specify a Go
module directory and cannot declare an RS entry. C builds cannot declare an entry
or module. Source lists supply explicit build/cache inputs; the artifact planner
must additionally account for discovered headers, module dependencies and lockfiles.

Go units may require aggregation into one bridge/runtime archive. This schema does
not promise arbitrary independent Go runtime archives can be linked together, nor
that Sindarin implementation bodies can compile for Go. GO-backed declarations
still need native bridge generation, and Sindarin-to-Go compilation remains a
later goal.

## Binding contracts

`symbol` names the C-callable wire export. Optional `function` names an ordinary
backing-language function and selects [generated provider adapters](native-package-imports.md#generated-provider-exports).
When omitted, the backing implementation supplies the wire export itself.
`convention` must be `C`. Foreign layouts do not enter the ABI through a cast.
Generated export/import adapters must use the declared signatures, shared ABI and
ownership metadata, respecting exposed layouts and language-visible identity,
copy hooks, aliases, evaluation order and `sizeof`.

Every binding declares a `parameters` ownership mapping (empty for no inputs)
and a result policy. Parameter names correspond to declaration inputs; `self`
identifies an instance receiver. The supported policy names are:

| Policy | Declared responsibility |
| --- | --- |
| `value` | Plain-value wire copy; no managed credit transfer |
| `borrowed` | Owner retains responsibility; borrower has the declared call lifetime |
| `owned` | Transfer one owned value/cleanup responsibility across the boundary |

A borrowed result must name its borrowed owning input in `borrowed_from`. Other
result policies cannot use that field. More complex retained callbacks, userdata,
thread-affine cleanup and managed record/array graphs require further contract
support and corresponding ABI capabilities. The current metadata reader does not
claim those contracts are implemented.

`failure` is explicit: `abort` declares a terminal-error boundary; `status`
declares the status/error transport path. The artifact/type/adapters pipeline must
validate each concrete export and implement the selected error protocol while
containing foreign unwinds. Parsing these policies does not prove function wire
signatures, error payloads or exception containment have been implemented.

## Validation and preservation

Unknown native fields, duplicate mapping keys/build names/binding declarations,
missing inputs, unsupported ABI/languages/conventions/error policies, unresolved
build references and incomplete borrowed-result ownership are rejected. Recursive
or excessive metadata nesting is diagnosed. This avoids accepting misspelled or
unsupported contracts as functioning integration.

`package_yaml_native_plan` returns an owned JSON reference containing the validated
native plan; the caller releases it with `json_object_put`. Scalar metadata remains
textual; source arrays and parameter maps retain their structure. Omission returns
NULL. Failure leaves the caller's output unchanged.

`PackageConfig` records native metadata presence without embedding dynamically
owned plans in recursive package-manager stack frames. Its scalar configuration
writer rejects a native-bearing configuration instead of dropping its contract.
The dependency editor works on the original YAML document and preserves native
metadata and unknown root/dependency fields. It replaces dependency data without
mutating unrelated aliases of the original dependency sequence. Invalid manifests
are rejected before opening the destination for replacement.

Tests cover all three backing languages, ownership/failure declarations, malformed
contracts, dependency add/update preservation and compiler diagnostics. Existing
runtime-selection and C interop regressions remain required.

`abi: 1.1` selects read-only borrowed string-array inputs and managed-value owned
array results. `abi: 1.5` selects live borrowed views of canonical C string-array
headers for inputs; owned array results retain the 1.1 managed-value format.
Generated C, Rust and Go native providers and C/RS Sindarin implementation bodies
support this input contract. Rust bodies compile canonical C array support in their
independent source bundle. These versions preserve nil/empty distinctions
and explicit input/result ownership; `abi: 1.0` retains scalar/string behaviour.

Optional `native.assembly` is a mapping with nonempty `path` and a 64-digit
lowercase hexadecimal `sha256` sealing the exact descriptor bytes. Public
declarations and build/binding contracts remain required. A selected assembly
is verified and consumed without reading backing sources or invoking their
toolchains; no source-build fallback occurs on incompatibility. See
[prebuilt artifacts](native-artifacts.md#consuming-a-prebuilt-artifact).
## ABI 1.6 byte-array contracts

`native.abi` accepts `1.6` in addition to `1.0`, `1.1` and `1.5`. Earlier scalar,
string, string-array and canonical-record contracts remain available in `1.6`.
Byte-array parameters require `borrowed` ownership; byte-array results require
`owned` ownership. Generated providers preserve live canonical C input storage
and return one unique adopted wire credit. See
[byte-array adapters](native-package-imports.md) and
[the runtime byte-view contract](runtime-abi.md).
