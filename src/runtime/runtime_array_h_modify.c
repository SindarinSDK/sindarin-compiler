/* ============================================================================
 * Handle-Based Array Functions Implementation - Modify (Rev, Rem, Ins)
 * ============================================================================
 * Rev: Creates new array with elements in reverse order.
 * Rem: Creates new array without element at given index.
 * Ins: Creates new array with element inserted at given index.
 * ============================================================================ */

#define DEFINE_ARRAY_REV_H(suffix, elem_type)                                   \
RtHandle rt_array_rev_##suffix##_h(RtManagedArena *arena, const elem_type *arr) {\
    if (arr == NULL) {                                                          \
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
    size_t count = ((RtArrayMetadata *)arr)[-1].size;                            \
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(elem_type);    \
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = count;                                                         \
    meta->capacity = count;                                                     \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    for (size_t i = 0; i < count; i++) {                                        \
        dst[i] = arr[count - 1 - i];                                            \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_REV_H(long, long long)
DEFINE_ARRAY_REV_H(double, double)
DEFINE_ARRAY_REV_H(char, char)
DEFINE_ARRAY_REV_H(bool, int)
DEFINE_ARRAY_REV_H(byte, unsigned char)
DEFINE_ARRAY_REV_H(int32, int32_t)
DEFINE_ARRAY_REV_H(uint32, uint32_t)
DEFINE_ARRAY_REV_H(uint, uint64_t)
DEFINE_ARRAY_REV_H(float, float)

/* String reverse -- copies RtHandle elements in reverse order */
RtHandle rt_array_rev_string_h(RtManagedArena *arena, const char **arr) {
    if (arr == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    const RtHandle *src = (const RtHandle *)arr;
    size_t count = ((RtArrayMetadata *)arr)[-1].size;
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        dst[i] = src[count - 1 - i];
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================ */

#define DEFINE_ARRAY_REM_H(suffix, elem_type)                                   \
RtHandle rt_array_rem_##suffix##_h(RtManagedArena *arena, const elem_type *arr, \
                                    long index) {                               \
    if (arr == NULL) {                                                          \
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
    size_t count = ((RtArrayMetadata *)arr)[-1].size;                            \
    /* Handle negative index */                                                  \
    long actual_index = index < 0 ? (long)count + index : index;                \
    if (actual_index < 0 || (size_t)actual_index >= count) {                    \
        fprintf(stderr, "rt_array_rem_" #suffix "_h: index out of bounds\n");   \
        exit(1);                                                                \
    }                                                                           \
    size_t new_count = count - 1;                                               \
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(elem_type);\
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = new_count;                                                     \
    meta->capacity = new_count;                                                 \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    /* Copy elements before index */                                             \
    if (actual_index > 0) {                                                      \
        memcpy(dst, arr, (size_t)actual_index * sizeof(elem_type));              \
    }                                                                           \
    /* Copy elements after index */                                              \
    if ((size_t)actual_index < count - 1) {                                     \
        memcpy(dst + actual_index, arr + actual_index + 1,                      \
               (count - 1 - (size_t)actual_index) * sizeof(elem_type));         \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_REM_H(long, long long)
DEFINE_ARRAY_REM_H(double, double)
DEFINE_ARRAY_REM_H(char, char)
DEFINE_ARRAY_REM_H(bool, int)
DEFINE_ARRAY_REM_H(byte, unsigned char)
DEFINE_ARRAY_REM_H(int32, int32_t)
DEFINE_ARRAY_REM_H(uint32, uint32_t)
DEFINE_ARRAY_REM_H(uint, uint64_t)
DEFINE_ARRAY_REM_H(float, float)

/* String remove at index -- copies RtHandle elements */
RtHandle rt_array_rem_string_h(RtManagedArena *arena, const char **arr, long index) {
    if (arr == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    const RtHandle *src = (const RtHandle *)arr;
    size_t count = ((RtArrayMetadata *)arr)[-1].size;
    long actual_index = index < 0 ? (long)count + index : index;
    if (actual_index < 0 || (size_t)actual_index >= count) {
        fprintf(stderr, "rt_array_rem_string_h: index out of bounds\n");
        exit(1);
    }
    size_t new_count = count - 1;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = new_count;
    meta->capacity = new_count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (actual_index > 0) {
        memcpy(dst, src, (size_t)actual_index * sizeof(RtHandle));
    }
    if ((size_t)actual_index < count - 1) {
        memcpy(dst + actual_index, src + actual_index + 1,
               (count - 1 - (size_t)actual_index) * sizeof(RtHandle));
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================ */

#define DEFINE_ARRAY_INS_H(suffix, elem_type)                                   \
RtHandle rt_array_ins_##suffix##_h(RtManagedArena *arena, const elem_type *arr, \
                                    elem_type elem, long index) {               \
    size_t count = arr ? ((RtArrayMetadata *)arr)[-1].size : 0;                  \
    /* Handle negative index */                                                  \
    long actual_index = index < 0 ? (long)count + index : index;                \
    if (actual_index < 0) actual_index = 0;                                      \
    if ((size_t)actual_index > count) actual_index = (long)count;               \
    size_t new_count = count + 1;                                               \
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(elem_type);\
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);           \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = new_count;                                                     \
    meta->capacity = new_count;                                                 \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    /* Copy elements before index */                                             \
    if (actual_index > 0 && arr) {                                               \
        memcpy(dst, arr, (size_t)actual_index * sizeof(elem_type));              \
    }                                                                           \
    /* Insert element */                                                         \
    dst[actual_index] = elem;                                                    \
    /* Copy elements after index */                                              \
    if (arr && (size_t)actual_index < count) {                                  \
        memcpy(dst + actual_index + 1, arr + actual_index,                      \
               (count - (size_t)actual_index) * sizeof(elem_type));             \
    }                                                                           \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_INS_H(long, long long)
DEFINE_ARRAY_INS_H(double, double)
DEFINE_ARRAY_INS_H(char, char)
DEFINE_ARRAY_INS_H(bool, int)
DEFINE_ARRAY_INS_H(byte, unsigned char)
DEFINE_ARRAY_INS_H(int32, int32_t)
DEFINE_ARRAY_INS_H(uint32, uint32_t)
DEFINE_ARRAY_INS_H(uint, uint64_t)
DEFINE_ARRAY_INS_H(float, float)

/* String insert at index -- stores element as RtHandle */
RtHandle rt_array_ins_string_h(RtManagedArena *arena, const char **arr,
                                const char *elem, long index) {
    const RtHandle *src = (const RtHandle *)arr;
    size_t count = arr ? ((RtArrayMetadata *)arr)[-1].size : 0;
    long actual_index = index < 0 ? (long)count + index : index;
    if (actual_index < 0) actual_index = 0;
    if ((size_t)actual_index > count) actual_index = (long)count;
    size_t new_count = count + 1;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = new_count;
    meta->capacity = new_count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (actual_index > 0 && src) {
        memcpy(dst, src, (size_t)actual_index * sizeof(RtHandle));
    }
    dst[actual_index] = rt_managed_strdup(arena, RT_HANDLE_NULL, elem ? elem : "");
    if (src && (size_t)actual_index < count) {
        memcpy(dst + actual_index + 1, src + actual_index,
               (count - (size_t)actual_index) * sizeof(RtHandle));
    }
    rt_managed_unpin(arena, h);
    return h;
}
