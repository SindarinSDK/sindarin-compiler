# Shared runtime ABI

Status: implemented value-transport foundation, ABI 1.0 (`0x00010000`) and
optional managed-value capabilities through ABI 1.1 (`0x00010001`) and package
lifecycle coordination through ABI 1.2 (`0x00010002`) and managed-array slot
replacement through ABI 1.3 (`0x00010003`) and managed-array mutation through
ABI 1.4 (`0x00010004`) and canonical native string-array views through
ABI 1.5 (`0x00010005`) and canonical native byte-array views through
ABI 1.6 (`0x00010006`). This is
part of the [runtime and package architecture](runtime-target-architecture.md).
It does not yet provide complete package record/interface contracts,
complete generated bindings, independent SDK artifacts or compiler-wide
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

## Package lifecycle (ABI 1.2)

Capability `32` adds opaque package controls and owned call credits. ABI 1.2
advertises mask `63`; queries for 1.0 and 1.1 retain masks `7` and `31` and reject
the lifecycle bit. Value/resource layouts and earlier status values are unchanged.

`package_new` captures initializer, cleanup and a native-sized context token.
`package_begin` initializes once, waiting for concurrent initialization. The
initializer runs outside the gate lock and may reenter on its own thread; other
callers see completed initialization. A failed status is sticky and preserves
call outputs. Initializers own rollback of provisional state on failure.

Call credits hold package ownership until `package_end`. End must run on the
creating OS thread; Go callers should pin that thread across a call with
`runtime.LockOSThread`. Calls can end in any order on their owning thread.

Shutdown rejects a same-thread active call/initializer with `PACKAGE_BUSY` (`7`)
rather than deadlocking. It blocks new calls, drains existing calls and invokes
cleanup once outside the lock. Cleanup may reenter shutdown; calls during/after
closing fail with `PACKAGE_CLOSED` (`8`) and preserve outputs. Last-credit release
also shuts down an initialized package. An uninitialized/failed control does not
invoke cleanup. Callback package pointers are borrowed; contexts belong to their
backing implementation and callbacks must not unwind through C.

`scripts/check_package_lifecycle.py` exercises C/Rust/Go callbacks, sixteen
concurrent C callers, reentry, shutdown draining, sticky failure, aliases and final
call-credit release. Its sanitizer mode instruments the canonical C implementation.

## Value ownership and identity

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

## Typed resource identity

Package adapters can adopt resources with `sn_abi_v1_resource_new_typed` and a
package/type/ABI identity such as `SindarinSDK/sindarin-pkg-sdk:io.TextFile@1`.
The runtime copies the identity; caller storage may be changed or freed after
construction. Retain preserves handle identity and final release invokes the
backing destructor exactly once before releasing the identity storage.

`sn_abi_v1_resource_data_typed` rejects generic, non-resource and differently
tagged live values before publishing a payload pointer. Nil remains nil-safe.
Invalid arguments/type mismatches leave outputs unchanged and failed construction
does not adopt cleanup responsibility. `sn_abi_v1_resource_type` borrows the
identity while an owner credit remains live. The tag is an ABI producer's type
assertion; it does not validate an arbitrary pointer's internal layout.

Existing generic resource calls, public layouts, status numbers and ABI version
remain unchanged. Typed-resource users require these runtime entry points;
complete package artifact compatibility must check their availability/provenance.
This foundation does not implement complete record/interface field contracts or
compiler-generated managed-resource imports. Those remain goal requirements.

## ABI 1.1 managed-value arrays

Query `SN_ABI_V1_1_VERSION` to negotiate managed-value arrays (`8`) and typed
resources (`16`), in addition to the original capabilities (`1/2/4`). ABI 1.1
reports capabilities `31`; ABI 1.0 queries retain their exact existing `7` mask
and layouts. Asking ABI 1.0 for new capabilities returns unsupported. Older
runtimes reject the 1.1 version explicitly rather than claiming its contracts.

Managed-value arrays hold C-runtime handle credits. Push/set borrow their input
and retain a credit; get returns an owned credit. Nil elements remain nil. Retain
aliases one array; copy creates independent slots retaining the same element
values. Strings are immutable through this ABI; mutable resource/array elements
retain their identity. Language copy hooks/deep-copy qualifiers require explicit
higher-level adapters and are not implied by this slot copy operation.

