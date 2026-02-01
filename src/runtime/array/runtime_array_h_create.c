/* ============================================================================
 * Handle-Based Array Functions Implementation - Create
 * ============================================================================
 * RtHandle-returning variants of array creation functions.
 * Array handle layout: [RtArrayMetadata][element data...]
 * ============================================================================ */

/* Common array creation: allocates [metadata][data], copies data in */
static RtHandle array_create_h(RtManagedArena *arena, size_t count, size_t elem_size, const void *data) {
    size_t alloc_size = sizeof(RtArrayMetadata) + count * elem_size;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    void *arr_data = (char *)raw + sizeof(RtArrayMetadata);
    if (data && count > 0) {
        memcpy(arr_data, data, count * elem_size);
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Create Functions
 * ============================================================================ */

#define DEFINE_ARRAY_CREATE_H(suffix, elem_type)                                \
RtHandle rt_array_create_##suffix##_h(RtManagedArena *arena, size_t count,      \
                                       const elem_type *data) {                 \
    return array_create_h(arena, count, sizeof(elem_type), data);               \
}

DEFINE_ARRAY_CREATE_H(long, long long)
DEFINE_ARRAY_CREATE_H(double, double)
DEFINE_ARRAY_CREATE_H(char, char)
DEFINE_ARRAY_CREATE_H(bool, int)
DEFINE_ARRAY_CREATE_H(byte, unsigned char)
DEFINE_ARRAY_CREATE_H(int32, int32_t)
DEFINE_ARRAY_CREATE_H(uint32, uint32_t)
DEFINE_ARRAY_CREATE_H(uint, uint64_t)
DEFINE_ARRAY_CREATE_H(float, float)

/* String create: converts char* pointers to RtHandle elements */
RtHandle rt_array_create_string_h(RtManagedArena *arena, size_t count, const char **data) {
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        arr[i] = rt_managed_strdup(arena, RT_HANDLE_NULL, data[i] ? data[i] : "");
    }
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_array_create_generic_h(RtManagedArena *arena, size_t count, size_t elem_size, const void *data) {
    return array_create_h(arena, count, elem_size, data);
}

/* Pointer (nested array) create -- elements are RtHandle (4 bytes) */
RtHandle rt_array_create_ptr_h(RtManagedArena *arena, size_t count, void **data) {
    return array_create_h(arena, count, sizeof(RtHandle), data);
}
