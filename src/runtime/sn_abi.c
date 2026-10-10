#include "sn_abi.h"
#include "sn_array.h"
#include "sn_string.h"
#include <stdatomic.h>

_Static_assert(CHAR_BIT == 8 && sizeof(long long) == 8, "ABI v1 requires 8-bit bytes and 64-bit int");
_Static_assert(sizeof(float) == 4 && sizeof(double) == 8, "ABI v1 requires 32/64-bit float widths");

enum SnAbiKind { ABI_STRING, ABI_BUFFER, ABI_ARRAY, ABI_RESOURCE, ABI_VALUE_ARRAY, ABI_NATIVE_STRING_ARRAY };

struct SnAbiValue {
    atomic_size_t credits;
    enum SnAbiKind kind;
    union {
        struct { uint8_t *data; uint64_t length; } bytes;
        SnArray *array;
        struct { SnArray *array; bool owned; } native_array;
        struct { void *data; SnAbiDestroy destroy; uintptr_t context; char *type_identity; } resource;
    } payload;
};

static SnAbiValue *new_value(enum SnAbiKind kind)
{
    SnAbiValue *value = sn_calloc(1, sizeof(*value));
    atomic_init(&value->credits, 1);
    value->kind = kind;
    return value;
}

SnAbiStatus sn_abi_v1_query(uint32_t version, uint64_t required_capabilities,
                          SnAbiInfo *out, uint32_t out_size)
{
    uint64_t capabilities = SN_ABI_CAP_VALUES | SN_ABI_CAP_POD_ARRAYS | SN_ABI_CAP_RESOURCES;
    if (!out || out_size < sizeof(*out)) return SN_ABI_INVALID_ARGUMENT;
    if (version != SN_ABI_V1_VERSION && version != SN_ABI_V1_1_VERSION &&
        version != SN_ABI_V1_2_VERSION && version != SN_ABI_V1_3_VERSION &&
        version != SN_ABI_V1_4_VERSION && version != SN_ABI_V1_5_VERSION) return SN_ABI_VERSION_MISMATCH;
    if (version != SN_ABI_V1_VERSION) capabilities |= SN_ABI_CAP_VALUE_ARRAYS | SN_ABI_CAP_TYPED_RESOURCES;
    if (version >= SN_ABI_V1_2_VERSION)
        capabilities |= SN_ABI_CAP_PACKAGE_LIFECYCLE;
    if (version >= SN_ABI_V1_3_VERSION) capabilities |= SN_ABI_CAP_ARRAY_REPLACEMENT;
    if (version >= SN_ABI_V1_4_VERSION) capabilities |= SN_ABI_CAP_ARRAY_MUTATION;
    if (version >= SN_ABI_V1_5_VERSION) capabilities |= SN_ABI_CAP_NATIVE_STRING_ARRAYS;
    if (required_capabilities & ~capabilities) return SN_ABI_UNSUPPORTED;
    SnAbiInfo info = { version, sizeof(void *) * CHAR_BIT, capabilities,
                       sizeof(long long) * CHAR_BIT, CHAR_BIT,
                       sizeof(float) * CHAR_BIT, sizeof(double) * CHAR_BIT };
    *out = info;
    return SN_ABI_OK;
}

const char *sn_abi_v1_status_message(SnAbiStatus status)
{
    switch (status) {
        case SN_ABI_OK: return "success";
        case SN_ABI_INVALID_ARGUMENT: return "invalid ABI argument";
        case SN_ABI_VERSION_MISMATCH: return "runtime ABI version mismatch";
        case SN_ABI_UNSUPPORTED: return "unsupported runtime ABI capability";
        case SN_ABI_WRONG_KIND: return "incorrect ABI value kind";
        case SN_ABI_OUT_OF_RANGE: return "runtime ABI index or length out of range";
        case SN_ABI_FOREIGN_ERROR: return "native backing function panicked";
        case SN_ABI_PACKAGE_BUSY: return "package lifecycle cannot close an active reentrant call";
        case SN_ABI_PACKAGE_CLOSED: return "package lifecycle is closed";
        default: return "unknown runtime ABI status";
    }
}

