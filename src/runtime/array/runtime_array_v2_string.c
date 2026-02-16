/*
 * Runtime Array V2 String Operations
 * ===================================
 * String-specific array operations including push, pop, clone, concat,
 * slice, reverse, remove, insert, and utility functions.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * Array Push Functions
 * ============================================================================ */

/* String push -- stores element as RtHandleV2* */
RtHandleV2 *rt_array_push_string_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element) {
    /* Strdup element into target arena for ownership safety */
    RtHandleV2 *elem_h;
    if (element) {
        rt_handle_begin_transaction(element);
        const char *str = (const char *)element->ptr;
        elem_h = rt_arena_v2_strdup(arena, str ? str : "");
        rt_handle_end_transaction(element);
    } else {
        elem_h = rt_arena_v2_strdup(arena, "");
    }

    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        rt_handle_begin_transaction(new_h);
        void *new_raw = new_h->ptr;
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        meta->element_size = sizeof(RtHandleV2 *);
        arr[0] = elem_h;
        rt_handle_end_transaction(new_h);
        rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
        rt_handle_set_free_callback(new_h, rt_array_free_callback);
        return new_h;
    }

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_handle_end_transaction(arr_h);
        return arr_h;
    }

    /* Need to grow */
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(new_h);
    void *new_raw = new_h->ptr;
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtHandleV2 *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_meta->element_size = sizeof(RtHandleV2 *);
    new_arr[old_size] = elem_h;

    rt_handle_end_transaction(new_h);
    rt_handle_end_transaction(arr_h);
    rt_arena_v2_free(arr_h);

    rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
    rt_handle_set_free_callback(new_h, rt_array_free_callback);
    return new_h;
}

/* Pointer (nested array) push -- stores element as RtHandleV2* */
RtHandleV2 *rt_array_push_ptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element) {
    RtHandleV2 *elem_h = element;

    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        rt_handle_begin_transaction(new_h);
        void *new_raw = new_h->ptr;
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        meta->element_size = sizeof(RtHandleV2 *);
        arr[0] = elem_h;
        rt_handle_end_transaction(new_h);
        rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
        rt_handle_set_free_callback(new_h, rt_array_free_callback);
        return new_h;
    }

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = elem_h;
        rt_handle_end_transaction(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(new_h);
    void *new_raw = new_h->ptr;
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtHandleV2 *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_meta->element_size = sizeof(RtHandleV2 *);
    new_arr[old_size] = elem_h;

    rt_handle_end_transaction(new_h);
    rt_handle_end_transaction(arr_h);
    rt_arena_v2_free(arr_h);

    rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
    rt_handle_set_free_callback(new_h, rt_array_free_callback);
    return new_h;
}

/* Void pointer push -- stores element as full RtHandleV2* (pointer size) */
RtHandleV2 *rt_array_push_voidptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        rt_handle_begin_transaction(new_h);
        void *new_raw = new_h->ptr;
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        meta->element_size = sizeof(RtHandleV2 *);
        arr[0] = element;
        rt_handle_end_transaction(new_h);
        rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
        rt_handle_set_free_callback(new_h, rt_array_free_callback);
        return new_h;
    }

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_handle_end_transaction(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(new_h);
    void *new_raw = new_h->ptr;
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtHandleV2 *));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_meta->element_size = sizeof(RtHandleV2 *);
    new_arr[old_size] = element;

    rt_handle_end_transaction(new_h);
    rt_handle_end_transaction(arr_h);
    rt_arena_v2_free(arr_h);

    rt_handle_set_copy_callback(new_h, rt_array_copy_callback);
    rt_handle_set_free_callback(new_h, rt_array_free_callback);
    return new_h;
}

/* Push RtAny element */
RtHandleV2 *rt_array_push_any_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtAny element) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtAny);
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        rt_handle_begin_transaction(new_h);
        void *new_raw = new_h->ptr;
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        RtAny *arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadataV2));
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        meta->element_size = sizeof(RtAny);
        arr[0] = element;
        rt_handle_end_transaction(new_h);
        rt_handle_set_copy_callback(new_h, rt_array_any_copy_callback);
        rt_handle_set_free_callback(new_h, rt_array_any_free_callback);
        return new_h;
    }

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    RtAny *arr = (RtAny *)((char *)raw + sizeof(RtArrayMetadataV2));

    if (meta->size < meta->capacity) {
        arr[meta->size++] = element;
        rt_handle_end_transaction(arr_h);
        return arr_h;
    }

    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * sizeof(RtAny);

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(new_h);
    void *new_raw = new_h->ptr;
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    RtAny *new_arr = (RtAny *)((char *)new_raw + sizeof(RtArrayMetadataV2));

    memcpy(new_arr, arr, old_size * sizeof(RtAny));
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_meta->element_size = sizeof(RtAny);
    new_arr[old_size] = element;

    rt_handle_end_transaction(new_h);
    rt_handle_end_transaction(arr_h);
    rt_arena_v2_free(arr_h);

    rt_handle_set_copy_callback(new_h, rt_array_any_copy_callback);
    rt_handle_set_free_callback(new_h, rt_array_any_free_callback);
    return new_h;
}