Replacing a slot acquires the incoming credit, publishes it, then releases the
previous owner. Destructors can reenter and grow a live array without leaving a
stale slot reference. Final array release drops all element credits. Callers
serialize mutation and avoid reference-count ownership cycles; these operations
do not introduce garbage collection or claim language-level cycle handling.

POD and managed arrays have different private kinds. Mixed calls are rejected,
bounds are checked and failure outputs remain unchanged. Nil copy remains nil
and nil length is zero. C/Rust/Go clients prove element lifetime after array
release, independent slot copies and ownership; C address/undefined sanitizers
also exercise reentrant destruction that resizes the array.

## ABI 1.3 managed-array replacement

Query `SN_ABI_V1_3_VERSION` with `SN_ABI_CAP_ARRAY_REPLACEMENT` (`64`) before using
`sn_abi_v1_value_array_assign(destination, source)`. ABI 1.3 advertises `127`;
queries for ABI 1.0, 1.1 and 1.2 retain exactly `7`, `31` and `63`. Those older
queries reject the new capability without changing their output structure.
The public layouts and older operations are unchanged.

Assignment copies the source slots into the existing destination handle. Each
incoming element obtains a retained credit before the replacement is published;
old slots are detached and released afterward. Retained destination aliases see
the replacement. The source keeps independent slots, while contained resource
identity remains shared. Retained elements survive clearing or releasing either
array. Nil source clears a non-nil destination to an empty array. Nil destination
is invalid; wrong-kind arguments preserve destination contents. Self-assignment
of a managed array is a no-op.

A resource destructor released from the old slots may inspect, grow, clear or
replace the already published destination. Nested changes remain visible after
outer assignment completes. An operation credit keeps the destination alive even
if cleanup releases its external credit. Caller serialization and the existing
prohibition on ownership cycles still apply; slot publication does not introduce
thread synchronization or garbage collection.

C, Rust and Go clients verify alias visibility, independent slots, element and
resource lifetime, nil/empty values, non-UTF-8 bytes, validation failures and old
version negotiation. The C sanitizer client additionally verifies nested
replacement/growth and cleanup that releases the destination's external credit.
Rust also consumes the Go-native replacement client through its C-callable bridge.

This operation supports mutable package transport, but does not yet implement
mutable body-input adapters, live callback visibility, Rust default-array alias
partitioning, public record/interface contracts or complete SDK migration. The
existing native `borrowed` array provider view remains read-only. Those higher
level contracts are required work for the completion goal.

## ABI 1.4 managed-array mutation

Query `SN_ABI_V1_4_VERSION` with `SN_ABI_CAP_ARRAY_MUTATION` (`128`) before using
the new entry points. ABI 1.4 advertises `255`; ABI 1.0/1.1/1.2/1.3 retain
their exact `7`/`31`/`63`/`127` capability masks and reject the new capability
without changing output fields. Existing public layouts and operations remain
compatible. This runtime capability does not enable a new package manifest
contract by itself.

| Operation | Ownership and mutation |
| --- | --- |
| `value_array_insert(array, index, element)` | Acquire one element credit and insert a slot; nil elements are allowed |
| `value_array_take(array, index, out)` | Remove the slot and transfer its owned element credit to `out` |
| `value_array_pop(array, out)` | Take the final slot; empty/nil arrays return out-of-range |
| `value_array_remove(array, index)` | Remove the slot, then release its detached credit |
| `value_array_clear(array)` | Publish an empty array, then release detached slots |
| `value_array_reverse(array)` | Reverse the slots without changing element credits |

All entry-point names have the `sn_abi_v1_` prefix. Mutation preserves the array
handle, so retained aliases observe current slots. Independent array copies
retain their own element credits. Taken or popped elements survive releasing
the array; the caller releases the transferred credit. A successful nil-element
take writes NULL, which owns no credit.

Indices are unsigned, zero-based; insertion permits `index == length`, while
take/remove require an existing slot. Adapters implement language-specific
negative-index rules before transport. Wrong-kind, invalid-output and bounds
errors preserve contents and outputs. Nil insert/clear/reverse are invalid;
nil take/pop/remove are out-of-range. Read-only array views and the caller's
serialization responsibility remain unchanged.

Remove and clear publish their new slot state before resource destruction.
Destructors may inspect, grow, clear, reverse or replace that state. An operation
credit protects the array even when cleanup consumes the caller's external
credit. Nested changes survive the outer operation; no relocated slot pointer
is used after reentry. Ownership cycles remain forbidden.