SnAbiValue *sn_abi_v1_retain(SnAbiValue *value)
{
    if (value) {
        /* A caller holds a live credit, so release cannot race this acquisition
         * to zero. Overflow is a fatal contract failure, never wrapped to zero. */
        size_t credits = atomic_load_explicit(&value->credits, memory_order_relaxed);
        for (;;) {
            if (credits == SIZE_MAX) {
                fprintf(stderr, "fatal: runtime ABI reference count overflow\n");
                exit(1);
            }
            if (atomic_compare_exchange_weak_explicit(&value->credits, &credits, credits + 1,
                    memory_order_relaxed, memory_order_relaxed)) break;
        }
    }
    return value;
}

void sn_abi_v1_release(SnAbiValue *value)
{
    if (!value || atomic_fetch_sub_explicit(&value->credits, 1, memory_order_acq_rel) != 1) return;
    switch (value->kind) {
        case ABI_STRING:
        case ABI_BUFFER: free(value->payload.bytes.data); break;
        case ABI_ARRAY:
        case ABI_VALUE_ARRAY: sn_array_free(value->payload.array); break;
        case ABI_NATIVE_STRING_ARRAY:
            if (value->payload.native_array.owned) sn_array_free(value->payload.native_array.array);
            break;
        case ABI_RESOURCE:
            /* No runtime lock is held: resource callbacks may reenter. */
            if (value->payload.resource.destroy)
                value->payload.resource.destroy(value->payload.resource.data,
                                                  value->payload.resource.context);
            free(value->payload.resource.type_identity);
            break;
    }
    free(value);
}

static SnAbiStatus native_string_array_validate(const SnArray *array)
{
    if (!array) return SN_ABI_OK;
    if (array->elem_size != sizeof(char *) ||
        (array->elem_tag != SN_TAG_STRING &&
         !(array->elem_tag == SN_TAG_DEFAULT && array->len == 0))) return SN_ABI_WRONG_KIND;
    if (array->len < 0 || array->cap < array->len || array->cap < 0 ||
        (array->cap && !array->data)) return SN_ABI_INVALID_ARGUMENT;
    if ((uint64_t)array->cap > SIZE_MAX / sizeof(char *)) return SN_ABI_OUT_OF_RANGE;
    return SN_ABI_OK;
}

