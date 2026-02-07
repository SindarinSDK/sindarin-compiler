/*
 * Runtime Array V2 Implementation
 * ================================
 *
 * Handle-based array operations using Arena V2.
 * Key differences from V1:
 * - No arena parameter for pin/unpin (handles are self-describing)
 * - Simpler promotion (no source arena parameter)
 * - String arrays store RtHandleV2* (pointer) instead of RtHandle (uint32)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime_array_v2.h"
#include "runtime/runtime_any.h"

/* ============================================================================
 * Raw Array Metadata (for compatibility with V1 raw arrays)
 * ============================================================================
 * Raw arrays (non-handle) have metadata stored immediately before the data.
 * This is compatible with V1 format: [RtArrayMetadata][data...]
 * We define a minimal version here to avoid V1 include chain.
 * ============================================================================ */

typedef struct {
    void *arena;        /* Arena that owns this array (unused in V2 context) */
    size_t size;        /* Number of elements currently in the array */
    size_t capacity;    /* Total allocated space for elements */
} RtArrayMetadataRaw;

/* Get length of a raw array (V1-compatible format) */
static inline size_t rt_raw_array_length(const void *arr) {
    if (arr == NULL) return 0;
    return ((const RtArrayMetadataRaw *)arr)[-1].size;
}

/* Get length from V2 data pointer (V2 format has metadata before data) */
static inline size_t rt_v2_data_array_length(const void *arr) {
    if (arr == NULL) return 0;
    const RtArrayMetadataV2 *meta = (const RtArrayMetadataV2 *)((const char *)arr - sizeof(RtArrayMetadataV2));
    return meta->size;
}

/* ============================================================================
 * Internal Helper: Create Array
 * ============================================================================ */

static RtHandleV2 *array_create_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    void *arr_data = (char *)raw + sizeof(RtArrayMetadataV2);
    if (data && count > 0) {
        memcpy(arr_data, data, count * elem_size);
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Create Functions
 * ============================================================================ */

#define DEFINE_ARRAY_CREATE_V2(suffix, elem_type)                                \
RtHandleV2 *rt_array_create_##suffix##_v2(RtArenaV2 *arena, size_t count,        \
                                           const elem_type *data) {              \
    return array_create_v2(arena, count, sizeof(elem_type), data);               \
}

DEFINE_ARRAY_CREATE_V2(long, long long)
DEFINE_ARRAY_CREATE_V2(double, double)
DEFINE_ARRAY_CREATE_V2(char, char)
DEFINE_ARRAY_CREATE_V2(bool, int)
DEFINE_ARRAY_CREATE_V2(byte, unsigned char)
DEFINE_ARRAY_CREATE_V2(int32, int32_t)
DEFINE_ARRAY_CREATE_V2(uint32, uint32_t)
DEFINE_ARRAY_CREATE_V2(uint, uint64_t)
DEFINE_ARRAY_CREATE_V2(float, float)

/* String array: converts char* pointers to RtHandleV2* elements */
RtHandleV2 *rt_array_create_string_v2(RtArenaV2 *arena, size_t count, const char **data) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        arr[i] = rt_arena_v2_strdup(arena, data[i] ? data[i] : "");
    }

    rt_handle_v2_unpin(h);
    return h;
}

RtHandleV2 *rt_array_create_generic_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data) {
    return array_create_v2(arena, count, elem_size, data);
}

/* Pointer (nested array) create -- elements are RtHandleV2* */
RtHandleV2 *rt_array_create_ptr_v2(RtArenaV2 *arena, size_t count, void **data) {
    return array_create_v2(arena, count, sizeof(RtHandleV2 *), data);
}

/* ============================================================================
 * Array Push Functions
 * ============================================================================ */

#define DEFINE_ARRAY_PUSH_V2(suffix, elem_type)                                  \
RtHandleV2 *rt_array_push_##suffix##_v2(RtArenaV2 *arena, RtHandleV2 *arr_h,     \
                                         elem_type element) {                    \
    if (arr_h == NULL) {                                                         \
        size_t new_cap = 4;                                                      \
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(elem_type);\
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);                \
        if (!new_h) return NULL;                                                 \
        void *new_raw = rt_handle_v2_pin(new_h);                                 \
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;                  \
        elem_type *arr = (elem_type *)((char *)new_raw + sizeof(RtArrayMetadataV2));\
        meta->arena = arena;                                                     \
        meta->size = 1;                                                          \
        meta->capacity = new_cap;                                                \
        arr[0] = element;                                                        \
        rt_handle_v2_unpin(new_h);                                               \
        return new_h;                                                            \
    }                                                                            \
                                                                                 \
    void *raw = rt_handle_v2_pin(arr_h);                                         \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
                                                                                 \
    if (meta->size < meta->capacity) {                                           \
        arr[meta->size++] = element;                                             \
        rt_handle_v2_unpin(arr_h);                                               \
        return arr_h;                                                            \
    }                                                                            \
                                                                                 \
    /* Need to grow */                                                           \
    size_t old_size = meta->size;                                                \
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;               \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(elem_type); \
                                                                                 \
    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);                    \
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }                      \
    void *new_raw = rt_handle_v2_pin(new_h);                                     \
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;                  \
    elem_type *new_arr = (elem_type *)((char *)new_raw + sizeof(RtArrayMetadataV2));\
                                                                                 \
    memcpy(new_arr, arr, old_size * sizeof(elem_type));                          \
    new_meta->arena = arena;                                                     \
    new_meta->size = old_size + 1;                                               \
    new_meta->capacity = new_cap;                                                \
    new_arr[old_size] = element;                                                 \
                                                                                 \
    rt_handle_v2_unpin(new_h);                                                   \
    rt_handle_v2_unpin(arr_h);                                                   \
    rt_arena_v2_free(arr_h);                                                     \
                                                                                 \
    return new_h;                                                                \
}

