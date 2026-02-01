/* ============================================================================
 * Handle-Based Array Functions Implementation - Push Copy, Alloc, Range, Utils
 * ============================================================================
 * Push Copy: Non-mutating push that creates new array with element appended.
 * Alloc: Creates new array filled with default value.
 * Range: Creates long long array from start to end-1.
 * ============================================================================ */

#define DEFINE_ARRAY_PUSH_COPY_H(suffix, elem_type)                             \
RtHandle rt_array_push_copy_##suffix##_h(RtManagedArena *arena,                 \
                                          const elem_type *arr, elem_type elem) {\
    size_t count = arr ? ((RtArrayMetadata *)arr)[-1].size : 0;                  \
    size_t new_count = count + 1;                                               \
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(elem_type);\
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = new_count;                                                     \
    meta->capacity = new_count;                                                 \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    if (count > 0 && arr) {                                                      \
        memcpy(dst, arr, count * sizeof(elem_type));                            \
    }                                                                           \
    dst[count] = elem;                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_PUSH_COPY_H(long, long long)
DEFINE_ARRAY_PUSH_COPY_H(double, double)
DEFINE_ARRAY_PUSH_COPY_H(char, char)
DEFINE_ARRAY_PUSH_COPY_H(bool, int)
DEFINE_ARRAY_PUSH_COPY_H(byte, unsigned char)
DEFINE_ARRAY_PUSH_COPY_H(int32, int32_t)
DEFINE_ARRAY_PUSH_COPY_H(uint32, uint32_t)
DEFINE_ARRAY_PUSH_COPY_H(uint, uint64_t)
DEFINE_ARRAY_PUSH_COPY_H(float, float)

/* String push copy -- stores element as RtHandle */
RtHandle rt_array_push_copy_string_h(RtManagedArena *arena, const char **arr,
                                      const char *elem) {
    const RtHandle *src = (const RtHandle *)arr;
    size_t count = arr ? ((RtArrayMetadata *)arr)[-1].size : 0;
    size_t new_count = count + 1;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = new_count;
    meta->capacity = new_count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (count > 0 && src) {
        memcpy(dst, src, count * sizeof(RtHandle));
    }
    dst[count] = rt_managed_strdup(arena, RT_HANDLE_NULL, elem ? elem : "");
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Alloc Functions (with default value)
 * ============================================================================ */

#define DEFINE_ARRAY_ALLOC_H(suffix, elem_type)                                 \
RtHandle rt_array_alloc_##suffix##_h(RtManagedArena *arena, size_t count,       \
                                      elem_type default_value) {                \
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(elem_type);    \
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = count;                                                         \
    meta->capacity = count;                                                     \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    for (size_t i = 0; i < count; i++) {                                        \
        dst[i] = default_value;                                                  \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_ALLOC_H(long, long long)
DEFINE_ARRAY_ALLOC_H(double, double)
DEFINE_ARRAY_ALLOC_H(char, char)
DEFINE_ARRAY_ALLOC_H(bool, int)
DEFINE_ARRAY_ALLOC_H(byte, unsigned char)
DEFINE_ARRAY_ALLOC_H(int32, int32_t)
DEFINE_ARRAY_ALLOC_H(uint32, uint32_t)
DEFINE_ARRAY_ALLOC_H(uint, uint64_t)
DEFINE_ARRAY_ALLOC_H(float, float)

/* String alloc -- fills with RtHandle copies of the default string */
RtHandle rt_array_alloc_string_h(RtManagedArena *arena, size_t count,
                                  const char *default_value) {
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    RtHandle default_h = rt_managed_strdup(arena, RT_HANDLE_NULL, default_value ? default_value : "");
    for (size_t i = 0; i < count; i++) {
        dst[i] = default_h;
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Range Function
 * ============================================================================ */

RtHandle rt_array_range_h(RtManagedArena *arena, long long start, long long end) {
    size_t count = 0;
    if (end > start) {
        count = (size_t)(end - start);
    }
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(long long);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    long long *dst = (long long *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        dst[i] = start + (long long)i;
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Legacy Bridge Functions
 * ============================================================================ */

RtHandle rt_array_from_raw_strings_h(RtManagedArena *arena, RtHandle old, const char **src) {
    if (src == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t count = ((RtArrayMetadata *)src)[-1].size;
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, old, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        dst[i] = rt_managed_strdup(arena, RT_HANDLE_NULL, src[i] ? src[i] : "");
    }
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_args_create_h(RtManagedArena *arena, int argc, char **argv) {
    size_t count = argc > 0 ? (size_t)argc : 0;
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        dst[i] = rt_managed_strdup(arena, RT_HANDLE_NULL, argv[i]);
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* String array equality */
int rt_array_eq_string_h(RtManagedArena *arena, RtHandle a_h, RtHandle b_h)
{
    if (a_h == RT_HANDLE_NULL && b_h == RT_HANDLE_NULL) return 1;
    if (a_h == RT_HANDLE_NULL || b_h == RT_HANDLE_NULL) return 0;

    RtHandle *a = (RtHandle *)rt_managed_pin_array(arena, a_h);
    RtHandle *b = (RtHandle *)rt_managed_pin_array(arena, b_h);

    size_t len_a = rt_array_length(a);
    size_t len_b = rt_array_length(b);
    if (len_a != len_b) return 0;

    for (size_t i = 0; i < len_a; i++) {
        if (a[i] == RT_HANDLE_NULL && b[i] == RT_HANDLE_NULL) continue;
        if (a[i] == RT_HANDLE_NULL || b[i] == RT_HANDLE_NULL) return 0;
        const char *sa = (const char *)rt_managed_pin(arena, a[i]);
        const char *sb = (const char *)rt_managed_pin(arena, b[i]);
        if (strcmp(sa, sb) != 0) return 0;
    }
    return 1;
}