/* ============================================================================
 * Array Pop Functions
 * ============================================================================ */

#define DEFINE_ARRAY_POP_V2(suffix, elem_type, default_val)                      \
elem_type rt_array_pop_##suffix##_v2(RtHandleV2 *arr_h) {                        \
    if (arr_h == NULL) return default_val;                                       \
    rt_handle_begin_transaction(arr_h);                                                     \
    void *raw = arr_h->ptr;                                                      \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    if (meta->size == 0) {                                                       \
        rt_handle_end_transaction(arr_h);                                               \
        return default_val;                                                      \
    }                                                                            \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    elem_type result = arr[--meta->size];                                        \
    rt_handle_end_transaction(arr_h);                                                   \
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
DEFINE_ARRAY_POP_V2(ptr, RtHandleV2 *, NULL)

/* String pop returns handle */
RtHandleV2 *rt_array_pop_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    if (meta->size == 0) {
        rt_handle_end_transaction(arr_h);
        return NULL;
    }
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 *result = arr[--meta->size];
    rt_handle_end_transaction(arr_h);
    return result;
}

/* ============================================================================
 * Array Clone Functions
 * ============================================================================ */

/* Clone RtAny array */
RtHandleV2 *rt_array_clone_any_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    size_t count = rt_array_length_v2(arr_h);
    const RtAny *src = (const RtAny *)rt_array_data_v2(arr_h);
    RtHandleV2 *result = rt_array_create_generic_v2(arena, count, sizeof(RtAny), src);
    rt_handle_end_transaction(arr_h);
    return result;
}

/* Clone string array (V2 string arrays contain RtHandleV2* elements) */
RtHandleV2 *rt_array_clone_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;

    /* Get source array metadata and elements */
    rt_handle_begin_transaction(arr_h);
    size_t count = rt_array_length_v2(arr_h);
    RtHandleV2 **src_elems = (RtHandleV2 **)rt_array_data_v2(arr_h);

    /* Allocate new array with same capacity */
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    meta->element_size = sizeof(RtHandleV2 *);
    RtHandleV2 **dst_elems = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    /* Clone each string handle to the target arena */
    for (size_t i = 0; i < count; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        if (src_elems[i] != NULL) {
            rt_handle_begin_transaction(src_elems[i]);
            const char *str = (const char *)src_elems[i]->ptr;
            dst_elems[i] = rt_arena_v2_strdup(arena, str);
            rt_handle_end_transaction(src_elems[i]);
        } else {
            dst_elems[i] = NULL;
        }
    }

    rt_handle_end_transaction(arr_h);
    rt_handle_end_transaction(h);
    /* Inherit callbacks from source */
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(arr_h));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(arr_h));
    return h;
}

/* Clone pointer array (nested arrays) */
RtHandleV2 *rt_array_clone_ptr_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    size_t count = rt_array_length_v2(arr_h);
    RtHandleV2 **src = (RtHandleV2 **)rt_array_data_v2(arr_h);
    RtHandleV2 *result = rt_array_create_ptr_v2(arena, count, src);
    rt_handle_end_transaction(arr_h);
    return result;
}

/* ============================================================================
 * Array Concat Functions
 * ============================================================================ */

/* String concat */
RtHandleV2 *rt_array_concat_string_v2(RtHandleV2 *a_h, RtHandleV2 *b_h) {
    if (a_h == NULL && b_h == NULL) return NULL;
    RtArenaV2 *arena = a_h ? a_h->arena : b_h->arena;
    if (a_h) rt_handle_begin_transaction(a_h);
    if (b_h) rt_handle_begin_transaction(b_h);
    size_t len_a = rt_array_length_v2(a_h);
    size_t len_b = rt_array_length_v2(b_h);
    RtHandleV2 **a = len_a ? (RtHandleV2 **)rt_array_data_v2(a_h) : NULL;
    RtHandleV2 **b = len_b ? (RtHandleV2 **)rt_array_data_v2(b_h) : NULL;
    size_t total = len_a + len_b;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = total;
    meta->capacity = total;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    /* Clone each string handle from source arrays to new arena */
    for (size_t i = 0; i < len_a; i++) {
        rt_handle_renew_transaction(h);
        if (a_h) rt_handle_renew_transaction(a_h);
        const char *str;
        if (a[i]) {
            rt_handle_begin_transaction(a[i]);
            str = (const char *)a[i]->ptr;
        } else {
            str = "";
        }
        arr[i] = rt_arena_v2_strdup(arena, str);
        if (a[i]) rt_handle_end_transaction(a[i]);
    }
    for (size_t i = 0; i < len_b; i++) {
        rt_handle_renew_transaction(h);
        if (b_h) rt_handle_renew_transaction(b_h);
        const char *str;
        if (b[i]) {
            rt_handle_begin_transaction(b[i]);
            str = (const char *)b[i]->ptr;
        } else {
            str = "";
        }
        arr[len_a + i] = rt_arena_v2_strdup(arena, str);
        if (b[i]) rt_handle_end_transaction(b[i]);
    }

    rt_handle_end_transaction(h);
    if (b_h) rt_handle_end_transaction(b_h);
    if (a_h) rt_handle_end_transaction(a_h);
    rt_handle_set_copy_callback(h, rt_array_copy_callback);
    rt_handle_set_free_callback(h, rt_array_free_callback);
    return h;
}