DEFINE_ARRAY_PUSH_V2(long, long long)
DEFINE_ARRAY_PUSH_V2(double, double)
DEFINE_ARRAY_PUSH_V2(char, char)
DEFINE_ARRAY_PUSH_V2(bool, int)
DEFINE_ARRAY_PUSH_V2(byte, unsigned char)
DEFINE_ARRAY_PUSH_V2(int32, int32_t)
DEFINE_ARRAY_PUSH_V2(uint32, uint32_t)
DEFINE_ARRAY_PUSH_V2(uint, uint64_t)
DEFINE_ARRAY_PUSH_V2(float, float)

/* String push -- stores element as RtHandleV2* */
RtHandleV2 *rt_array_push_string_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const char *element) {
    RtHandleV2 *elem_h = rt_arena_v2_strdup(arena, element ? element : "");

    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        void *new_raw = rt_handle_v2_pin(new_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = elem_h;
        rt_handle_v2_unpin(new_h);
        return new_h;
    }

    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_handle_v2_unpin(arr_h);
        return arr_h;
    }

    /* Need to grow */
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }
    void *new_raw = rt_handle_v2_pin(new_h);
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtHandleV2 *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = elem_h;

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Pointer (nested array) push -- stores element as RtHandleV2* */
RtHandleV2 *rt_array_push_ptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, void *element) {
    RtHandleV2 *elem_h = (RtHandleV2 *)element;

    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        void *new_raw = rt_handle_v2_pin(new_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = elem_h;
        rt_handle_v2_unpin(new_h);
        return new_h;
    }

    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_handle_v2_unpin(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }
    void *new_raw = rt_handle_v2_pin(new_h);
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtHandleV2 *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = elem_h;

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Void pointer push -- stores element as full void* (8 bytes) */
RtHandleV2 *rt_array_push_voidptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, void *element) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(void *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        void *new_raw = rt_handle_v2_pin(new_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        void **arr = (void **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = element;
        rt_handle_v2_unpin(new_h);
        return new_h;
    }

    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    void **arr = (void **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_handle_v2_unpin(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(void *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }
    void *new_raw = rt_handle_v2_pin(new_h);
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    void **new_arr = (void **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(void *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = element;

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Generic struct push */
RtHandleV2 *rt_array_push_struct_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const void *element, size_t elem_size) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * elem_size;
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        void *new_raw = rt_handle_v2_pin(new_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        char *arr = (char *)new_raw + sizeof(RtArrayMetadataV2);
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        memcpy(arr, element, elem_size);
        rt_handle_v2_unpin(new_h);
        return new_h;
    }

    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    char *arr = (char *)raw + sizeof(RtArrayMetadataV2);

    if (meta->size < meta->capacity) {
        memcpy(arr + meta->size * elem_size, element, elem_size);
        meta->size++;
        rt_handle_v2_unpin(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * elem_size;

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }
    void *new_raw = rt_handle_v2_pin(new_h);
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    char *new_arr = (char *)new_raw + sizeof(RtArrayMetadataV2);

    memcpy(new_arr, arr, old_size * elem_size);
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    memcpy(new_arr + old_size * elem_size, element, elem_size);

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Push RtAny element */
RtHandleV2 *rt_array_push_any_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtAny element) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtAny);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        void *new_raw = rt_handle_v2_pin(new_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtAny *arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        arr[0] = element;
        rt_handle_v2_unpin(new_h);
        return new_h;
    }

    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtAny *arr = (RtAny *)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_handle_v2_unpin(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtAny);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_v2_unpin(arr_h); return NULL; }
    void *new_raw = rt_handle_v2_pin(new_h);
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtAny *new_arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtAny));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_arr[old_size] = element;

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* ============================================================================
 * Array Pop Functions
 * ============================================================================ */

#define DEFINE_ARRAY_POP_V2(suffix, elem_type, default_val)                      \
elem_type rt_array_pop_##suffix##_v2(RtHandleV2 *arr_h) {                        \
    if (arr_h == NULL) return default_val;                                       \
    void *raw = rt_handle_v2_pin(arr_h);                                         \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    if (meta->size == 0) {                                                       \
        rt_handle_v2_unpin(arr_h);                                               \
        return default_val;                                                      \
    }                                                                            \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    elem_type result = arr[--meta->size];                                        \
    rt_handle_v2_unpin(arr_h);                                                   \
    return result;                                                               \
}

DEFINE_ARRAY_POP_V2(long, long long, 0)
DEFINE_ARRAY_POP_V2(double, double, 0.0)
DEFINE_ARRAY_POP_V2(char, char, '\0')
DEFINE_ARRAY_POP_V2(bool, int, 0)
DEFINE_ARRAY_POP_V2(byte, unsigned char, 0)
DEFINE_ARRAY_POP_V2(int32, int32_t, 0)
DEFINE_ARRAY_POP_V2(uint32, uint32_t, 0)
DEFINE_ARRAY_POP_V2(uint, uint64_t, 0)
DEFINE_ARRAY_POP_V2(float, float, 0.0f)
DEFINE_ARRAY_POP_V2(ptr, void *, NULL)

/* String pop returns handle */
RtHandleV2 *rt_array_pop_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    void *raw = rt_handle_v2_pin(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    if (meta->size == 0) {
        rt_handle_v2_unpin(arr_h);
        return NULL;
    }
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 *result = arr[--meta->size];
    rt_handle_v2_unpin(arr_h);
    return result;
}

/* ============================================================================
 * Array Clone Functions (from raw arrays)
 * ============================================================================ */

#define DEFINE_ARRAY_CLONE_V2(suffix, elem_type)                                 \
RtHandleV2 *rt_array_clone_##suffix##_v2(RtArenaV2 *arena, const elem_type *src) {\
    if (src == NULL) return rt_array_create_##suffix##_v2(arena, 0, NULL);       \
    size_t count = rt_v2_data_array_length((void *)src);                                 \
    return rt_array_create_##suffix##_v2(arena, count, src);                     \
}

