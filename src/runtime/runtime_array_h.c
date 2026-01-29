#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "runtime_array_h.h"
#include "runtime_array.h"
#include "runtime_arena.h"

/* ============================================================================
 * Handle-Based Array Functions Implementation
 * ============================================================================
 * RtHandle-returning variants of all allocating array functions.
 * Array handle layout: [RtArrayMetadata][element data...]
 *
 * For source arrays passed as raw pointers, metadata is at:
 *   ((RtArrayMetadata *)arr)[-1]
 *
 * For handle-based arrays, metadata is at the start of the allocation.
 * ============================================================================ */

/* ============================================================================
 * Internal Helpers
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

/* ============================================================================
 * Array Push Functions
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

/* ============================================================================
 * Array Pop Functions
 * ============================================================================
 * Pin the handle, decrement size, read the popped value, unpin.
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
 * ============================================================================
 * Source is a raw pointer array with metadata at ((RtArrayMetadata *)src)[-1].
 * Creates a new handle-based array with the same data.
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

/* ============================================================================
 * Array Concat Functions
 * ============================================================================
 * Source arrays are raw pointer arrays with metadata at [-1].
 * Creates a new handle-based array with combined data.
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

/* Pointer concat -- copies pointer arrays */
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
 * ============================================================================
 * Source is a raw pointer array with metadata at [-1].
 * start/end/step use LONG_MIN as sentinel for default values.
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

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Source is a raw pointer array with metadata at [-1].
 * Creates a new handle-based array with elements in reverse order.
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
 * ============================================================================
 * Source is a raw pointer array with metadata at [-1].
 * Creates a new handle-based array without the element at the given index.
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
 * ============================================================================
 * Source is a raw pointer array with metadata at [-1].
 * Creates a new handle-based array with element inserted at the given index.
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

/* ============================================================================
 * Array Push Copy Functions (Non-Mutating)
 * ============================================================================
 * Source is a raw pointer array with metadata at [-1].
 * Creates a new handle-based array with element appended.
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
 * ============================================================================
 * Creates a new handle-based array filled with the given default value.
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
 * ============================================================================
 * Creates a long long array from start to end-1.
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
 * Legacy Bridge: Convert raw char** to handle-based string array
 * ============================================================================
 * Source is a legacy raw pointer array (char* elements, metadata at [-1]).
 * Creates a new handle-based array with RtHandle elements.
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

/* ============================================================================
 * Args Create Function
 * ============================================================================
 * Creates a string (char *) array from argc/argv.
 * ============================================================================ */

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

/**
 * Compare two string arrays (handle-based) for equality.
 * Elements are stored as RtHandle values that must be pinned to compare.
 */
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

/* ============================================================================
 * Handle-Aware 2D Array to String Functions
 * ============================================================================
 * These convert nested arrays (where the outer array stores RtHandle elements)
 * to string representation. The outer array has been pinned by the caller to
 * yield RtHandle *. Each element handle is pinned here to get inner arrays.
 * ============================================================================ */