/* Pointer (nested array) concat */
RtHandleV2 *rt_array_concat_ptr_v2(RtHandleV2 *a_h, RtHandleV2 *b_h) {
    if (a_h == NULL && b_h == NULL) return NULL;
    RtArenaV2 *arena = a_h ? a_h->arena : b_h->arena;
    if (a_h) rt_handle_begin_transaction(a_h);
    if (b_h) rt_handle_begin_transaction(b_h);
    size_t len_a = rt_array_length_v2(a_h);
    size_t len_b = rt_array_length_v2(b_h);
    RtHandleV2 **a = len_a ? (RtHandleV2 **)rt_array_data_v2(a_h) : NULL;
    RtHandleV2 **b = len_b ? (RtHandleV2 **)rt_array_data_v2(b_h) : NULL;
    size_t total = len_a + len_b;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = total;
    meta->capacity = total;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    if (len_a > 0) memcpy(arr, a, len_a * sizeof(RtHandleV2 *));
    if (len_b > 0) memcpy(arr + len_a, b, len_b * sizeof(RtHandleV2 *));

    rt_handle_end_transaction(h);
    if (b_h) rt_handle_end_transaction(b_h);
    if (a_h) rt_handle_end_transaction(a_h);
    /* Inherit callbacks from first source */
    RtHandleV2 *src = a_h ? a_h : b_h;
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(src));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(src));
    return h;
}

/* ============================================================================
 * Array Slice Functions
 * ============================================================================ */

/* String slice */
RtHandleV2 *rt_array_slice_string_v2(RtHandleV2 *arr_h,
                                      long start, long end, long step) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
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
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    size_t j = 0;
    if (step > 0) {
        for (long i = start; i < end && j < count; i += step) {
            rt_handle_renew_transaction(h);
            rt_handle_renew_transaction(arr_h);
            /* Clone the string handle to the new arena */
            if (arr[i] != NULL) {
                rt_handle_begin_transaction(arr[i]);
                const char *s = (const char *)arr[i]->ptr;
                result[j++] = rt_arena_v2_strdup(arena, s);
                rt_handle_end_transaction(arr[i]);
            } else {
                result[j++] = rt_arena_v2_strdup(arena, "");
            }
        }
    } else {
        for (long i = start; i > end && j < count; i += step) {
            rt_handle_renew_transaction(h);
            rt_handle_renew_transaction(arr_h);
            /* Clone the string handle to the new arena */
            if (arr[i] != NULL) {
                rt_handle_begin_transaction(arr[i]);
                const char *s = (const char *)arr[i]->ptr;
                result[j++] = rt_arena_v2_strdup(arena, s);
                rt_handle_end_transaction(arr[i]);
            } else {
                result[j++] = rt_arena_v2_strdup(arena, "");
            }
        }
    }

    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    /* Inherit callbacks from source */
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(arr_h));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(arr_h));
    return h;
}

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================ */

/* String reverse */
RtHandleV2 *rt_array_rev_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = len;
    meta->capacity = len;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        /* Clone string to new arena in reversed order */
        if (arr[len - 1 - i] != NULL) {
            rt_handle_begin_transaction(arr[len - 1 - i]);
            const char *s = (const char *)arr[len - 1 - i]->ptr;
            result[i] = rt_arena_v2_strdup(arena, s);
            rt_handle_end_transaction(arr[len - 1 - i]);
        } else {
            result[i] = rt_arena_v2_strdup(arena, "");
        }
    }

    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(arr_h));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(arr_h));
    return h;
}

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================ */