DEFINE_ARRAY_CLONE_V2(long, long long)
DEFINE_ARRAY_CLONE_V2(double, double)
DEFINE_ARRAY_CLONE_V2(char, char)
DEFINE_ARRAY_CLONE_V2(bool, int)
DEFINE_ARRAY_CLONE_V2(byte, unsigned char)
DEFINE_ARRAY_CLONE_V2(int32, int32_t)
DEFINE_ARRAY_CLONE_V2(uint32, uint32_t)
DEFINE_ARRAY_CLONE_V2(uint, uint64_t)
DEFINE_ARRAY_CLONE_V2(float, float)

/* Clone RtAny array from raw RtAny* */
RtHandleV2 *rt_array_clone_void_v2(RtArenaV2 *arena, const RtAny *src) {
    if (src == NULL) return rt_array_create_generic_v2(arena, 0, sizeof(RtAny), NULL);
    size_t count = rt_v2_data_array_length((void *)src);
    return rt_array_create_generic_v2(arena, count, sizeof(RtAny), src);
}

/* String clone from raw char** */
RtHandleV2 *rt_array_clone_string_v2(RtArenaV2 *arena, const char **src) {
    if (src == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t count = rt_v2_data_array_length((void *)src);
    return rt_array_create_string_v2(arena, count, src);
}

/* Clone a string array from a handle (V2 string arrays contain RtHandleV2* elements) */
RtHandleV2 *rt_array_clone_string_handle_v2(RtArenaV2 *arena, RtHandleV2 *src_arr) {
    if (src_arr == NULL) {
        /* Return empty array */
        size_t alloc_size = sizeof(RtArrayMetadataV2);
        RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
        if (!h) return NULL;
        void *raw = rt_handle_v2_pin(h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = 0;
        rt_handle_v2_unpin(h);
        return h;
    }

    /* Get source array metadata and elements */
    size_t count = rt_array_length_v2(src_arr);
    RtHandleV2 **src_elems = (RtHandleV2 **)rt_array_data_v2(src_arr);

    /* Allocate new array with same capacity */
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;
    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    RtHandleV2 **dst_elems = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    /* Clone each string handle to the target arena */
    for (size_t i = 0; i < count; i++) {
        if (src_elems[i] != NULL) {
            const char *str = (const char *)rt_handle_v2_pin(src_elems[i]);
            dst_elems[i] = rt_arena_v2_strdup(arena, str);
            rt_handle_v2_unpin(src_elems[i]);
        } else {
            dst_elems[i] = NULL;
        }
    }

    rt_handle_v2_unpin(h);
    return h;
}

RtHandleV2 *rt_array_clone_ptr_v2(RtArenaV2 *arena, void **src) {
    if (src == NULL) return rt_array_create_ptr_v2(arena, 0, NULL);
    size_t count = rt_v2_data_array_length(src);
    return rt_array_create_ptr_v2(arena, count, src);
}

/* ============================================================================
 * Array Concat Functions
 * ============================================================================ */

#define DEFINE_ARRAY_CONCAT_V2(suffix, elem_type)                                \
RtHandleV2 *rt_array_concat_##suffix##_v2(RtArenaV2 *arena, const elem_type *a,  \
                                           const elem_type *b) {                 \
    size_t len_a = a ? rt_v2_data_array_length((void *)a) : 0;                           \
    size_t len_b = b ? rt_v2_data_array_length((void *)b) : 0;                           \
    size_t total = len_a + len_b;                                                \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * sizeof(elem_type);   \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = total;                                                          \
    meta->capacity = total;                                                      \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    if (len_a > 0) memcpy(arr, a, len_a * sizeof(elem_type));                    \
    if (len_b > 0) memcpy(arr + len_a, b, len_b * sizeof(elem_type));            \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_CONCAT_V2(long, long long)
DEFINE_ARRAY_CONCAT_V2(double, double)
DEFINE_ARRAY_CONCAT_V2(char, char)
DEFINE_ARRAY_CONCAT_V2(bool, int)
DEFINE_ARRAY_CONCAT_V2(byte, unsigned char)
DEFINE_ARRAY_CONCAT_V2(int32, int32_t)
DEFINE_ARRAY_CONCAT_V2(uint32, uint32_t)
DEFINE_ARRAY_CONCAT_V2(uint, uint64_t)
DEFINE_ARRAY_CONCAT_V2(float, float)

/* String concat - need to convert char** to RtHandleV2* elements */
RtHandleV2 *rt_array_concat_string_v2(RtArenaV2 *arena, RtHandleV2 **a, RtHandleV2 **b) {
    size_t len_a = a ? rt_v2_data_array_length((void *)a) : 0;
    size_t len_b = b ? rt_v2_data_array_length((void *)b) : 0;
    size_t total = len_a + len_b;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = total;
    meta->capacity = total;

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    /* Clone each string handle from source arrays to new arena */
    for (size_t i = 0; i < len_a; i++) {
        const char *str = a[i] ? (const char *)rt_handle_v2_pin(a[i]) : "";
        arr[i] = rt_arena_v2_strdup(arena, str);
        if (a[i]) rt_handle_v2_unpin(a[i]);
    }
    for (size_t i = 0; i < len_b; i++) {
        const char *str = b[i] ? (const char *)rt_handle_v2_pin(b[i]) : "";
        arr[len_a + i] = rt_arena_v2_strdup(arena, str);
        if (b[i]) rt_handle_v2_unpin(b[i]);
    }

    rt_handle_v2_unpin(h);
    return h;
}

RtHandleV2 *rt_array_concat_ptr_v2(RtArenaV2 *arena, void **a, void **b) {
    size_t len_a = a ? rt_v2_data_array_length(a) : 0;
    size_t len_b = b ? rt_v2_data_array_length(b) : 0;
    size_t total = len_a + len_b;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = total;
    meta->capacity = total;

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    if (len_a > 0) memcpy(arr, a, len_a * sizeof(RtHandleV2 *));
    if (len_b > 0) memcpy(arr + len_a, b, len_b * sizeof(RtHandleV2 *));

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Slice Functions
 * ============================================================================ */

static inline long normalize_index(long idx, size_t len) {
    if (idx < 0) idx += (long)len;
    if (idx < 0) idx = 0;
    if (idx > (long)len) idx = (long)len;
    return idx;
}

#define DEFINE_ARRAY_SLICE_V2(suffix, elem_type)                                 \
RtHandleV2 *rt_array_slice_##suffix##_v2(RtArenaV2 *arena, const elem_type *arr, \
                                          long start, long end, long step) {     \
    if (arr == NULL) return rt_array_create_##suffix##_v2(arena, 0, NULL);       \
    size_t len = rt_v2_data_array_length((void *)arr);                           \
    /* LONG_MIN means "use default" */                                           \
    if (step == LONG_MIN || step == 0) step = 1;                                 \
    if (start == LONG_MIN) start = step > 0 ? 0 : (long)len - 1;                 \
    if (end == LONG_MIN) end = step > 0 ? (long)len : -1;                        \
    start = normalize_index(start, len);                                         \
    end = normalize_index(end, len);                                             \
    size_t count = 0;                                                            \
    if (step > 0 && start < end) {                                               \
        count = (end - start + step - 1) / step;                                 \
    } else if (step < 0 && start > end) {                                        \
        count = (start - end - step - 1) / (-step);                              \
    }                                                                            \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(elem_type);   \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = count;                                                          \
    meta->capacity = count;                                                      \
    elem_type *result = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));  \
    size_t j = 0;                                                                \
    if (step > 0) {                                                              \
        for (long i = start; i < end && j < count; i += step) {                  \
            result[j++] = arr[i];                                                \
        }                                                                        \
    } else {                                                                     \
        for (long i = start; i > end && j < count; i += step) {                  \
            result[j++] = arr[i];                                                \
        }                                                                        \
    }                                                                            \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_SLICE_V2(long, long long)
DEFINE_ARRAY_SLICE_V2(double, double)
DEFINE_ARRAY_SLICE_V2(char, char)
DEFINE_ARRAY_SLICE_V2(bool, int)
DEFINE_ARRAY_SLICE_V2(byte, unsigned char)
DEFINE_ARRAY_SLICE_V2(int32, int32_t)
DEFINE_ARRAY_SLICE_V2(uint32, uint32_t)
DEFINE_ARRAY_SLICE_V2(uint, uint64_t)
DEFINE_ARRAY_SLICE_V2(float, float)

/* String slice - takes V2 string handle array, returns new handle array.
 * Input arr is RtHandleV2** where each element is a string handle. */
RtHandleV2 *rt_array_slice_string_v2(RtArenaV2 *arena, RtHandleV2 **arr,
                                      long start, long end, long step) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    /* LONG_MIN means "use default" */
    if (step == LONG_MIN || step == 0) step = 1;
    if (start == LONG_MIN) start = step > 0 ? 0 : (long)len - 1;
    if (end == LONG_MIN) end = step > 0 ? (long)len : -1;
    start = normalize_index(start, len);
    end = normalize_index(end, len);

    size_t count = 0;
    if (step > 0 && start < end) {
        count = (end - start + step - 1) / step;
    } else if (step < 0 && start > end) {
        count = (start - end - step - 1) / (-step);
    }

    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    size_t j = 0;
    if (step > 0) {
        for (long i = start; i < end && j < count; i += step) {
            /* Clone the string handle to the new arena */
            if (arr[i] != NULL) {
                const char *s = (const char *)rt_handle_v2_pin(arr[i]);
                result[j++] = rt_arena_v2_strdup(arena, s);
                rt_handle_v2_unpin(arr[i]);
            } else {
                result[j++] = rt_arena_v2_strdup(arena, "");
            }
        }
    } else {
        for (long i = start; i > end && j < count; i += step) {
            /* Clone the string handle to the new arena */
            if (arr[i] != NULL) {
                const char *s = (const char *)rt_handle_v2_pin(arr[i]);
                result[j++] = rt_arena_v2_strdup(arena, s);
                rt_handle_v2_unpin(arr[i]);
            } else {
                result[j++] = rt_arena_v2_strdup(arena, "");
            }
        }
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================ */

#define DEFINE_ARRAY_REV_V2(suffix, elem_type)                                   \
RtHandleV2 *rt_array_rev_##suffix##_v2(RtArenaV2 *arena, const elem_type *arr) { \
    if (arr == NULL) return rt_array_create_##suffix##_v2(arena, 0, NULL);       \
    size_t len = rt_v2_data_array_length((void *)arr);                                   \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(elem_type);     \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = len;                                                            \
    meta->capacity = len;                                                        \
    elem_type *result = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));  \
    for (size_t i = 0; i < len; i++) {                                           \
        result[i] = arr[len - 1 - i];                                            \
    }                                                                            \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_REV_V2(long, long long)
