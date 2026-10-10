#ifndef SN_ABI_H
#define SN_ABI_H

/* Shared C runtime ABI. Public layouts use fixed-width scalar fields; handles
 * are opaque. This header is usable from C99, C++, Rust FFI and Go cgo.
 * See docs/runtime-abi.md for ownership, bounds and threading contracts. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SN_ABI_V1_VERSION UINT32_C(0x00010000)
#define SN_ABI_V1_1_VERSION UINT32_C(0x00010001)
#define SN_ABI_V1_2_VERSION UINT32_C(0x00010002)
#define SN_ABI_V1_3_VERSION UINT32_C(0x00010003)
#define SN_ABI_V1_4_VERSION UINT32_C(0x00010004)
#define SN_ABI_V1_5_VERSION UINT32_C(0x00010005)
#define SN_ABI_CAP_VALUES UINT64_C(1)
#define SN_ABI_CAP_POD_ARRAYS UINT64_C(2)
#define SN_ABI_CAP_RESOURCES UINT64_C(4)
#define SN_ABI_CAP_VALUE_ARRAYS UINT64_C(8)
#define SN_ABI_CAP_TYPED_RESOURCES UINT64_C(16)
#define SN_ABI_CAP_PACKAGE_LIFECYCLE UINT64_C(32)
#define SN_ABI_CAP_ARRAY_REPLACEMENT UINT64_C(64)
#define SN_ABI_CAP_ARRAY_MUTATION UINT64_C(128)
#define SN_ABI_CAP_NATIVE_STRING_ARRAYS UINT64_C(256)

typedef uint32_t SnAbiStatus;
#define SN_ABI_OK UINT32_C(0)
#define SN_ABI_INVALID_ARGUMENT UINT32_C(1)
#define SN_ABI_VERSION_MISMATCH UINT32_C(2)
#define SN_ABI_UNSUPPORTED UINT32_C(3)
#define SN_ABI_WRONG_KIND UINT32_C(4)
#define SN_ABI_OUT_OF_RANGE UINT32_C(5)
#define SN_ABI_FOREIGN_ERROR UINT32_C(6)
#define SN_ABI_PACKAGE_BUSY UINT32_C(7)
#define SN_ABI_PACKAGE_CLOSED UINT32_C(8)

typedef struct SnAbiValue SnAbiValue;
struct SnArray;

typedef struct {
    uint32_t abi_version;
    uint32_t pointer_bits;
    uint64_t capabilities;
    uint32_t int_bits;
    uint32_t char_bits;
    uint32_t float_bits;
    uint32_t double_bits;
} SnAbiInfo;

typedef struct {
    const uint8_t *data;
    uint64_t length;
} SnAbiBytes;

/* Destroy is invoked exactly once after the last credit is released. Context
 * is a native-sized token, suitable for a Go cgo.Handle or adapter registry ID.
 * Resource may be NULL; no implicit free(resource) is performed. */
typedef void (*SnAbiDestroy)(void *resource, uintptr_t context);

SnAbiStatus sn_abi_v1_query(uint32_t version, uint64_t required_capabilities,
                          SnAbiInfo *out, uint32_t out_size);
const char *sn_abi_v1_status_message(SnAbiStatus status);

/* Retain/release are thread safe and nil safe. A live credit is required to
 * access a value or acquire another credit. Mutable payload access is serialized
 * by the caller; reference-count safety does not synchronize array contents. */
SnAbiValue *sn_abi_v1_retain(SnAbiValue *value);
void sn_abi_v1_release(SnAbiValue *value);

/* Constructors/copies return one owned credit. Outputs remain unchanged on
 * failure. A NULL C string or NULL zero-length buffer creates nil; non-NULL
 * zero-length input creates an empty value. Strings follow C strlen semantics;
 * buffers preserve binary bytes, including embedded NUL. */
SnAbiStatus sn_abi_v1_string_copy(const char *text, SnAbiValue **out);
SnAbiStatus sn_abi_v1_string_concat(const SnAbiValue *left, const SnAbiValue *right,
                                  SnAbiValue **out);
