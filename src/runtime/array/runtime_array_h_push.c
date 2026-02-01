/* ============================================================================
 * Handle-Based Array Functions Implementation - Push
 * ============================================================================
 * Push element to end of array. If capacity allows, writes in-place and returns
 * the same handle. Otherwise allocates a new handle with doubled capacity,
 * copies old data + new element, marks old handle dead.
 * ============================================================================ */

#define DEFINE_ARRAY_PUSH_H(suffix, elem_type)                                  \
RtHandle rt_array_push_##suffix##_h(RtManagedArena *arena, RtHandle arr_h,      \
                                     elem_type element) {                       \
    if (arr_h == RT_HANDLE_NULL) {                                              \
        size_t new_cap = 4;                                                     \
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(elem_type);\
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);   \
        void *new_raw = rt_managed_pin(arena, new_h);                           \
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;                     \
        elem_type *arr = (elem_type *)((char *)new_raw + sizeof(RtArrayMetadata));\
        meta->arena = (RtArena *)arena;                                         \
        meta->size = 1;                                                         \
        meta->capacity = new_cap;                                               \
        arr[0] = element;                                                       \
        rt_managed_unpin(arena, new_h);                                         \
        return new_h;                                                           \
    }                                                                           \
    void *raw = rt_managed_pin(arena, arr_h);                                   \
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;                             \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadata));       \
                                                                                \
    if (meta->size < meta->capacity) {                                          \
        arr[meta->size++] = element;                                            \
        rt_managed_unpin(arena, arr_h);                                         \
        return arr_h;                                                           \
    }                                                                           \
                                                                                \
    /* Need to grow */                                                          \
    size_t old_size = meta->size;                                               \
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;              \
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(elem_type);  \
                                                                                \
    /* Allocate new handle (don't mark old dead yet -- still pinned) */          \
    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);       \
    void *new_raw = rt_managed_pin(arena, new_h);                               \
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;                     \
    elem_type *new_arr = (elem_type *)((char *)new_raw + sizeof(RtArrayMetadata)); \
                                                                                \
    /* Copy old data (arr is still pinned via arr_h) */                          \
    memcpy(new_arr, arr, old_size * sizeof(elem_type));                         \
    new_meta->arena = (RtArena *)arena;                                         \
    new_meta->size = old_size + 1;                                              \
    new_meta->capacity = new_cap;                                               \
    new_arr[old_size] = element;                                                \
                                                                                \
    rt_managed_unpin(arena, new_h);                                             \
    rt_managed_unpin(arena, arr_h);                                             \
                                                                                \
    /* Mark old handle as dead */                                                \
    rt_managed_mark_dead(arena, arr_h);                                         \
                                                                                \
    return new_h;                                                               \
}

DEFINE_ARRAY_PUSH_H(long, long long)
DEFINE_ARRAY_PUSH_H(double, double)
DEFINE_ARRAY_PUSH_H(char, char)
DEFINE_ARRAY_PUSH_H(bool, int)
DEFINE_ARRAY_PUSH_H(byte, unsigned char)
DEFINE_ARRAY_PUSH_H(int32, int32_t)
DEFINE_ARRAY_PUSH_H(uint32, uint32_t)
DEFINE_ARRAY_PUSH_H(uint, uint64_t)
DEFINE_ARRAY_PUSH_H(float, float)

/* Pointer (nested array) push -- stores element as RtHandle */
RtHandle rt_array_push_ptr_h(RtManagedArena *arena, RtHandle arr_h, void *element) {
    RtHandle elem_h = (RtHandle)(uintptr_t)element;
    if (arr_h == RT_HANDLE_NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtHandle);
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *new_raw = rt_managed_pin(arena, new_h);
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;
        RtHandle *arr = (RtHandle *)((char *)new_raw + sizeof(RtArrayMetadata));
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = elem_h;
        rt_managed_unpin(arena, new_h);
        return new_h;
    }
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_managed_unpin(arena, arr_h);
        return arr_h;
    }
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtHandle);
    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *new_raw = rt_managed_pin(arena, new_h);
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;
    RtHandle *new_arr = (RtHandle *)((char *)new_raw + sizeof(RtArrayMetadata));
    memcpy(new_arr, arr, old_size * sizeof(RtHandle));
    new_meta->arena = (RtArena *)arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = elem_h;
    rt_managed_unpin(arena, new_h);
    rt_managed_unpin(arena, arr_h);
    rt_managed_mark_dead(arena, arr_h);
    return new_h;
}