static SnAbiStatus native_string_array_wrap(SnArray *array, bool owned, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnAbiStatus status = native_string_array_validate(array);
    if (status) return status;
    if (!array) { *out = NULL; return SN_ABI_OK; }
    SnAbiValue *value = new_value(ABI_NATIVE_STRING_ARRAY);
    value->payload.native_array.array = array;
    value->payload.native_array.owned = owned;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_native_string_array_borrow(SnArray *array, SnAbiValue **out)
{
    return native_string_array_wrap(array, false, out);
}

SnAbiStatus sn_abi_v1_native_string_array_adopt(SnArray *array, SnAbiValue **out)
{
    return native_string_array_wrap(array, true, out);
}

SnAbiStatus sn_abi_v1_native_string_array_data(const SnAbiValue *value, SnArray **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (value && value->kind != ABI_NATIVE_STRING_ARRAY) return SN_ABI_WRONG_KIND;
    *out = value ? value->payload.native_array.array : NULL;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_native_string_array_copy(const SnAbiValue *value, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (value && value->kind != ABI_NATIVE_STRING_ARRAY) return SN_ABI_WRONG_KIND;
    SnArray *array = value ? value->payload.native_array.array : NULL;
    SnAbiStatus status = native_string_array_validate(array);
    if (status) return status;
    return native_string_array_wrap(sn_array_copy(array), true, out);
}

SnAbiStatus sn_abi_v1_string_copy(const char *text, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (!text) { *out = NULL; return SN_ABI_OK; }
    size_t length = strlen(text);
    SnAbiValue *value = new_value(ABI_STRING);
    value->payload.bytes.data = sn_malloc(length + 1);
    memcpy(value->payload.bytes.data, text, length + 1);
    value->payload.bytes.length = length;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_string_concat(const SnAbiValue *left, const SnAbiValue *right,
                                  SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if ((left && left->kind != ABI_STRING) || (right && right->kind != ABI_STRING))
        return SN_ABI_WRONG_KIND;
    uint64_t left_length = left ? left->payload.bytes.length : 0;
    uint64_t right_length = right ? right->payload.bytes.length : 0;
    if (left_length > SIZE_MAX - 1 || right_length > SIZE_MAX - 1 - left_length)
        return SN_ABI_OUT_OF_RANGE;
    SnAbiValue *value = new_value(ABI_STRING);
    value->payload.bytes.data = (uint8_t *)sn_str_concat(
        left ? (const char *)left->payload.bytes.data : NULL,
        right ? (const char *)right->payload.bytes.data : NULL);
    value->payload.bytes.length = left_length + right_length;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_buffer_copy(const uint8_t *data, uint64_t length, SnAbiValue **out)
{
    if (!out || (!data && length)) return SN_ABI_INVALID_ARGUMENT;
    if (length > SIZE_MAX) return SN_ABI_OUT_OF_RANGE;
    if (!data) { *out = NULL; return SN_ABI_OK; }
    SnAbiValue *value = new_value(ABI_BUFFER);
    value->payload.bytes.data = sn_malloc(length ? (size_t)length : 1);
    if (length) memcpy(value->payload.bytes.data, data, (size_t)length);
    value->payload.bytes.length = length;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_bytes(const SnAbiValue *value, SnAbiBytes *out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (!value) { SnAbiBytes nil = {NULL, 0}; *out = nil; return SN_ABI_OK; }
    if (value->kind != ABI_STRING && value->kind != ABI_BUFFER) return SN_ABI_WRONG_KIND;
    SnAbiBytes bytes = {value->payload.bytes.data, value->payload.bytes.length};
    *out = bytes;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_string_bytes(const SnAbiValue *value, SnAbiBytes *out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (value && value->kind != ABI_STRING) return SN_ABI_WRONG_KIND;
    return sn_abi_v1_bytes(value, out);
}

SnAbiStatus sn_abi_v1_array_new(uint64_t element_size, SnAbiValue **out)
{
    if (!out || !element_size) return SN_ABI_INVALID_ARGUMENT;
    if (element_size > SIZE_MAX / 4 || element_size > LLONG_MAX)
        return SN_ABI_OUT_OF_RANGE;
    SnAbiValue *value = new_value(ABI_ARRAY);
    value->payload.array = sn_array_new((size_t)element_size, 4);
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_copy(const SnAbiValue *array, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (!array) { *out = NULL; return SN_ABI_OK; }
    if (array->kind != ABI_ARRAY) return SN_ABI_WRONG_KIND;
    SnAbiValue *value = new_value(ABI_ARRAY);
    value->payload.array = sn_array_copy(array->payload.array);
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_length(const SnAbiValue *array, uint64_t *out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (array && array->kind != ABI_ARRAY) return SN_ABI_WRONG_KIND;
    *out = array ? (uint64_t)sn_array_length(array->payload.array) : 0;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_element_size(const SnAbiValue *array, uint64_t *out)
{
    if (!array || !out) return SN_ABI_INVALID_ARGUMENT;
    if (array->kind != ABI_ARRAY) return SN_ABI_WRONG_KIND;
    *out = array->payload.array->elem_size;
    return SN_ABI_OK;
}

static SnAbiStatus array_element(const SnAbiValue *array, const void *element, uint64_t size)
{
    if (!array || !element) return SN_ABI_INVALID_ARGUMENT;
    if (array->kind != ABI_ARRAY) return SN_ABI_WRONG_KIND;
    if (size != array->payload.array->elem_size) return SN_ABI_INVALID_ARGUMENT;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_push(SnAbiValue *array, const void *element, uint64_t size)
{
    SnAbiStatus status = array_element(array, element, size);
    if (status != SN_ABI_OK) return status;
    SnArray *storage = array->payload.array;
    if (storage->len >= LLONG_MAX) return SN_ABI_OUT_OF_RANGE;
    if (storage->len >= storage->cap &&
        ((uint64_t)storage->cap > (uint64_t)LLONG_MAX / 2 ||
         (uint64_t)storage->cap > SIZE_MAX / storage->elem_size / 2))
        return SN_ABI_OUT_OF_RANGE;
    sn_array_push(storage, element);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_get(const SnAbiValue *array, uint64_t index,
                              void *out, uint64_t size)
{
    SnAbiStatus status = array_element(array, out, size);
    if (status != SN_ABI_OK) return status;
    if (index >= (uint64_t)array->payload.array->len) return SN_ABI_OUT_OF_RANGE;
    memcpy(out, sn_array_get(array->payload.array, (long long)index), (size_t)size);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_array_set(SnAbiValue *array, uint64_t index,
                              const void *element, uint64_t size)
{
    SnAbiStatus status = array_element(array, element, size);
    if (status != SN_ABI_OK) return status;
    if (index >= (uint64_t)array->payload.array->len) return SN_ABI_OUT_OF_RANGE;
    memcpy(sn_array_get(array->payload.array, (long long)index), element, (size_t)size);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_resource_new(void *resource, SnAbiDestroy destroy,
                                 uintptr_t context, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *value = new_value(ABI_RESOURCE);
    value->payload.resource.data = resource;
    value->payload.resource.destroy = destroy;
    value->payload.resource.context = context;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_resource_data(const SnAbiValue *value, void **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (value && value->kind != ABI_RESOURCE) return SN_ABI_WRONG_KIND;
    *out = value ? value->payload.resource.data : NULL;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_resource_new_typed(const char *type_identity, void *resource,
                                       SnAbiDestroy destroy, uintptr_t context, SnAbiValue **out)
{
    if (!out || !type_identity || !type_identity[0]) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *value = new_value(ABI_RESOURCE);
    size_t length = strlen(type_identity);
    value->payload.resource.type_identity = sn_malloc(length + 1);
    memcpy(value->payload.resource.type_identity, type_identity, length + 1);
    value->payload.resource.data = resource;
    value->payload.resource.destroy = destroy;
    value->payload.resource.context = context;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_resource_data_typed(const SnAbiValue *value, const char *type_identity,
                                        void **out)
{
    if (!out || !type_identity || !type_identity[0]) return SN_ABI_INVALID_ARGUMENT;
    if (value && (value->kind != ABI_RESOURCE || !value->payload.resource.type_identity ||
        strcmp(value->payload.resource.type_identity, type_identity))) return SN_ABI_WRONG_KIND;
    *out = value ? value->payload.resource.data : NULL;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_resource_type(const SnAbiValue *value, const char **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (value && value->kind != ABI_RESOURCE) return SN_ABI_WRONG_KIND;
    *out = value ? value->payload.resource.type_identity : NULL;
    return SN_ABI_OK;
}

static void abi_value_slot_release(void *slot)
{
    sn_abi_v1_release(*(SnAbiValue **)slot);
}

static void abi_value_slot_copy(const void *source, void *destination)
{
    *(SnAbiValue **)destination = sn_abi_v1_retain(*(SnAbiValue *const *)source);
}

SnAbiStatus sn_abi_v1_value_array_new(SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *value = new_value(ABI_VALUE_ARRAY);
    value->payload.array = sn_array_new(sizeof(SnAbiValue *), 4);
    value->payload.array->elem_release = abi_value_slot_release;
    value->payload.array->elem_copy = abi_value_slot_copy;
    *out = value;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_copy(const SnAbiValue *array, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (!array) { *out = NULL; return SN_ABI_OK; }
    if (array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    SnAbiValue *copy = new_value(ABI_VALUE_ARRAY);
    copy->payload.array = sn_array_copy(array->payload.array);
    *out = copy;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_length(const SnAbiValue *array, uint64_t *out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (array && array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    *out = array ? (uint64_t)array->payload.array->len : 0;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_push(SnAbiValue *array, SnAbiValue *element)
{
    if (!array) return SN_ABI_INVALID_ARGUMENT;
    if (array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    SnArray *storage = array->payload.array;
    if (storage->len >= LLONG_MAX || (uint64_t)storage->len >= SIZE_MAX / sizeof(SnAbiValue *) / 2)
        return SN_ABI_OUT_OF_RANGE;
    SnAbiValue *owned = sn_abi_v1_retain(element);
    sn_array_push(storage, &owned);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_get(const SnAbiValue *array, uint64_t index, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (array && array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    if (!array || index >= (uint64_t)array->payload.array->len) return SN_ABI_OUT_OF_RANGE;
    *out = sn_abi_v1_retain(((SnAbiValue **)array->payload.array->data)[index]);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_set(SnAbiValue *array, uint64_t index, SnAbiValue *element)
{
    if (array && array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    if (!array || index >= (uint64_t)array->payload.array->len) return SN_ABI_OUT_OF_RANGE;
    SnAbiValue **slot = &((SnAbiValue **)array->payload.array->data)[index];
    SnAbiValue *previous = *slot;
    *slot = sn_abi_v1_retain(element);
    /* Publish before cleanup: a destructor may reenter this array and resize it.
     * No slot pointer is accessed after invoking the previous owner's cleanup. */
    sn_abi_v1_release(previous);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_assign(SnAbiValue *destination, const SnAbiValue *source)
{
    if (!destination) return SN_ABI_INVALID_ARGUMENT;
    if (destination->kind != ABI_VALUE_ARRAY || (source && source->kind != ABI_VALUE_ARRAY))
        return SN_ABI_WRONG_KIND;
    if (destination == source) return SN_ABI_OK;
    SnArray *replacement = source ? sn_array_copy(source->payload.array)
                                 : sn_array_new(sizeof(SnAbiValue *), 4);
    replacement->elem_release = abi_value_slot_release;
    replacement->elem_copy = abi_value_slot_copy;
    SnArray *previous = destination->payload.array;
    sn_abi_v1_retain(destination);
    destination->payload.array = replacement;
    /* Old element cleanup can replace/grow the destination again, or release
     * the caller's credit. No pointer into either published array is used after
     * reentry. The detached storage and operation credit remain ours. */
    sn_array_free(previous);
    sn_abi_v1_release(destination);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_insert(SnAbiValue *array, uint64_t index, SnAbiValue *element)
{
    if (!array) return SN_ABI_INVALID_ARGUMENT;
    if (array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    SnArray *storage = array->payload.array;
    if (index > (uint64_t)storage->len || storage->len >= LLONG_MAX ||
        (uint64_t)storage->len >= SIZE_MAX / sizeof(SnAbiValue *) / 2)
        return SN_ABI_OUT_OF_RANGE;
    SnAbiValue *owned = sn_abi_v1_retain(element);
    sn_array_insert(storage, &owned, (long long)index);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_take(SnAbiValue *array, uint64_t index, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (array && array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    if (!array || index >= (uint64_t)array->payload.array->len) return SN_ABI_OUT_OF_RANGE;
    SnArray *storage = array->payload.array;
    SnAbiValue **slots = storage->data;
    SnAbiValue *owned = slots[index];
    size_t remaining = (size_t)((uint64_t)storage->len - index - 1);
    memmove(slots + index, slots + index + 1, remaining * sizeof(*slots));
    slots[--storage->len] = NULL;
    *out = owned;
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_pop(SnAbiValue *array, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    if (array && array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    if (!array || array->payload.array->len == 0) return SN_ABI_OUT_OF_RANGE;
    return sn_abi_v1_value_array_take(array, (uint64_t)array->payload.array->len - 1, out);
}

SnAbiStatus sn_abi_v1_value_array_remove(SnAbiValue *array, uint64_t index)
{
    SnAbiValue *detached = NULL;
    SnAbiStatus status = sn_abi_v1_value_array_take(array, index, &detached);
    if (status != SN_ABI_OK) return status;
    sn_abi_v1_retain(array);
    sn_abi_v1_release(detached);
    sn_abi_v1_release(array);
    return SN_ABI_OK;
}

SnAbiStatus sn_abi_v1_value_array_clear(SnAbiValue *array)
{
    return sn_abi_v1_value_array_assign(array, NULL);
}

SnAbiStatus sn_abi_v1_value_array_reverse(SnAbiValue *array)
{
    if (!array) return SN_ABI_INVALID_ARGUMENT;
    if (array->kind != ABI_VALUE_ARRAY) return SN_ABI_WRONG_KIND;
    sn_array_reverse(array->payload.array);
    return SN_ABI_OK;
}