SnAbiStatus sn_abi_v1_buffer_copy(const uint8_t *data, uint64_t length, SnAbiValue **out);
/* Borrowed bytes remain valid while the value has a live credit. For strings,
 * data[length] is NUL; that terminator is not included in length. Nil yields
 * {NULL, 0}. Do not free the view or cast it to a Rust/Go owned representation. */
SnAbiStatus sn_abi_v1_bytes(const SnAbiValue *value, SnAbiBytes *out);
/* String-only borrowed view. Rejects buffers, which need not be terminated.
 * Outputs remain unchanged on failure; nil gives {NULL, 0}. */
SnAbiStatus sn_abi_v1_string_bytes(const SnAbiValue *value, SnAbiBytes *out);

/* Plain-value arrays use the existing C SnArray implementation. Element size
 * is an explicit wire-layout commitment. These operations do not provide managed
 * element ownership/copy hooks; adapters must not use them for owned pointers. */
SnAbiStatus sn_abi_v1_array_new(uint64_t element_size, SnAbiValue **out);
SnAbiStatus sn_abi_v1_array_copy(const SnAbiValue *array, SnAbiValue **out);
SnAbiStatus sn_abi_v1_array_length(const SnAbiValue *array, uint64_t *out);
SnAbiStatus sn_abi_v1_array_element_size(const SnAbiValue *array, uint64_t *out);
SnAbiStatus sn_abi_v1_array_push(SnAbiValue *array, const void *element, uint64_t size);
SnAbiStatus sn_abi_v1_array_get(const SnAbiValue *array, uint64_t index,
                              void *out, uint64_t size);
SnAbiStatus sn_abi_v1_array_set(SnAbiValue *array, uint64_t index,
                              const void *element, uint64_t size);
/* ABI 1.1 managed-value arrays. Push/set acquire a credit from a borrowed input;
 * get returns an owned credit. Array copy creates independent slots retaining
 * their elements, while retain aliases the same array. Caller serializes mutation
 * and avoids ownership cycles (this runtime uses reference counting). */
SnAbiStatus sn_abi_v1_value_array_new(SnAbiValue **out);
SnAbiStatus sn_abi_v1_value_array_copy(const SnAbiValue *array, SnAbiValue **out);
SnAbiStatus sn_abi_v1_value_array_length(const SnAbiValue *array, uint64_t *out);
SnAbiStatus sn_abi_v1_value_array_push(SnAbiValue *array, SnAbiValue *element);
SnAbiStatus sn_abi_v1_value_array_get(const SnAbiValue *array, uint64_t index, SnAbiValue **out);
SnAbiStatus sn_abi_v1_value_array_set(SnAbiValue *array, uint64_t index, SnAbiValue *element);
/* ABI 1.3 bulk slot replacement preserves destination identity. Source is
 * borrowed; retain its elements before publishing, then release the detached
 * old slots. Nil source clears to an empty destination. Nil destination is
 * invalid. Kind errors preserve destination contents. Self-assignment is a
 * no-op. Destructors may reenter and mutate the newly published array; an
 * operation credit keeps the destination alive through that cleanup. */
SnAbiStatus sn_abi_v1_value_array_assign(SnAbiValue *destination, const SnAbiValue *source);
/* ABI 1.4 mutation preserves the array handle and retained aliases. Insert
 * borrows/acquires an element credit; take/pop transfer a slot's owned credit.
 * Remove/clear publish their mutation before releasing detached elements, so
 * destructors can inspect or mutate the current array. An operation credit
 * protects the array through that cleanup. Indices are unsigned, zero-based;
 * adapters resolve language-specific negative indices before calling. Failed
 * operations preserve contents and output arguments. */
SnAbiStatus sn_abi_v1_value_array_insert(SnAbiValue *array, uint64_t index, SnAbiValue *element);
SnAbiStatus sn_abi_v1_value_array_take(SnAbiValue *array, uint64_t index, SnAbiValue **out);
SnAbiStatus sn_abi_v1_value_array_pop(SnAbiValue *array, SnAbiValue **out);
SnAbiStatus sn_abi_v1_value_array_remove(SnAbiValue *array, uint64_t index);
SnAbiStatus sn_abi_v1_value_array_clear(SnAbiValue *array);
SnAbiStatus sn_abi_v1_value_array_reverse(SnAbiValue *array);

