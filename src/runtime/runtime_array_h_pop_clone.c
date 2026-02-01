/* ============================================================================
 * Handle-Based Array Functions Implementation - Pop and Clone
 * ============================================================================
 * Pop: Pin the handle, decrement size, read the popped value, unpin.
 * Clone: Source is raw pointer array with metadata at [-1]. Creates new handle.
 * ============================================================================ */

#define DEFINE_ARRAY_POP_H(suffix, elem_type, default_val)                      \
elem_type rt_array_pop_##suffix##_h(RtManagedArena *arena, RtHandle arr_h) {    \
    void *raw = rt_managed_pin(arena, arr_h);                                   \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
                                                                                \
    if (meta->size == 0) {                                                      \
        fprintf(stderr, "rt_array_pop_" #suffix "_h: empty array\n");           \
        rt_managed_unpin(arena, arr_h);                                         \
        exit(1);                                                                \
    }                                                                           \
                                                                                \
    meta->size--;                                                               \
    elem_type val = arr[meta->size];                                            \
    rt_managed_unpin(arena, arr_h);                                             \
    return val;                                                                 \
}

DEFINE_ARRAY_POP_H(long, long long, 0)
DEFINE_ARRAY_POP_H(double, double, 0.0)
DEFINE_ARRAY_POP_H(char, char, '\0')
DEFINE_ARRAY_POP_H(bool, int, 0)
DEFINE_ARRAY_POP_H(byte, unsigned char, 0)
DEFINE_ARRAY_POP_H(int32, int32_t, 0)
DEFINE_ARRAY_POP_H(uint32, uint32_t, 0)
DEFINE_ARRAY_POP_H(uint, uint64_t, 0)
DEFINE_ARRAY_POP_H(float, float, 0.0f)

RtHandle rt_array_pop_string_h(RtManagedArena *arena, RtHandle arr_h) {
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));

    if (meta->size == 0) {
        fprintf(stderr, "rt_array_pop_string_h: empty array\n");
        rt_managed_unpin(arena, arr_h);
        exit(1);
    }

    meta->size--;
    RtHandle elem_h = arr[meta->size];
    rt_managed_unpin(arena, arr_h);
    return elem_h;
}

void *rt_array_pop_ptr_h(RtManagedArena *arena, RtHandle arr_h) {
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (meta->size == 0) {
        fprintf(stderr, "rt_array_pop_ptr_h: empty array\n");
        rt_managed_unpin(arena, arr_h);
        exit(1);
    }
    meta->size--;
    RtHandle val = arr[meta->size];
    rt_managed_unpin(arena, arr_h);
    return (void *)(uintptr_t)val;
}

/* ============================================================================
 * Array Clone Functions
 * ============================================================================ */

#define DEFINE_ARRAY_CLONE_H(suffix, elem_type)                                 \
RtHandle rt_array_clone_##suffix##_h(RtManagedArena *arena, RtHandle old,       \
                                      const elem_type *src) {                   \
    if (src == NULL) {                                                          \
        return array_create_h(arena, 0, sizeof(elem_type), NULL);               \
    }                                                                           \
    size_t count = ((RtArrayMetadata *)src)[-1].size;                            \
    RtHandle h = rt_managed_alloc(arena, old, sizeof(RtArrayMetadata) + count * sizeof(elem_type)); \
    void *raw = rt_managed_pin(arena, h);                                       \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    meta->arena = (RtArena *)arena;                                             \
    meta->size = count;                                                         \
    meta->capacity = count;                                                     \
    elem_type *dst = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
    /* Use memmove instead of memcpy: source and destination may overlap when   \
     * both are allocated in the same arena block (bump allocator). */          \
    memmove(dst, src, count * sizeof(elem_type));                               \
    rt_managed_unpin(arena, h);                                                 \
    return h;                                                                   \
}

DEFINE_ARRAY_CLONE_H(long, long long)
DEFINE_ARRAY_CLONE_H(double, double)
DEFINE_ARRAY_CLONE_H(char, char)
DEFINE_ARRAY_CLONE_H(bool, int)
DEFINE_ARRAY_CLONE_H(byte, unsigned char)
DEFINE_ARRAY_CLONE_H(int32, int32_t)
DEFINE_ARRAY_CLONE_H(uint32, uint32_t)
DEFINE_ARRAY_CLONE_H(uint, uint64_t)
DEFINE_ARRAY_CLONE_H(float, float)
DEFINE_ARRAY_CLONE_H(void, RtAny)

/* String clone -- copies RtHandle elements from source to new handle array */
RtHandle rt_array_clone_string_h(RtManagedArena *arena, RtHandle old, const char **src) {
    if (src == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t count = ((RtArrayMetadata *)src)[-1].size;
    RtHandle h = rt_managed_alloc(arena, old, sizeof(RtArrayMetadata) + count * sizeof(RtHandle));
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    memmove(dst, src, count * sizeof(RtHandle));  /* memmove for potential overlap */
    rt_managed_unpin(arena, h);
    return h;
}

/* Pointer clone -- copies RtHandle elements (nested arrays) */
RtHandle rt_array_clone_ptr_h(RtManagedArena *arena, RtHandle old, void **src) {
    if (src == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t count = ((RtArrayMetadata *)src)[-1].size;
    RtHandle h = rt_managed_alloc(arena, old, sizeof(RtArrayMetadata) + count * sizeof(RtHandle));
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    memmove(dst, src, count * sizeof(RtHandle));  /* memmove for potential overlap */
    rt_managed_unpin(arena, h);
    return h;
}