char *rt_to_string_array2_long_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        long long *inner = (long long *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_long((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_double_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        double *inner = (double *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_double((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_char_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        char *inner = (char *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_char((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_bool_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        int *inner = (int *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_bool((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_byte_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        unsigned char *inner = (unsigned char *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_byte((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_string_h(RtManagedArena *arena, RtHandle *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t len = rt_array_length(arr);
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        if (i > 0) total_len += 2; /* ", " */
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            total_len += strlen(s) + 2; /* "str" */
        } else {
            total_len += 4; /* null */
        }
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    if (result == NULL) return "{}";
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        if (arr[i] != RT_HANDLE_NULL) {
            *p++ = '"';
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            while (*s) *p++ = *s++;
            *p++ = '"';
        } else {
            const char *null_str = "null";
            while (*null_str) *p++ = *null_str++;
        }
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_string_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        /* Inner string arrays store RtHandle elements - use handle-aware toString */
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_string_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_any_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtAny *inner = (RtAny *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_any((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_any_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_any_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

/* 3D array formatters - call 2D formatters for each inner 2D array */

char *rt_to_string_array3_long_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_long_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_double_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_double_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_char_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_char_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_bool_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_bool_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_byte_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_byte_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_string_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_string_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

/* ============================================================================
 * Handle-Aware Array Join for String Arrays
 * ============================================================================
 * Elements are RtHandle values that must be pinned to get char* strings.
 * ============================================================================ */

char *rt_array_join_string_h(RtManagedArena *arena, RtHandle *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup((RtArena *)arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            total_len += strlen(s);
        }
    }
    total_len += (len - 1) * sep_len + 1;

    char *result = rt_arena_alloc((RtArena *)arena, total_len);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_string_h: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            size_t l = strlen(separator);
            memcpy(ptr, separator, l);
            ptr += l;
        }
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            size_t l = strlen(s);
            memcpy(ptr, s, l);
            ptr += l;
        }
    }
    *ptr = '\0';
    return result;
}

/* ============================================================================
 * Handle-Aware Array Print for String Arrays
 * ============================================================================
 * Elements are RtHandle values that must be pinned to get char* strings.
 * ============================================================================ */

void rt_print_array_string_h(RtManagedArena *arena, RtHandle *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            if (arr[i] != RT_HANDLE_NULL) {
                const char *s = (const char *)rt_managed_pin(arena, arr[i]);
                printf("\"%s\"", s);
            } else {
                printf("null");
            }
        }
    }
    printf("]");
}

/* ============================================================================
 * Handle-Aware Array indexOf/contains for String Arrays
 * ============================================================================ */

long rt_array_indexOf_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem) {
    if (arr == NULL) {
        return -1L;
    }
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        if (arr[i] == RT_HANDLE_NULL && elem == NULL) {
            return (long)i;
        }
        if (arr[i] != RT_HANDLE_NULL && elem != NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            if (strcmp(s, elem) == 0) {
                return (long)i;
            }
        }
    }
    return -1L;
}

int rt_array_contains_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem) {
    return rt_array_indexOf_string_h(arena, arr, elem) >= 0;
}

/* Convert a legacy char** string array to a handle-based string array.
 * Each char* element is copied via rt_managed_strdup into a new RtHandle element. */
RtHandle rt_array_from_legacy_string_h(RtManagedArena *arena, char **src) {
    if (src == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t count = ((RtArrayMetadata *)src)[-1].size;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, sizeof(RtArrayMetadata) + count * sizeof(RtHandle));
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

/* Convert a handle-based string array (RtHandle elements) to a legacy char** array.
 * Each RtHandle element is pinned to get the char*. The returned array has
 * RtArrayMetadata at [-1] so it can be used with rt_array_length(). */
char **rt_managed_pin_string_array(RtManagedArena *arena, RtHandle arr_h) {
    if (arr_h == RT_HANDLE_NULL) return NULL;
    RtHandle *handles = (RtHandle *)rt_managed_pin_array(arena, arr_h);
    if (handles == NULL) return NULL;
    size_t count = rt_array_length(handles);
    /* Allocate a legacy char** array from the arena */
    char **result = rt_array_create_string((RtArena *)arena, count, NULL);
    for (size_t i = 0; i < count; i++) {
        result[i] = (char *)rt_managed_pin(arena, handles[i]);
    }
    return result;
}

/* Convert a handle-based string array (RtHandle elements) to a legacy RtAny* array.
 * Each RtHandle element is pinned to get the char*, then boxed as RtAny. */
RtAny *rt_array_to_any_string_h(RtManagedArena *arena, RtHandle arr_h) {
    if (arr_h == RT_HANDLE_NULL) return NULL;
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *elements = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    size_t len = meta->size;
    if (len == 0) {
        rt_managed_unpin(arena, arr_h);
        return NULL;
    }
    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        char *str = (char *)rt_managed_pin(arena, elements[i]);
        result = rt_array_push_any((RtArena *)arena, result, rt_box_string(str));
        rt_managed_unpin(arena, elements[i]);
    }
    rt_managed_unpin(arena, arr_h);
    return result;
}

/* ============================================================================
 * 2D Array to any[][] Conversion (handle-based)
 * ============================================================================
 * These convert a handle to an array of typed-array handles into a handle to
 * an array of any-array handles. Each inner typed array is pinned, converted
 * to RtAny* using the legacy function, then stored as a new handle.
 * ============================================================================ */

RtHandle rt_array2_to_any_long_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        long long *inner_data = (long long *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_long((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_double_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        double *inner_data = (double *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_double((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_char_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        char *inner_data = (char *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_char((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_bool_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        int *inner_data = (int *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_bool((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_byte_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        unsigned char *inner_data = (unsigned char *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_byte((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_string_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *any_inner = rt_array_to_any_string_h(arena, inner_handles[i]);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

/* ============================================================================
 * 3D Array to any[][][] Conversion (handle-based)
 * ============================================================================ */

RtHandle rt_array3_to_any_long_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_long_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_double_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_double_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_char_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_char_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_bool_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_bool_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_byte_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_byte_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_string_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_string_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

/* ============================================================================
 * Deep Array Promotion (child -> parent arena)
 * ============================================================================
 * Promotes an array and all its handle-type elements from one arena to another.
 * Used for returning str[] from functions to prevent corruption when the
 * callee's arena is destroyed.
 * ============================================================================ */

RtHandle rt_managed_promote_array_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First, get the source array's size while src arena is still valid */
    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each string element FIRST (while src arena is valid) */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        /* Allocate temporary array on stack or heap depending on size */
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Promote each string handle from src to dest arena */
            promoted_handles[i] = rt_managed_promote(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted array with the promoted string handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

/* ============================================================================
 * Deep promotion for 3D string arrays (str[][][])
 * ============================================================================
 * For str[][][], we need to:
 * 1. Promote the outermost array
 * 2. For each middle array (str[][]), use rt_managed_promote_array2_string
 * ============================================================================ */

RtHandle rt_managed_promote_array3_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First, get the source array's size while src arena is still valid */
    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner 2D string array using rt_managed_promote_array2_string */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Each element is a str[][] - use 2D string array promotion */
            promoted_handles[i] = rt_managed_promote_array2_string(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted outer array with the promoted inner array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

/* ============================================================================
 * Deep promotion for 2D/3D arrays (handle arrays)
 * ============================================================================
 * Similar to rt_managed_promote_array_string, but for arrays where elements
 * are RtHandle values pointing to other arrays (2D, 3D arrays).
 * ============================================================================ */

/* Internal helper with depth parameter for recursive promotion */
static RtHandle rt_managed_promote_array_handle_depth(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h, int depth) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First, get the source array's size while src arena is still valid */
    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner array handle FIRST (while src arena is valid) */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        /* Allocate temporary array on stack or heap depending on size */
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            if (depth > 1) {
                /* Inner arrays also contain handles - recurse with reduced depth */
                promoted_handles[i] = rt_managed_promote_array_handle_depth(dest, src, src_handles[i], depth - 1);
            } else {
                /* Innermost level - just shallow promote */
                promoted_handles[i] = rt_managed_promote(dest, src, src_handles[i]);
            }
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted array with the promoted inner array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

RtHandle rt_managed_promote_array_handle(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    /* Default depth=1 for 2D arrays (outer array contains handles to 1D arrays) */
    return rt_managed_promote_array_handle_depth(dest, src, arr_h, 1);
}

RtHandle rt_managed_promote_array_handle_3d(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    /* Depth=2 for 3D arrays (outer array -> 2D arrays -> 1D arrays) */
    return rt_managed_promote_array_handle_depth(dest, src, arr_h, 2);
}

/* ============================================================================
 * Deep promotion for 2D string arrays (str[][])
 * ============================================================================
 * For str[][], we need to:
 * 1. Promote the outer array
 * 2. For each inner array, use rt_managed_promote_array_string to promote
 *    both the inner array structure AND all string elements within it
 * ============================================================================ */

RtHandle rt_managed_promote_array2_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First, get the source array's size while src arena is still valid */
    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner string array using rt_managed_promote_array_string */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Each element is a str[] - use deep string array promotion */
            promoted_handles[i] = rt_managed_promote_array_string(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted outer array with the promoted inner string array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}