DEFINE_ARRAY_REV_V2(double, double)
DEFINE_ARRAY_REV_V2(char, char)
DEFINE_ARRAY_REV_V2(bool, int)
DEFINE_ARRAY_REV_V2(byte, unsigned char)

/* String reverse - takes raw C string array */
RtHandleV2 *rt_array_rev_string_v2(RtArenaV2 *arena, const char **arr) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = len;
    meta->capacity = len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < len; i++) {
        result[i] = rt_arena_v2_strdup(arena, arr[len - 1 - i] ? arr[len - 1 - i] : "");
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* String reverse handle - takes handle array (for V2 string array mutations) */
RtHandleV2 *rt_array_rev_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = len;
    meta->capacity = len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < len; i++) {
        result[i] = arr[len - 1 - i];  /* Keep existing handle, just reorder */
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* String clone data - clone array of string handles (data pointer variant) */
RtHandleV2 *rt_array_clone_string_data_v2(RtArenaV2 *arena, RtHandleV2 **arr) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = len;
    meta->capacity = len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < len; i++) {
        result[i] = arr[i];  /* Keep existing handles */
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================ */

#define DEFINE_ARRAY_REM_V2(suffix, elem_type)                                   \
RtHandleV2 *rt_array_rem_##suffix##_v2(RtArenaV2 *arena, const elem_type *arr,   \
                                        long index) {                            \
    if (arr == NULL) return rt_array_create_##suffix##_v2(arena, 0, NULL);       \
    size_t len = rt_v2_data_array_length((void *)arr);                                   \
    if (index < 0) index += (long)len;                                           \
    if (index < 0 || index >= (long)len) {                                       \
        return rt_array_clone_##suffix##_v2(arena, arr);                         \
    }                                                                            \
    size_t new_len = len - 1;                                                    \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(elem_type); \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = new_len;                                                        \
    meta->capacity = new_len;                                                    \
    elem_type *result = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));  \
    memcpy(result, arr, index * sizeof(elem_type));                              \
    memcpy(result + index, arr + index + 1, (len - index - 1) * sizeof(elem_type));\
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_REM_V2(long, long long)
DEFINE_ARRAY_REM_V2(double, double)
DEFINE_ARRAY_REM_V2(char, char)
DEFINE_ARRAY_REM_V2(bool, int)
DEFINE_ARRAY_REM_V2(byte, unsigned char)