/* String remove */
RtHandleV2 *rt_array_rem_string_v2(RtHandleV2 *arr_h, long index) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (index < 0) index += (long)len;
    if (index < 0 || index >= (long)len) {
        rt_handle_end_transaction(arr_h);
        return rt_array_clone_string_v2(arr_h);
    }

    size_t new_len = len - 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;

    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        if (i != (size_t)index) {
            /* Clone string to new arena */
            if (arr[i] != NULL) {
                rt_handle_begin_transaction(arr[i]);
                const char *s = (const char *)arr[i]->ptr;
                result[j++] = rt_arena_v2_strdup(arena, s);
                rt_handle_end_transaction(arr[i]);
            } else {
                result[j++] = rt_arena_v2_strdup(arena, "");
            }
        }
    }

    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(arr_h));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(arr_h));
    return h;
}

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================ */

/* String insert - takes handle element, returns new array handle */
RtHandleV2 *rt_array_ins_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem, long index) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);

    if (index < 0) index += (long)len + 1;
    if (index < 0) index = 0;
    if (index > (long)len) index = (long)len;

    /* Strdup the element into the array's arena */
    RtHandleV2 *elem_h;
    if (elem) {
        rt_handle_begin_transaction(elem);
        const char *str = (const char *)elem->ptr;
        elem_h = rt_arena_v2_strdup(arena, str ? str : "");
        rt_handle_end_transaction(elem);
    } else {
        elem_h = rt_arena_v2_strdup(arena, "");
    }

    size_t new_len = len + 1;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * sizeof(RtHandleV2 *);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **result = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    /* Copy existing handles before insertion point */
    for (long i = 0; i < index && arr; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        result[i] = arr[i];  /* Keep existing handle */
    }
    /* Insert new element */
    result[index] = elem_h;
    /* Copy remaining handles after insertion point */
    for (size_t i = index; i < len && arr; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        result[i + 1] = arr[i];  /* Keep existing handle */
    }

    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    rt_handle_set_copy_callback(h, rt_handle_get_copy_callback(arr_h));
    rt_handle_set_free_callback(h, rt_handle_get_free_callback(arr_h));
    return h;
}

/* ============================================================================
 * Array Push Copy Functions (non-mutating)
 * ============================================================================ */

RtHandleV2 *rt_array_push_copy_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem) {
    if (arr_h == NULL) return NULL;
    return rt_array_ins_string_v2(arr_h, elem, (long)rt_array_length_v2(arr_h));
}

/* ============================================================================
 * String Array Utility Functions
 * ============================================================================ */

/**
 * Find the index of an element in a string array (V2).
 */
long rt_array_indexOf_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem_h) {
    if (arr_h == NULL) return -1;

    /* Extract search string */
    const char *elem = NULL;
    if (elem_h) {
        rt_handle_begin_transaction(elem_h);
        elem = (const char *)elem_h->ptr;
    }

    rt_handle_begin_transaction(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);

    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        if (elem_h) rt_handle_renew_transaction(elem_h);
        /* Pin each string handle to compare */
        rt_handle_begin_transaction(arr[i]);
        const char *str = (const char *)arr[i]->ptr;
        if (str != NULL && elem != NULL && strcmp(str, elem) == 0) {
            rt_handle_end_transaction(arr[i]);
            rt_handle_end_transaction(arr_h);
            if (elem_h) rt_handle_end_transaction(elem_h);
            return (long)i;
        }
        if (str == NULL && elem == NULL) {
            rt_handle_end_transaction(arr[i]);
            rt_handle_end_transaction(arr_h);
            if (elem_h) rt_handle_end_transaction(elem_h);
            return (long)i;
        }
        rt_handle_end_transaction(arr[i]);
    }

    rt_handle_end_transaction(arr_h);
    if (elem_h) rt_handle_end_transaction(elem_h);
    return -1;
}

/**
 * Check if a string array contains an element (V2).
 */
int rt_array_contains_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem) {
    return rt_array_indexOf_string_v2(arr_h, elem) >= 0;
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

    rt_handle_begin_transaction(a_h);
    void *raw_a = a_h->ptr;
    rt_handle_begin_transaction(b_h);
    void *raw_b = b_h->ptr;

    RtHandleV2 **arr_a = (RtHandleV2 **)((char *)raw_a + sizeof(RtArrayMetadataV2));
    RtHandleV2 **arr_b = (RtHandleV2 **)((char *)raw_b + sizeof(RtArrayMetadataV2));

    int equal = 1;
    for (size_t i = 0; i < len_a; i++) {
        rt_handle_renew_transaction(a_h);
        rt_handle_renew_transaction(b_h);
        const char *str_a = arr_a[i] ? (const char *)arr_a[i]->ptr : "";
        const char *str_b = arr_b[i] ? (const char *)arr_b[i]->ptr : "";
        if (strcmp(str_a, str_b) != 0) {
            equal = 0;
            break;
        }
    }

    rt_handle_end_transaction(b_h);
    rt_handle_end_transaction(a_h);

    return equal;
}
