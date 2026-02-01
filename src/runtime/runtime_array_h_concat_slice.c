/* ============================================================================
 * Handle-Based Array Functions Implementation - Concat and Slice
 * ============================================================================
 * Concat: Source arrays are raw pointer arrays with metadata at [-1].
 * Slice: Source is raw pointer array. start/end/step use LONG_MIN as sentinel.
 * ============================================================================ */

#define DEFINE_ARRAY_CONCAT_H(suffix, elem_type)                                \
RtHandle rt_array_concat_##suffix##_h(RtManagedArena *arena, RtHandle old,      \
                                       const elem_type *a, const elem_type *b) {\
    size_t len_a = a ? ((RtArrayMetadata *)a)[-1].size : 0;                     \
    size_t len_b = b ? ((RtArrayMetadata *)b)[-1].size : 0;                     \
    size_t total = len_a + len_b;                                               \
    size_t capacity = total > 4 ? total : 4;                                    \
    size_t alloc_size = sizeof(RtArrayMetadata) + capacity * sizeof(elem_type); \
    RtHandle h = rt_managed_alloc(arena, old, alloc_size);                      \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = total;                                                         \
    meta->capacity = capacity;                                                  \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    if (len_a > 0) {                                                            \
        memcpy(dst, a, len_a * sizeof(elem_type));                              \
    }                                                                           \
    if (len_b > 0) {                                                            \
        memcpy(dst + len_a, b, len_b * sizeof(elem_type));                      \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_CONCAT_H(long, long long)
DEFINE_ARRAY_CONCAT_H(double, double)
DEFINE_ARRAY_CONCAT_H(char, char)
DEFINE_ARRAY_CONCAT_H(bool, int)
DEFINE_ARRAY_CONCAT_H(byte, unsigned char)
DEFINE_ARRAY_CONCAT_H(int32, int32_t)
DEFINE_ARRAY_CONCAT_H(uint32, uint32_t)
DEFINE_ARRAY_CONCAT_H(uint, uint64_t)
DEFINE_ARRAY_CONCAT_H(float, float)

/* String concat -- copies RtHandle elements from both sources */
RtHandle rt_array_concat_string_h(RtManagedArena *arena, RtHandle old,
                                   const char **a, const char **b) {
    size_t len_a = a ? ((RtArrayMetadata *)a)[-1].size : 0;
    size_t len_b = b ? ((RtArrayMetadata *)b)[-1].size : 0;
    size_t total = len_a + len_b;
    size_t capacity = total > 4 ? total : 4;
    size_t alloc_size = sizeof(RtArrayMetadata) + capacity * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, old, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = total;
    meta->capacity = capacity;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (len_a > 0) {
        memcpy(dst, a, len_a * sizeof(RtHandle));
    }
    if (len_b > 0) {
        memcpy(dst + len_a, b, len_b * sizeof(RtHandle));
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* Pointer concat -- copies RtHandle elements (nested arrays) */
RtHandle rt_array_concat_ptr_h(RtManagedArena *arena, RtHandle old,
                                void **a, void **b) {
    size_t len_a = a ? ((RtArrayMetadata *)a)[-1].size : 0;
    size_t len_b = b ? ((RtArrayMetadata *)b)[-1].size : 0;
    size_t total = len_a + len_b;
    size_t capacity = total > 4 ? total : 4;
    size_t alloc_size = sizeof(RtArrayMetadata) + capacity * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, old, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = total;
    meta->capacity = capacity;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (len_a > 0) {
        memcpy(dst, a, len_a * sizeof(RtHandle));
    }
    if (len_b > 0) {
        memcpy(dst + len_a, b, len_b * sizeof(RtHandle));
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Slice Functions
 * ============================================================================ */

#define DEFINE_ARRAY_SLICE_H(suffix, elem_type)                                 \
RtHandle rt_array_slice_##suffix##_h(RtManagedArena *arena, const elem_type *arr, \
                                      long start, long end, long step) {        \
    if (arr == NULL) {                                                          \
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
    size_t len = ((RtArrayMetadata *)arr)[-1].size;                              \
                                                                                \
    /* Handle step: LONG_MIN means step of 1 */                                  \
    long actual_step = (step == LONG_MIN) ? 1 : step;                            \
    if (actual_step <= 0) {                                                      \
        fprintf(stderr, "rt_array_slice_" #suffix "_h: step must be positive\n");\
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
                                                                                \
    /* Handle start */                                                           \
    long actual_start;                                                           \
    if (start == LONG_MIN) {                                                     \
        actual_start = 0;                                                        \
    } else if (start < 0) {                                                      \
        actual_start = (long)len + start;                                        \
        if (actual_start < 0) actual_start = 0;                                  \
    } else {                                                                     \
        actual_start = start;                                                    \
    }                                                                           \
                                                                                \
    /* Handle end */                                                              \
    long actual_end;                                                             \
    if (end == LONG_MIN) {                                                       \
        actual_end = (long)len;                                                  \
    } else if (end < 0) {                                                        \
        actual_end = (long)len + end;                                            \
        if (actual_end < 0) actual_end = 0;                                      \
    } else {                                                                     \
        actual_end = end;                                                        \
        if (actual_end > (long)len) actual_end = (long)len;                      \
    }                                                                           \
                                                                                \
    /* Clamp start */                                                             \
    if (actual_start >= (long)len || actual_start >= actual_end) {               \
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
                                                                                \
    /* Count elements */                                                          \
    size_t count = 0;                                                            \
    for (long i = actual_start; i < actual_end; i += actual_step) {              \
        count++;                                                                 \
    }                                                                           \
                                                                                \
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(elem_type);    \
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = count;                                                         \
    meta->capacity = count;                                                     \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    size_t idx = 0;                                                              \
    for (long i = actual_start; i < actual_end; i += actual_step) {              \
        dst[idx++] = arr[i];                                                     \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_SLICE_H(long, long long)
DEFINE_ARRAY_SLICE_H(double, double)
DEFINE_ARRAY_SLICE_H(char, char)
DEFINE_ARRAY_SLICE_H(bool, int)
DEFINE_ARRAY_SLICE_H(byte, unsigned char)
DEFINE_ARRAY_SLICE_H(int32, int32_t)
DEFINE_ARRAY_SLICE_H(uint32, uint32_t)
DEFINE_ARRAY_SLICE_H(uint, uint64_t)
DEFINE_ARRAY_SLICE_H(float, float)

/* String slice -- copies RtHandle elements from source */
RtHandle rt_array_slice_string_h(RtManagedArena *arena, const char **arr,
                                  long start, long end, long step) {
    if (arr == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t len = ((RtArrayMetadata *)arr)[-1].size;

    long actual_step = (step == LONG_MIN) ? 1 : step;
    if (actual_step <= 0) {
        fprintf(stderr, "rt_array_slice_string_h: step must be positive\n");
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }

    long actual_start;
    if (start == LONG_MIN) {
        actual_start = 0;
    } else if (start < 0) {
        actual_start = (long)len + start;
        if (actual_start < 0) actual_start = 0;
    } else {
        actual_start = start;
    }

    long actual_end;
    if (end == LONG_MIN) {
        actual_end = (long)len;
    } else if (end < 0) {
        actual_end = (long)len + end;
        if (actual_end < 0) actual_end = 0;
    } else {
        actual_end = end;
        if (actual_end > (long)len) actual_end = (long)len;
    }

    if (actual_start >= (long)len || actual_start >= actual_end) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }

    size_t count = 0;
    for (long i = actual_start; i < actual_end; i += actual_step) {
        count++;
    }

    const RtHandle *src = (const RtHandle *)arr;
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    size_t idx = 0;
    for (long i = actual_start; i < actual_end; i += actual_step) {
        dst[idx++] = src[i];
    }
    rt_managed_unpin(arena, h);
    return h;
}