C/Rust/Go and Rust-consuming-Go clients exercise nil/empty/non-UTF-8 values,
aliases, independent copies, error preservation, resource identity, transferred
element lifetimes and reentry after consuming an external credit. C sanitizer
controls additionally cover slot relocation and nested clear/reverse/insert.
Complete staged and hosted validation is required before acceptance of this
increment. Mutable body-input adapters, live foreign callback visibility,
Rust parameter alias/rebind semantics and wider SDK/package contracts remain work.

## ABI 1.5 canonical native string-array views

Query `SN_ABI_V1_5_VERSION` with `SN_ABI_CAP_NATIVE_STRING_ARRAYS` (`256`).
The new version advertises mask `511`; versions 1.0 through 1.4 retain their
exact masks `7`, `31`, `63`, `127` and `255`. Public value/info layouts and earlier
entry points remain compatible. This capability is a runtime prerequisite for
generated mutable body-input adapters. Native package ABI 1.5 now selects these
views for C body and C/Rust/Go native string-array inputs; owned array results
retain generic managed-value transport. Rust Sindarin body inputs now use the
same views through canonical array codecs and C support bundles.

These values wrap the actual canonical C `SnArray` header, whose slots contain
`char *`. They are distinct from ABI 1.1 arrays of `SnAbiValue *` and reject each
other's accessors. The public ABI header forward-declares the C array type;
generated C interop includes `sn_array.h`. Rust/Go clients use opaque pointers and
C accessors, never a vector/slice cast or an assumed foreign element layout.

| Operation (with `sn_abi_v1_` prefix) | Ownership |
| --- | --- |
| `native_string_array_borrow(array, out)` | One owned view credit; native header/storage ownership stays with its caller |
| `native_string_array_adopt(array, out)` | Transfer one owned native header only on success |
| `native_string_array_data(value, out)` | Borrow the same canonical header while the view credit is live |
| `native_string_array_copy(value, out)` | One owned view of a distinct canonical C array copied with its original element hooks |

Borrowing and retaining a borrowed view do **not** extend the native owner's
lifetime. That owner must outlive every view credit, including any retained by a
callback or other backing implementation. Releasing the last borrowed view frees
only its wrapper. Adoption retains the original allocator, element release/copy
hooks and tag, and final release runs canonical C array cleanup. Copies follow
the actual C hooks; absent copy/release hooks do not invent element ownership.
Borrowed elements must keep their existing external owners alive.

Mutation through the native header is visible immediately through aliases and
reentrant callbacks, including growth that relocates slot storage. Callers reload
data and length after each mutation and serialize access. Operation adapters must
hold their own view credit when a callback may consume the caller's credit. No
Rust shared-header `Sync` guarantee or Go pointer retention is introduced.

Nil constructors/access/copies remain nil. Non-nil headers must have pointer-sized
slots, the string tag (or an empty untyped tag), valid nonnegative length/capacity
and backing data whenever capacity is nonzero. Invalid layout, kind, output or
allocation range preserves outputs and ownership. Empty headers with no backing
allocation are valid; canonical zero-element copies skip `memcpy` to avoid its
nonnull precondition. The regression reproduces the earlier UBSan failure.

Ten C/Rust/Go and Rust-consuming-Go archive clients exercise live alias identity,
independent native copies, nil/empty/non-UTF-8 strings, lifetime beyond original
array cleanup, retained copy/release hooks, relocation and callbacks consuming
external credits. Four C clients instrument the actual runtime with strict
address/undefined/leak checks. Generated C/RS body and C/Rust/Go native input adapters now use these views.
Reference-qualified rebinding and wider record/interface
and SDK migration remain required work.

## ABI 1.6 canonical native byte-array views

Query `SN_ABI_V1_6_VERSION` with `SN_ABI_CAP_NATIVE_BYTE_ARRAYS` (`512`).
ABI 1.6 advertises mask `1023`; earlier versions retain their exact masks,
including ABI 1.5's `511`. Public info/value layouts and existing entry points
remain compatible. Package byte-array adapters and SDK migration must consume
this runtime capability. Native package ABI 1.6 now generates byte-array
consumers/providers for C/Rust applications and C/Rust/Go backing functions.

The seven `sn_abi_v1_native_byte_array_*` functions expose actual canonical C
`SnArray` storage with one-byte unsigned elements. `borrow`, `adopt`, `data` and
`copy` follow the string-view ownership contract. `bytes` borrows binary data and
length; reload both after mutation or growth. `copy_bytes` copies caller bytes
into an owned C array, retaining no Rust/Go pointer. NULL with zero length is nil;
a non-NULL pointer with zero length creates a non-nil empty array. Copies preserve
embedded NUL, bytes above 127, native element-copy/release hooks and independent
storage. A copy operation holds its own view credit through reentrant copy hooks.