/* String remove - takes raw C string array */
RtHandleV2 *rt_array_rem_string_v2(RtArenaV2 *arena, const char **arr, long index) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    if (index < 0) index += (long)len;
    if (index < 0 || index >= (long)len) {
        return rt_array_clone_string_v2(arena, arr);
    }

    size_t new_len = len - 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (i != (size_t)index) {
            result[j++] = rt_arena_v2_strdup(arena, arr[i] ? arr[i] : "");
        }
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* String remove handle - takes handle array (for V2 string array mutations) */
RtHandleV2 *rt_array_rem_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr, long index) {
    if (arr == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t len = rt_v2_data_array_length((void *)arr);
    if (index < 0) index += (long)len;
    if (index < 0 || index >= (long)len) {
        return rt_array_clone_string_data_v2(arena, arr);
    }

    size_t new_len = len - 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (i != (size_t)index) {
            result[j++] = arr[i];  /* Keep existing handle */
        }
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================ */

#define DEFINE_ARRAY_INS_V2(suffix, elem_type)                                   \
RtHandleV2 *rt_array_ins_##suffix##_v2(RtArenaV2 *arena, const elem_type *arr,   \
                                        elem_type elem, long index) {            \
    size_t len = arr ? rt_v2_data_array_length((void *)arr) : 0;                         \
    if (index < 0) index += (long)len + 1;                                       \
    if (index < 0) index = 0;                                                    \
    if (index > (long)len) index = (long)len;                                    \
    size_t new_len = len + 1;                                                    \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(elem_type); \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = new_len;                                                        \
    meta->capacity = new_len;                                                    \
    elem_type *result = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));  \
    if (arr) {                                                                   \
        memcpy(result, arr, index * sizeof(elem_type));                          \
        memcpy(result + index + 1, arr + index, (len - index) * sizeof(elem_type));\
    }                                                                            \
    result[index] = elem;                                                        \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_INS_V2(long, long long)
DEFINE_ARRAY_INS_V2(double, double)
DEFINE_ARRAY_INS_V2(char, char)
DEFINE_ARRAY_INS_V2(bool, int)
DEFINE_ARRAY_INS_V2(byte, unsigned char)

