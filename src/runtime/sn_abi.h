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
#define SN_ABI_CAP_VALUES UINT64_C(1)
#define SN_ABI_CAP_POD_ARRAYS UINT64_C(2)
#define SN_ABI_CAP_RESOURCES UINT64_C(4)

typedef uint32_t SnAbiStatus;
#define SN_ABI_OK UINT32_C(0)
#define SN_ABI_INVALID_ARGUMENT UINT32_C(1)
#define SN_ABI_VERSION_MISMATCH UINT32_C(2)
#define SN_ABI_UNSUPPORTED UINT32_C(3)
#define SN_ABI_WRONG_KIND UINT32_C(4)
#define SN_ABI_OUT_OF_RANGE UINT32_C(5)
#define SN_ABI_FOREIGN_ERROR UINT32_C(6)

typedef struct SnAbiValue SnAbiValue;

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

#ifdef __cplusplus
}
#endif
#endif