`take` consumes a unique adopted/copied view and transfers its actual owned
header, clearing the wire handle. Generated consumers use this transfer for
owned results, retaining pointer identity and hooks. Borrowed views and retained
aliases reject transfer with `INVALID_ARGUMENT`, preserving the handle and output.
The caller serializes ownership transfer with access and credit acquisition.

Generated providers keep borrowed input view credits alive through callbacks
and failures. Native Rust/Go functions receive opaque canonical C headers for
byte inputs. Their owned vectors/slices are copied into C arrays; they are never
cast to foreign headers. Rust Sindarin bodies use canonical array codecs and C
support bundles, including mixed string/byte input guards. Their owned byte
results transfer canonical storage directly. Owned results must provide one
unique adopted wire credit; retained/borrowed result views are not valid for the
current byte-result contract. String-array result transport is unchanged.

These views are distinct from buffer values, plain wire arrays and string-array
views. Every accessor/copy validates the current native layout and byte tag,
rejecting wrong widths/tags, negative lengths, invalid capacities and missing
storage while leaving outputs unchanged. An empty untagged header is accepted
without rewriting it. Adoption transfers ownership only on success. A borrowed
view's native owner must outlive all its retained credits; mutable storage access
remains caller-serialized.

Twelve C/Rust/Go clients, including Rust consuming an original Go native archive,
cover both native view kinds. Five C clients instrument the actual C runtime with
address/undefined/leak checks. Byte tests exercise pointer identity, independent
copies, retained hooks, callback-visible growth and final cleanup. Reentrant
callbacks and copy hooks consume caller credits while operation guards keep
owned headers alive. The same guard repairs an ASAN-reproduced lifetime defect
in ABI 1.5 string-array copying. Removing the byte-copy guard in a private
negative control reproduces an ASAN heap-use-after-free. Rust vectors and Go slices are copied into
C storage rather than cast to a foreign array layout.

## ABI 1.7 typed canonical arrays

Query `SN_ABI_V1_7_VERSION` with `SN_ABI_CAP_NATIVE_ARRAYS` (`1024`). Its mask
is `2047`; exact earlier masks and existing functions remain compatible.
`SnAbiNativeArrayType` contains two uint32 fields, `leaf_kind` and `rank`.
Rank 1..32 counts array layers. Leaf IDs 1..10 identify int64 (also long), int32,
uint64, uint32, byte, bool, float32, float64, char8 and C string pointers.
Bool slots are one byte containing 0 or 1. Nested slots contain `SnArray *`.
Record/interface leaves require further contracts.

The six `sn_abi_v1_native_array_*` operations borrow, adopt, inspect data/type,
copy and transfer unique owned storage. Borrow/adopt/data/take preserve actual
headers and hooks. Nil arrays and nil children are valid. Invalid leaf/rank,
width/tag, length/capacity, missing storage, boolean values and overflow reject
without changing outputs or ownership. Descending rank rejects cycles. Each
access validates the current storage recursively. Empty untagged headers are
valid. Nonempty untagged int32, uint32, uint64 and float32 leaves are also valid:
existing C interop literals use that representation. Other nonempty leaves
require their canonical tags; the operation never rewrites the source tag.

Copy holds an operation credit through reentrant hooks. Custom element copy and
cleanup hooks survive. Missing string or nested copy hooks produce independent
canonical copies, installing default copy and (if absent) cleanup hooks on the
new headers so subsequent ordinary C copies are safe. Existing cleanup hooks
remain authoritative and must accept that canonical copied representation.
Custom copy hooks must keep source storage stable during the operation; they may
consume the caller's wire credits. Borrowed native owners must outlive every
view credit. Callers serialize mutation and transfer, and reload pointers after
mutation. Unique owned take follows ABI 1.6: borrowed/aliased handles reject,
success clears the wire credit and transfers the unchanged header.

Fifteen C/Rust/Go clients exercise the shared runtime, including nested live
aliases, callbacks, reentrant credit consumption, GC, missing hooks, ordinary
copies after original cleanup and actual SDK Environment rows. Seven C clients
run with address/undefined/leak checking. Generic array package generation in
this batch supports C native and C Sindarin bodies; Rust/Go native providers and
Rust generic body exports still require canonical guards and result contracts.