/* String insert - takes raw C string array (for creation from literals) */
RtHandleV2 *rt_array_ins_string_v2(RtArenaV2 *arena, const char **arr,
                                    const char *elem, long index) {
    size_t len = arr ? rt_v2_data_array_length((void *)arr) : 0;
    if (index < 0) index += (long)len + 1;
    if (index < 0) index = 0;
    if (index > (long)len) index = (long)len;

    size_t new_len = len + 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (long i = 0; i < index && arr; i++) {
        result[i] = rt_arena_v2_strdup(arena, arr[i] ? arr[i] : "");
    }
    result[index] = rt_arena_v2_strdup(arena, elem ? elem : "");
    for (size_t i = index; i < len && arr; i++) {
        result[i + 1] = rt_arena_v2_strdup(arena, arr[i] ? arr[i] : "");
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* String insert handle - takes handle array (for V2 string array mutations) */
RtHandleV2 *rt_array_ins_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr,
                                           const char *elem, long index) {
    size_t len = arr ? rt_v2_data_array_length((void *)arr) : 0;
    if (index < 0) index += (long)len + 1;
    if (index < 0) index = 0;
    if (index > (long)len) index = (long)len;

    size_t new_len = len + 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    /* Copy existing handles before insertion point */
    for (long i = 0; i < index && arr; i++) {
        result[i] = arr[i];  /* Keep existing handle */
    }
    /* Insert new element */
    result[index] = rt_arena_v2_strdup(arena, elem ? elem : "");
    /* Copy remaining handles after insertion point */
    for (size_t i = index; i < len && arr; i++) {
        result[i + 1] = arr[i];  /* Keep existing handle */
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Push Copy Functions (non-mutating)
 * ============================================================================ */

#define DEFINE_ARRAY_PUSH_COPY_V2(suffix, elem_type)                             \
RtHandleV2 *rt_array_push_copy_##suffix##_v2(RtArenaV2 *arena,                   \
                                              const elem_type *arr, elem_type elem) {\
    return rt_array_ins_##suffix##_v2(arena, arr, elem, arr ? (long)rt_v2_data_array_length((void *)arr) : 0);\
}

DEFINE_ARRAY_PUSH_COPY_V2(long, long long)
DEFINE_ARRAY_PUSH_COPY_V2(double, double)
DEFINE_ARRAY_PUSH_COPY_V2(char, char)
DEFINE_ARRAY_PUSH_COPY_V2(bool, int)
DEFINE_ARRAY_PUSH_COPY_V2(byte, unsigned char)

RtHandleV2 *rt_array_push_copy_string_v2(RtArenaV2 *arena, const char **arr, const char *elem) {
    return rt_array_ins_string_v2(arena, arr, elem, arr ? (long)rt_v2_data_array_length((void *)arr) : 0);
}

/* ============================================================================
 * Array Alloc Functions (with default value)
 * ============================================================================ */

#define DEFINE_ARRAY_ALLOC_V2(suffix, elem_type)                                 \
RtHandleV2 *rt_array_alloc_##suffix##_v2(RtArenaV2 *arena, size_t count,         \
                                          elem_type default_value) {             \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(elem_type);   \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = count;                                                          \
    meta->capacity = count;                                                      \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    for (size_t i = 0; i < count; i++) {                                         \
        arr[i] = default_value;                                                  \
    }                                                                            \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

DEFINE_ARRAY_ALLOC_V2(long, long long)
DEFINE_ARRAY_ALLOC_V2(double, double)
DEFINE_ARRAY_ALLOC_V2(char, char)
DEFINE_ARRAY_ALLOC_V2(bool, int)
DEFINE_ARRAY_ALLOC_V2(byte, unsigned char)

/* String alloc */
RtHandleV2 *rt_array_alloc_string_v2(RtArenaV2 *arena, size_t count, const char *default_value) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        arr[i] = rt_arena_v2_strdup(arena, default_value ? default_value : "");
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Range Function
 * ============================================================================ */

RtHandleV2 *rt_array_range_v2(RtArenaV2 *arena, long long start, long long end) {
    size_t count = 0;
    if (end > start) {
        count = (size_t)(end - start);
    }

    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(long long);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long long *arr = (long long *)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        arr[i] = start + (long long)i;
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array from raw strings
 * ============================================================================ */

RtHandleV2 *rt_array_from_raw_strings_v2(RtArenaV2 *arena, const char **src) {
    if (src == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t count = rt_v2_data_array_length((void *)src);
    return rt_array_create_string_v2(arena, count, src);
}

/* Convert legacy char** array (from native functions) to V2 handle-based string array */
RtHandleV2 *rt_array_from_legacy_string_v2(RtArenaV2 *arena, char **src) {
    if (src == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t count = rt_v2_data_array_length((void *)src);
    return rt_array_create_string_v2(arena, count, (const char **)src);
}

/* ============================================================================
 * String Array Pin for Native Interop
 * ============================================================================
 * Converts a V2 string array (RtHandleV2* with RtHandleV2* string elements)
 * to a legacy char** for use with native C functions.
 * Each element is pinned to extract the raw char*.
 * ============================================================================ */

char **rt_pin_string_array_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    void *raw = rt_handle_v2_pin(arr_h);
    if (raw == NULL) return NULL;

    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    size_t count = meta->size;

    /* Get pointer to the array of string handles */
    RtHandleV2 **handles = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    /* Allocate a char** array from the arena (for compatibility with native functions) */
    RtArenaV2 *arena = rt_handle_v2_arena(arr_h);
    if (arena == NULL) return NULL;

    /* Allocate space for the char** result with null terminator */
    size_t alloc_size = (count + 1) * sizeof(char *);
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, alloc_size);
    char **result = (char **)rt_handle_v2_ptr(result_h);

    /* Pin each string element to extract char* */
    for (size_t i = 0; i < count; i++) {
        if (handles[i] != NULL) {
            result[i] = (char *)rt_handle_v2_pin(handles[i]);
        } else {
            result[i] = NULL;
        }
    }
    result[count] = NULL; /* Null terminator */

    return result;
}

/* ============================================================================
 * Args Creation
 * ============================================================================ */

RtHandleV2 *rt_args_create_v2(RtArenaV2 *arena, int argc, char **argv) {
    if (argc <= 0 || argv == NULL) {
        return rt_array_create_string_v2(arena, 0, NULL);
    }
    return rt_array_create_string_v2(arena, (size_t)argc, (const char **)argv);
}

/* ============================================================================
 * String Array Equality
 * ============================================================================ */

int rt_array_eq_string_v2(RtHandleV2 *a_h, RtHandleV2 *b_h) {
    if (a_h == NULL && b_h == NULL) return 1;
    if (a_h == NULL || b_h == NULL) return 0;

    size_t len_a = rt_array_length_v2(a_h);
    size_t len_b = rt_array_length_v2(b_h);
    if (len_a != len_b) return 0;

    void *raw_a = rt_handle_v2_pin(a_h);
    void *raw_b = rt_handle_v2_pin(b_h);

    RtHandleV2 **arr_a = (RtHandleV2 **)((char *)raw_a + sizeof(RtArrayMetadataV2));
    RtHandleV2 **arr_b = (RtHandleV2 **)((char *)raw_b + sizeof(RtArrayMetadataV2));

    int equal = 1;
    for (size_t i = 0; i < len_a; i++) {
        const char *str_a = arr_a[i] ? (const char *)rt_handle_v2_ptr(arr_a[i]) : "";
        const char *str_b = arr_b[i] ? (const char *)rt_handle_v2_ptr(arr_b[i]) : "";
        if (strcmp(str_a, str_b) != 0) {
            equal = 0;
            break;
        }
    }

    rt_handle_v2_unpin(b_h);
    rt_handle_v2_unpin(a_h);

    return equal;
}

/* ============================================================================
 * Deep Array Promotion
 * ============================================================================
 * V2 promotion is simpler - handle carries its arena reference!
 * No source arena parameter needed.
 * ============================================================================ */

/* Promote str[] - promotes array AND all string elements */
RtHandleV2 *rt_promote_array_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    /* Get source arena from handle */
    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;  /* Already in dest arena */

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Promote each string element */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_arena_v2_promote(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);

    /* Mark old array as dead */
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote T[][] - promotes outer array AND all inner array handles */
RtHandleV2 *rt_promote_array_handle_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Promote each inner array handle */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_arena_v2_promote(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote T[][][] - promotes all three levels of handles */
RtHandleV2 *rt_promote_array_handle_3d_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each 2D inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array_handle_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote str[][] - promotes outer, inner arrays, AND strings */
RtHandleV2 *rt_promote_array2_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each str[] inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array_string_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote str[][][] - promotes all three levels AND strings */
RtHandleV2 *rt_promote_array3_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each str[][] inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array2_string_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* ============================================================================
 * String Array Search Functions (V2)
 * ============================================================================ */

/**
 * Find the index of an element in a string array (V2).
 * Takes array data pointer (array of string handles) and element to find.
 * Returns index if found, -1 if not found.
 */
long rt_array_indexOf_string_v2(RtHandleV2 **arr, const char *elem) {
    if (arr == NULL) return -1;

    /* Get length from metadata (metadata is before the data pointer) */
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)((char *)arr - sizeof(RtArrayMetadataV2));
    size_t len = meta->size;

    for (size_t i = 0; i < len; i++) {
        /* Pin each string handle to compare */
        const char *str = (const char *)rt_handle_v2_pin(arr[i]);
        if (str != NULL && elem != NULL && strcmp(str, elem) == 0) {
            rt_handle_v2_unpin(arr[i]);
            return (long)i;
        }
        if (str == NULL && elem == NULL) {
            return (long)i;
        }
        rt_handle_v2_unpin(arr[i]);
    }
    return -1;
}

/**
 * Check if a string array contains an element (V2).
 */
int rt_array_contains_string_v2(RtHandleV2 **arr, const char *elem) {
    return rt_array_indexOf_string_v2(arr, elem) >= 0;
}

/* ============================================================================
 * Array Join Functions (V2)
 * ============================================================================
 * Join array elements into a string with separator.
 * These take raw data pointers (after pinning array handle) and use V2 arena.
 * ============================================================================ */

/* Helper to get length from raw V2 array data pointer */
static inline size_t get_array_len_from_data(const void *arr) {
    if (arr == NULL) return 0;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)((char *)arr - sizeof(RtArrayMetadataV2));
    return meta->size;
}

char *rt_array_join_long_v2(RtArenaV2 *arena, const long long *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lld", arr[i]);
    }
    return result;
}

char *rt_array_join_double_v2(RtArenaV2 *arena, const double *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", arr[i]);
    }
    return result;
}