/* ABI 1.5 typed views of canonical C string-array storage. These are distinct
 * from managed-value arrays: their slots contain C char*, never SnAbiValue*.
 * Borrow preserves the actual header, slots, allocation and copy/cleanup hooks;
 * its native owner must outlive EVERY retained view credit, including callbacks.
 * Adopt transfers an owned header only on success. Nil remains nil. Layout/tag
 * errors preserve outputs and ownership. Mutable access is caller-serialized.
 * The data accessor borrows the header; reload its data/length after mutation.
 * Copy follows the canonical C element-copy hooks and returns one owned header
 * credit. Adapters must preserve borrowed element lifetimes when hooks are nil.
 * Native pointers are for generated C interop; Rust/Go must not cast their own
 * vectors/slices to SnArray or assume that C slots contain language values. */
SnAbiStatus sn_abi_v1_native_string_array_borrow(struct SnArray *array, SnAbiValue **out);
SnAbiStatus sn_abi_v1_native_string_array_adopt(struct SnArray *array, SnAbiValue **out);
SnAbiStatus sn_abi_v1_native_string_array_data(const SnAbiValue *value, struct SnArray **out);
SnAbiStatus sn_abi_v1_native_string_array_copy(const SnAbiValue *value, SnAbiValue **out);

/* Resource adoption transfers cleanup responsibility only on success. Adapters
 * keep resource layouts private or expose documented fields through accessors. */
SnAbiStatus sn_abi_v1_resource_new(void *resource, SnAbiDestroy destroy,
                                 uintptr_t context, SnAbiValue **out);
SnAbiStatus sn_abi_v1_resource_data(const SnAbiValue *value, void **out);
/* Package/type/ABI identity is copied by the runtime. Adoption occurs only on
 * success. Typed access rejects untyped or differently tagged live resources;
 * nil remains nil-safe. Outputs remain unchanged on any error. */
SnAbiStatus sn_abi_v1_resource_new_typed(const char *type_identity, void *resource,
                                       SnAbiDestroy destroy, uintptr_t context, SnAbiValue **out);
SnAbiStatus sn_abi_v1_resource_data_typed(const SnAbiValue *value, const char *type_identity,
                                        void **out);
/* Borrowed identity view; valid while the resource has a live credit. Generic
 * resources and nil have no identity. Does not expose private resource layout. */
SnAbiStatus sn_abi_v1_resource_type(const SnAbiValue *value, const char **out);

/* ABI 1.2 package lifecycle. Initialization executes once without holding the
 * gate lock. A same-thread initializer may reenter; other threads wait. Init
 * failures are sticky and the initializer owns rollback of provisional state.
 * Call credits keep the package alive and must end on their creating thread.
 * Shutdown rejects an active same-thread call, drains other calls, and invokes
 * cleanup once outside the lock. Calls after shutdown preserve outputs/fail.
 * Callers own context and callbacks must not unwind across the C boundary. */
typedef struct SnAbiPackage SnAbiPackage;
typedef struct SnAbiPackageCall SnAbiPackageCall;
typedef SnAbiStatus (*SnAbiPackageInit)(SnAbiPackage *package, uintptr_t context);
typedef void (*SnAbiPackageCleanup)(SnAbiPackage *package, uintptr_t context);
SnAbiStatus sn_abi_v1_package_new(SnAbiPackageInit initialize, SnAbiPackageCleanup cleanup,
                                 uintptr_t context, SnAbiPackage **out);
SnAbiPackage *sn_abi_v1_package_retain(SnAbiPackage *package);
void sn_abi_v1_package_release(SnAbiPackage *package);
SnAbiStatus sn_abi_v1_package_begin(SnAbiPackage *package, SnAbiPackageCall **out);
void sn_abi_v1_package_end(SnAbiPackageCall *call);
SnAbiStatus sn_abi_v1_package_shutdown(SnAbiPackage *package);

#ifdef __cplusplus
}
#endif
#endif