/* Generic struct push -- copies element by value using memcpy */
RtHandle rt_array_push_struct_h(RtManagedArena *arena, RtHandle arr_h, const void *element, size_t elem_size) {
    if (arr_h == RT_HANDLE_NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * elem_size;
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *new_raw = rt_managed_pin(arena, new_h);
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;
        char *arr = (char *)new_raw + sizeof(RtArrayMetadata);
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = new_cap;
        memcpy(arr, element, elem_size);
        rt_managed_unpin(arena, new_h);
        return new_h;
    }
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    char *arr = (char *)raw + sizeof(RtArrayMetadata);
    if (meta->size < meta->capacity) {
        memcpy(arr + meta->size * elem_size, element, elem_size);
        meta->size++;
        rt_managed_unpin(arena, arr_h);
        return arr_h;
    }
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * elem_size;
    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *new_raw = rt_managed_pin(arena, new_h);
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;
    char *new_arr = (char *)new_raw + sizeof(RtArrayMetadata);
    memcpy(new_arr, arr, old_size * elem_size);
    new_meta->arena = (RtArena *)arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    memcpy(new_arr + old_size * elem_size, element, elem_size);
    rt_managed_unpin(arena, new_h);
    rt_managed_unpin(arena, arr_h);
    rt_managed_mark_dead(arena, arr_h);
    return new_h;
}

/* Void pointer push -- stores element as full void* (8 bytes, for closures/function pointers) */
RtHandle rt_array_push_voidptr_h(RtManagedArena *arena, RtHandle arr_h, void *element) {
    if (arr_h == RT_HANDLE_NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(void *);
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *new_raw = rt_managed_pin(arena, new_h);
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;
        void **arr = (void **)((char *)new_raw + sizeof(RtArrayMetadata));
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = element;
        rt_managed_unpin(arena, new_h);
        return new_h;
    }
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    void **arr = (void **)((char *)raw + sizeof(RtArrayMetadata));
    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_managed_unpin(arena, arr_h);
        return arr_h;
    }
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(void *);
    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *new_raw = rt_managed_pin(arena, new_h);
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;
    void **new_arr = (void **)((char *)new_raw + sizeof(RtArrayMetadata));
    memcpy(new_arr, arr, old_size * sizeof(void *));
    new_meta->arena = (RtArena *)arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = element;
    rt_managed_unpin(arena, new_h);
    rt_managed_unpin(arena, arr_h);
    rt_managed_mark_dead(arena, arr_h);
    return new_h;
}

/* String push -- stores element as RtHandle */
RtHandle rt_array_push_string_h(RtManagedArena *arena, RtHandle arr_h, const char *element) {
    RtHandle elem_h = rt_managed_strdup(arena, RT_HANDLE_NULL, element ? element : "");
    if (arr_h == RT_HANDLE_NULL) {
        /* Create initial array with capacity 4 */
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtHandle);
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *new_raw = rt_managed_pin(arena, new_h);
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;
        RtHandle *arr = (RtHandle *)((char *)new_raw + sizeof(RtArrayMetadata));
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = elem_h;
        rt_managed_unpin(arena, new_h);
        return new_h;
    }
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_managed_unpin(arena, arr_h);
        return arr_h;
    }

    /* Need to grow */
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtHandle);

    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *new_raw = rt_managed_pin(arena, new_h);
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;
    RtHandle *new_arr = (RtHandle *)((char *)new_raw + sizeof(RtArrayMetadata));

    memcpy(new_arr, arr, old_size * sizeof(RtHandle));
    new_meta->arena = (RtArena *)arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = elem_h;

    rt_managed_unpin(arena, new_h);
    rt_managed_unpin(arena, arr_h);

    rt_managed_mark_dead(arena, arr_h);

    return new_h;
}

/* Any (RtAny) push -- handles RT_HANDLE_NULL for initial creation */
RtHandle rt_array_push_any_h(RtManagedArena *arena, RtHandle arr_h, RtAny element) {
    if (arr_h == RT_HANDLE_NULL) {
        /* Create initial array with capacity 4 */
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtAny);
        RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *new_raw = rt_managed_pin(arena, new_h);
        RtArrayMetadata *meta = (RtArrayMetadata *)new_raw;
        RtAny *arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadata));
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = element;
        rt_managed_unpin(arena, new_h);
        return new_h;
    }

    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtAny *arr = (RtAny *)((char *)raw + sizeof(RtArrayMetadata));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_managed_unpin(arena, arr_h);
        return arr_h;
    }

    /* Need to grow */
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadata) + new_cap * sizeof(RtAny);

    RtHandle new_h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *new_raw = rt_managed_pin(arena, new_h);
    RtArrayMetadata *new_meta = (RtArrayMetadata *)new_raw;
    RtAny *new_arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadata));

    memcpy(new_arr, arr, old_size * sizeof(RtAny));
    new_meta->arena = (RtArena *)arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = element;

    rt_managed_unpin(arena, new_h);
    rt_managed_unpin(arena, arr_h);
    rt_managed_mark_dead(arena, arr_h);

    return new_h;
}