char *rt_array_join_char_v2(RtArenaV2 *arena, const char *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        *ptr++ = arr[i];
    }
    *ptr = '\0';
    return result;
}

char *rt_array_join_bool_v2(RtArenaV2 *arena, const int *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%s", arr[i] ? "true" : "false");
    }
    return result;
}

char *rt_array_join_byte_v2(RtArenaV2 *arena, const unsigned char *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "0xXX" (4 chars) + separators */
    size_t buf_size = len * 4 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "0x%02X", arr[i]);
    }
    return result;
}

char *rt_array_join_int32_v2(RtArenaV2 *arena, const int32_t *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each int32 can be up to 11 chars + separators */
    size_t buf_size = len * 12 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%d", arr[i]);
    }
    return result;
}

char *rt_array_join_uint32_v2(RtArenaV2 *arena, const uint32_t *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint32 can be up to 10 chars + separators */
    size_t buf_size = len * 11 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%u", arr[i]);
    }
    return result;
}

char *rt_array_join_uint_v2(RtArenaV2 *arena, const uint64_t *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint64 can be up to 20 chars + separators */
    size_t buf_size = len * 21 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lu", (unsigned long)arr[i]);
    }
    return result;
}

char *rt_array_join_float_v2(RtArenaV2 *arena, const float *arr, const char *separator) {
    if (arr == NULL || get_array_len_from_data(arr) == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", (double)arr[i]);
    }
    return result;
}

char *rt_array_join_string_v2(RtArenaV2 *arena, RtHandleV2 **arr, const char *separator) {
    if (arr == NULL) {
        /* Return empty string for NULL array */
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t len = get_array_len_from_data(arr);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }

    /* Calculate total size needed */
    size_t sep_len = separator ? strlen(separator) : 0;
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != NULL) {
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            if (s) total_len += strlen(s);
            rt_handle_v2_unpin(arr[i]);
        }
        if (i < len - 1) total_len += sep_len;
    }

    /* Allocate result buffer */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    if (!handle) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    char *result = (char *)rt_handle_v2_pin(handle);

    /* Build result string */
    char *p = result;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != NULL) {
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            if (s) {
                size_t slen = strlen(s);
                memcpy(p, s, slen);
                p += slen;
            }
            rt_handle_v2_unpin(arr[i]);
        }
        if (i < len - 1 && separator) {
            memcpy(p, separator, sep_len);
            p += sep_len;
        }
    }
    *p = '\0';

    return result;
}

/* ============================================================================
 * String Array toString V2
 * ============================================================================ */

char *rt_to_string_array_string_v2(RtArenaV2 *arena, RtHandleV2 **arr) {
    if (arr == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    /* Calculate total length needed */
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        if (i > 0) total_len += 2; /* ", " */
        if (arr[i] != NULL) {
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            total_len += strlen(s) + 2; /* "str" */
            rt_handle_v2_unpin(arr[i]);
        } else {
            total_len += 4; /* null */
        }
    }

    /* Allocate result buffer */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    if (!handle) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }
    char *result = (char *)rt_handle_v2_pin(handle);

    /* Build result string */
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        if (arr[i] != NULL) {
            *p++ = '"';
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            while (*s) *p++ = *s++;
            rt_handle_v2_unpin(arr[i]);
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

/* ============================================================================
 * 2D Array toString V2 Functions
 * ============================================================================ */

/* Helper: generic 1D array to string */
static char *rt_to_string_array1_v2_generic(RtArenaV2 *arena, void *arr, size_t elem_size,
                                             const char *(*elem_fmt)(void *elem, char *buf, size_t buflen)) {
    if (arr == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t len = rt_v2_data_array_length(arr);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    /* Build each element string */
    char **elem_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, len * sizeof(char *)));
    size_t total_len = 2; /* {} */
    char buf[64];
    for (size_t i = 0; i < len; i++) {
        void *elem = (char *)arr + i * elem_size;
        const char *s = elem_fmt(elem, buf, sizeof(buf));
        elem_strs[i] = (char *)rt_handle_v2_pin(rt_arena_v2_strdup(arena, s));
        if (i > 0) total_len += 2;
        total_len += strlen(elem_strs[i]);
    }

    /* Allocate and build result */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

/* Element formatters */
static const char *fmt_long(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%lld", *(long long *)elem);
    return buf;
}
static const char *fmt_double(void *elem, char *buf, size_t buflen) {
    double d = *(double *)elem;
    if (d == (long long)d) {
        snprintf(buf, buflen, "%lld", (long long)d);
    } else {
        snprintf(buf, buflen, "%g", d);
    }
    return buf;
}
static const char *fmt_char(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "'%c'", *(char *)elem);
    return buf;
}
static const char *fmt_bool(void *elem, char *buf, size_t buflen) {
    (void)buflen;
    return *(int *)elem ? "true" : "false";
}
static const char *fmt_byte(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%u", *(unsigned char *)elem);
    return buf;
}

