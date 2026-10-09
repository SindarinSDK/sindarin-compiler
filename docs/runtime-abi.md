# Shared runtime ABI

Status: implemented value-transport foundation, ABI 1.0 (`0x00010000`). This is
part of the [runtime and package architecture](runtime-target-architecture.md).
It does not yet provide complete package record/interface contracts, managed
array elements, complete generated bindings, independent SDK artifacts or compiler-wide
adoption. Those remain required by the [Rust completion goal](rust-completion-goal.md).

The public header is `src/runtime/sn_abi.h`, staged at
`bin/include/runtime/sn_abi.h`. Implementations are compiled in C into the existing
`libsn_runtime_min.a`; no separate Rust or Go implementation of these services is
introduced. String concatenation and plain-value array allocation, growth and
copying use the existing C runtime operations. Existing generated C layouts,
entry points and default-target behaviour are preserved.

## Negotiation and scalar representations

Call `sn_abi_v1_query` before consuming an artifact. It checks the exact version,
required capability bits and output structure capacity. Failure leaves outputs
unchanged. ABI 1.0's `SnAbiInfo` layout is fixed; a later incompatible contract
needs new versioned entry points. A package artifact must also match the host's
pointer width, architecture and calling convention; this query does not replace
artifact compatibility metadata.

| Sindarin/ABI value | Wire representation |
| --- | --- |
| `int`, `long` | signed 64-bit integer |
| `uint` | unsigned 64-bit integer |
| `int32`, `uint32` | signed/unsigned 32-bit integer |
| `bool` | unsigned 8-bit value, 0 or 1 |
| `byte`, `char` | 8-bit octet; adapters preserve existing char interpretation |
| `float`, `double` | host C 32-bit/64-bit floating representation |
| Status/version | unsigned 32-bit integer |
| Lengths/capabilities | unsigned 64-bit integer |
| Foreign resource pointer | `void *`, host pointer width |
| Callback context | `uintptr_t`, native-sized registry token |
| Managed value | opaque `SnAbiValue *`; NULL is nil |

Public structs use ordinary platform C alignment, no packed structs or enum-width
assumptions. Rust declarations need matching C layout. Go uses cgo declarations.
Lengths are range-checked before conversion to host allocation sizes and C array
indices. Values are a native ABI, not a serialized/network format.

Capabilities currently cover values/bytes (`1`), plain-value arrays (`2`) and
resources with destructor callbacks (`4`). Unimplemented capabilities are not
advertised. The C, Rust and Go clients verify the negotiated widths and layouts.

## Ownership and identity

Every successful non-nil constructor or copy returns one owned credit. Retain
returns the same handle and another credit; release drops one credit. The final
release frees C-owned storage or calls the resource's declared destructor exactly
once. Retain and release accept nil. Callers must hold a live credit before using
or retaining a handle; releasing a credit does not clear another alias variable.

Credits are atomic. Concurrent retain/release is supported while each participating
caller has a live credit. Mutable payload access requires external synchronization.
Credit operations do not make concurrent array mutation safe. No global runtime
lock is held during resource destruction, allowing callbacks to reenter services.
A destruction callback cannot resurrect or access the handle being destroyed.

Borrowed byte views and resource pointers acquire no credit and must not outlive
their owner. Rust/Go clients must not free C storage or cast it into an owned
String/Vec/string/slice. Resource payloads may belong to another allocator: the
registered destructor, compiled in the backing language, owns their cleanup.

Retain expresses aliasing; copy produces independent value storage. Generated
adapters must select these operations using the language qualifiers and package
ownership metadata. An opaque handle's private size is not the language-visible
`sizeof` or an exposed SDK record layout. Existing public fields, identity and
layout contracts still require compatible package adapters.

## Strings and buffers