/* 2D array toString: iterate outer, call 1D on each inner */
char *rt_to_string_array2_long_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        long long *inner = outer[i] ? (long long *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array1_v2_generic(arena, inner, sizeof(long long), fmt_long);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array2_double_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        double *inner = outer[i] ? (double *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array1_v2_generic(arena, inner, sizeof(double), fmt_double);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array2_char_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        char *inner = outer[i] ? (char *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array1_v2_generic(arena, inner, sizeof(char), fmt_char);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array2_bool_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        int *inner = outer[i] ? (int *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array1_v2_generic(arena, inner, sizeof(int), fmt_bool);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array2_byte_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        unsigned char *inner = outer[i] ? (unsigned char *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array1_v2_generic(arena, inner, sizeof(unsigned char), fmt_byte);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array2_string_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array_string_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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
 * 3D Array ToString Functions
 * ============================================================================
 * 3D arrays: iterate outermost, call 2D on each middle layer.
 * ============================================================================ */

char *rt_to_string_array3_long_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_long_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_double_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_double_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_char_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_char_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_bool_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_bool_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_byte_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_byte_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_string_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_string_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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
 * 1D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed V2 arrays to any[] handles by boxing each element.
 * Takes raw data pointer (from rt_array_data_v2), returns V2 handle.
 */

RtHandleV2 *rt_array_to_any_long_v2(RtArenaV2 *arena, const long long *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_long(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_double_v2(RtArenaV2 *arena, const double *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_double(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_char_v2(RtArenaV2 *arena, const char *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_char(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_bool_v2(RtArenaV2 *arena, const int *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_bool(arr[i] != 0));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_byte_v2(RtArenaV2 *arena, const unsigned char *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_byte(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        const char *s = arr[i] ? (const char *)rt_handle_v2_pin(arr[i]) : "";
        result = rt_array_push_any_v2(arena, result, rt_box_string(s));
        if (arr[i]) rt_handle_v2_unpin(arr[i]);
    }
    return result;
}

/* ============================================================================
 * 2D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 2D V2 arrays to any[][] handles.
 * Takes outer handle, returns handle to array of any-array handles.
 */

RtHandleV2 *rt_array2_to_any_long_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        long long *inner_data = inner_handles[i] ? (long long *)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_long_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_double_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        double *inner_data = inner_handles[i] ? (double *)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_double_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_char_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        char *inner_data = inner_handles[i] ? (char *)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_char_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_bool_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        int *inner_data = inner_handles[i] ? (int *)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_bool_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_byte_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        unsigned char *inner_data = inner_handles[i] ? (unsigned char *)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_byte_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 **inner_data = inner_handles[i] ? (RtHandleV2 **)rt_array_data_v2(inner_handles[i]) : NULL;
        RtHandleV2 *any_h = rt_array_to_any_string_v2(arena, inner_data);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_h);
    }
    return result;
}

/* ============================================================================
 * 3D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 3D V2 arrays to any[][][] handles.
 */

RtHandleV2 *rt_array3_to_any_long_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_long_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_double_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_double_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_char_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_char_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_bool_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_bool_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_byte_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_byte_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_string_v2(arena, inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, (void *)any_2d_h);
    }
    return result;
}

/* ============================================================================
 * Any Array toString V2 Functions
 * ============================================================================ */

char *rt_to_string_array_any_v2(RtArenaV2 *arena, const RtAny *arr) {
    if (arr == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t len = rt_v2_data_array_length((void *)arr);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    /* First pass: convert each element to string and calculate total length */
    char **elem_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, len * sizeof(char *)));
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        elem_strs[i] = rt_any_to_string((RtArena *)arena, arr[i]);
        if (i > 0) total_len += 2; /* ", " */
        total_len += strlen(elem_strs[i]);
    }

    /* Build result string */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);

    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_any_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtAny *inner = outer[i] ? (RtAny *)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array_any_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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

char *rt_to_string_array3_any_v2(RtArenaV2 *arena, RtHandleV2 **outer) {
    if (outer == NULL) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    size_t outer_len = rt_v2_data_array_length((void *)outer);
    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        return (char *)rt_handle_v2_pin(h);
    }

    char **inner_strs = (char **)rt_handle_v2_pin(rt_arena_v2_alloc(arena, outer_len * sizeof(char *)));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 **inner = outer[i] ? (RtHandleV2 **)rt_array_data_v2(outer[i]) : NULL;
        inner_strs[i] = rt_to_string_array2_any_v2(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    char *result = (char *)rt_handle_v2_pin(handle);
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
 * Print Functions V2
 * ============================================================================ */

void rt_print_array_string_v2(RtHandleV2 **arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_v2_data_array_length((void *)arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            if (arr[i] != NULL) {
                const char *s = (const char *)rt_handle_v2_pin(arr[i]);
                printf("\"%s\"", s);
            } else {
                printf("null");
            }
        }
    }
    printf("]");
}