String copy consumes a borrowed NUL-terminated C string synchronously and copies
bytes through its first terminator. Non-UTF-8 octets are preserved. Nil is distinct
from empty: NULL input gives nil; a non-NULL empty string gives a live empty handle.
The returned view excludes the trailing terminator, available at `data[length]`.
`sn_abi_v1_string_bytes` provides a string-only view and rejects buffers without
changing its output, so provider adapters never assume an arbitrary buffer has
a terminator. Existing general byte views retain their established behaviour.
Concatenation follows the existing C string operation, including nil operands
producing an empty string when both operands are nil.

Buffer copy accepts an explicit length and preserves all binary bytes, including
NUL. NULL/zero gives nil; non-NULL/zero gives a non-nil empty buffer. The input is
not retained. Buffer views do not promise a trailing terminator. This contract does
not reinterpret byte buffers as language strings or change existing string rules.

## Plain-value arrays

ABI arrays wrap the existing `SnArray`. The declared element size is the exact
wire stride. Push/get/set validate that size and indices; get copies into caller
storage. Copy duplicates the array; retain shares its identity and mutations.
Nil copy stays nil and nil length is zero. Nil has no element-size declaration.

These operations support scalar and explicitly plain-value layouts. They do not
implement owned string/record/function/array elements or user copy hooks. Package
adapters need managed-element descriptors and ownership operations before those
contracts can be enabled. Passing bytes containing an owned pointer is not an
implementation of managed array semantics.

## Resources, callbacks and errors

Resource creation accepts a payload, C-callable destructor and native-sized context.
On success, the handle owns its declared cleanup responsibility; on failure,
responsibility stays with the caller. A NULL destructor means no payload cleanup
is performed. NULL payloads are permitted for registry-token resources.

Destructors run synchronously on the thread performing the final release. The
backing adapter must permit that thread or arrange an appropriate final release.
The backing library must remain loaded while its handles/callbacks are live.
Callbacks must contain foreign exceptions/panics; no unwind crosses C. Go-owned
state uses `runtime/cgo.Handle` or an equivalent valid registry protocol; C does
not retain a Go object pointer. The test also exercises callbacks that call the
shared runtime again. General callable exports, error payloads and thread-affine
resource contracts remain part of the package ABI implementation work.

Status codes report invalid arguments, version/capability mismatches, wrong value
kinds and out-of-range indices/lengths. Outputs remain unchanged on failure.
Status messages are static borrowed strings. There is no mutable global last-error
slot. Allocation exhaustion retains the existing C runtime's fatal error/exit
policy. Adapters must preserve established evaluation order, alias lifetimes and
replacement cleanup; a new transport call does not authorize reordering.

These services require no process initialization. Package initialization and
shutdown ordering still need artifact metadata and final-link planning.

## Validation

Run `python3 scripts/check_runtime_abi.py` after building. C, Rust and Go clients
link the same staged archive and exercise negotiation, nil/empty distinctions,
binary bytes, copied inputs, alias lifetime, independent array copies, mutation,
bounds, destruction and callback reentry. A native Go archive is also built and
its C-callable exports are consumed from Rust without translating its backing code.
The Rust client performs final Go-resource release on a worker thread. Go
clients/bridges build with `GOEXPERIMENT=cgocheck2`.

Run `python3 scripts/check_runtime_abi.py --sanitize` to instrument the actual C
implementation and C lifetime client with AddressSanitizer and
UndefinedBehaviorSanitizer. Eight threads perform 160,000 retain/release operations
while a resource remains alive, then its final cleanup is checked exactly once.
This gate runs on Linux; ordinary C/Rust/Go clients run on all supported CI platforms.

These are runtime ABI clients, not proof that a Sindarin application's generated
package adapters already consume this ABI. Full mixed-package SDK acceptance and
all remaining language parity requirements are still outstanding.

References: [Rust FFI](https://doc.rust-lang.org/nomicon/ffi.html),
[Go cgo and pointer rules](https://pkg.go.dev/cmd/cgo).

Generated status providers return `SN_ABI_FOREIGN_ERROR` (6) for contained
backing-language panics. Existing status numbers and ABI 1.0 layouts remain
unchanged. String provider artifacts require the string-view runtime entry point;
their provenance/cache identity includes the exact runtime archive and header.
