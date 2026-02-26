/*
 * Runtime Array V2 Core Implementation
 * =====================================
 * Core generic array operations that work with any element type.
 * Uses elem_size parameter for type-agnostic operations.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * Internal Helper: Create Array
 * ============================================================================ */

static RtHandleV2 *array_create_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    meta->element_size = elem_size;

    void *arr_data = (char *)raw + sizeof(RtArrayMetadataV2);
    if (data && count > 0) {
        memcpy(arr_data, data, count * elem_size);
    }

    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * Generic Array Operations
 * ============================================================================ */

/* Clone: Create deep copy of array (generic - works for any element type) */
RtHandleV2 *rt_array_clone_v2(RtHandleV2 *arr_h, size_t elem_size) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    size_t count = rt_array_length_v2(arr_h);
    const void *src = rt_array_data_v2(arr_h);
    RtHandleV2 *result = array_create_v2(arena, count, elem_size, src);
    /* Propagate callbacks from source array */
    result->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(arr_h);
    return result;
}

/* Concat: Create new array from two arrays */
RtHandleV2 *rt_array_concat_v2(RtHandleV2 *a_h, RtHandleV2 *b_h, size_t elem_size) {
    if (a_h == NULL && b_h == NULL) return NULL;
    RtArenaV2 *arena = a_h ? a_h->arena : b_h->arena;
    if (a_h) rt_handle_begin_transaction(a_h);
    if (b_h) rt_handle_begin_transaction(b_h);
    size_t len_a = rt_array_length_v2(a_h);
    size_t len_b = rt_array_length_v2(b_h);
    const char *a = len_a ? (const char *)rt_array_data_v2(a_h) : NULL;
    const char *b = len_b ? (const char *)rt_array_data_v2(b_h) : NULL;
    size_t total = len_a + len_b;

    size_t alloc_size = sizeof(RtArrayMetadataV2) + total * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = total;
    meta->capacity = total;
    meta->element_size = elem_size;
    char *arr = (char *)raw + sizeof(RtArrayMetadataV2);
    if (len_a > 0) memcpy(arr, a, len_a * elem_size);
    if (len_b > 0) memcpy(arr + len_a * elem_size, b, len_b * elem_size);
    /* Propagate callbacks from source array */
    if (a_h) {
        h->copy_callback = a_h->copy_callback;
    } else if (b_h) {
        h->copy_callback = b_h->copy_callback;
    }
    rt_handle_end_transaction(h);
    if (b_h) rt_handle_end_transaction(b_h);
    if (a_h) rt_handle_end_transaction(a_h);
    return h;
}

/* Slice: Create new array from portion of source */
RtHandleV2 *rt_array_slice_v2(RtHandleV2 *arr_h, long start, long end, long step, size_t elem_size) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    const char *src = (const char *)rt_array_data_v2(arr_h);

    /* Normalize negative indices */
    if (start < 0) start = (long)len + start;
    if (end < 0) end = (long)len + end;
    if (start < 0) start = 0;
    if (end > (long)len) end = (long)len;
    if (step == 0 || step == LONG_MIN) step = 1;

    /* Calculate result size */
    size_t result_size = 0;
    if (step > 0) {
        for (long i = start; i < end; i += step) result_size++;
    } else {
        for (long i = start; i > end; i += step) result_size++;
    }

    size_t alloc_size = sizeof(RtArrayMetadataV2) + result_size * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = result_size;
    meta->capacity = result_size;
    meta->element_size = elem_size;
    char *dst = (char *)raw + sizeof(RtArrayMetadataV2);

    size_t j = 0;
    if (step > 0) {
        for (long i = start; i < end; i += step) {
            rt_handle_renew_transaction(h);
            rt_handle_renew_transaction(arr_h);
            memcpy(dst + j * elem_size, src + i * elem_size, elem_size);
            j++;
        }
    } else {
        for (long i = start; i > end; i += step) {
            rt_handle_renew_transaction(h);
            rt_handle_renew_transaction(arr_h);
            memcpy(dst + j * elem_size, src + i * elem_size, elem_size);
            j++;
        }
    }
    /* Propagate callbacks from source array */
    h->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    return h;
}

/* Reverse: Create new reversed array */
RtHandleV2 *rt_array_rev_v2(RtHandleV2 *arr_h, size_t elem_size) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    const char *src = (const char *)rt_array_data_v2(arr_h);

    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = len;
    meta->capacity = len;
    meta->element_size = elem_size;
    char *dst = (char *)raw + sizeof(RtArrayMetadataV2);

    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(h);
        rt_handle_renew_transaction(arr_h);
        memcpy(dst + i * elem_size, src + (len - 1 - i) * elem_size, elem_size);
    }
    /* Propagate callbacks from source array */
    h->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    return h;
}

/* Remove: Create new array without element at index */
RtHandleV2 *rt_array_rem_v2(RtHandleV2 *arr_h, long index, size_t elem_size) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    size_t len = rt_array_length_v2(arr_h);

    /* Normalize negative index */
    if (index < 0) index = (long)len + index;
    if (index < 0 || index >= (long)len) return rt_array_clone_v2(arr_h, elem_size);

    rt_handle_begin_transaction(arr_h);
    const char *src = (const char *)rt_array_data_v2(arr_h);
    size_t new_len = len - 1;

    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;
    meta->element_size = elem_size;
    char *dst = (char *)raw + sizeof(RtArrayMetadataV2);

    if (index > 0) memcpy(dst, src, index * elem_size);
    if ((size_t)index < len - 1) {
        memcpy(dst + index * elem_size, src + (index + 1) * elem_size, (len - index - 1) * elem_size);
    }
    /* Propagate callbacks from source array */
    h->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    return h;
}

/* Insert: Create new array with element inserted at index */
RtHandleV2 *rt_array_ins_v2(RtHandleV2 *arr_h, const void *elem, long index, size_t elem_size) {
    RtArenaV2 *arena = arr_h ? arr_h->arena : NULL;
    if (arena == NULL) return NULL;

    size_t len = rt_array_length_v2(arr_h);

    /* Normalize index */
    if (index < 0) index = (long)len + index + 1;
    if (index < 0) index = 0;
    if (index > (long)len) index = (long)len;

    rt_handle_begin_transaction(arr_h);
    const char *src = len ? (const char *)rt_array_data_v2(arr_h) : NULL;
    size_t new_len = len + 1;

    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;
    meta->element_size = elem_size;
    char *dst = (char *)raw + sizeof(RtArrayMetadataV2);

    if (index > 0 && src) memcpy(dst, src, index * elem_size);
    memcpy(dst + index * elem_size, elem, elem_size);
    if ((size_t)index < len && src) {
        memcpy(dst + (index + 1) * elem_size, src + index * elem_size, (len - index) * elem_size);
    }
    /* Propagate callbacks from source array */
    h->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    return h;
}

/* Push (generic): Append element to array, may reallocate */
RtHandleV2 *rt_array_push_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const void *elem, size_t elem_size) {
    if (arr_h == NULL) {
        size_t new_cap = 4;
        size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * elem_size;
        RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
        if (!new_h) return NULL;
        rt_handle_begin_transaction(new_h);
        void *new_raw = new_h->ptr;
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)new_raw;
        char *arr = (char *)new_raw + sizeof(RtArrayMetadataV2);
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = new_cap;
        meta->element_size = elem_size;
        memcpy(arr, elem, elem_size);
        rt_handle_end_transaction(new_h);
        return new_h;
    }

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    char *arr = (char *)raw + sizeof(RtArrayMetadataV2);

    if (meta->size < meta->capacity) {
        memcpy(arr + meta->size * elem_size, elem, elem_size);
        meta->size++;
        rt_handle_end_transaction(arr_h);
        return arr_h;
    }

    /* Need to grow */
    size_t old_size = meta->size;
    size_t new_cap = meta->capacity == 0 ? 4 : meta->capacity * 2;
    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_cap * elem_size;

    RtHandleV2 *new_h = rt_arena_v2_alloc(arena, alloc_size);
    if (!new_h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(new_h);
    void *new_raw = new_h->ptr;
    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    char *new_arr = (char *)new_raw + sizeof(RtArrayMetadataV2);

    memcpy(new_arr, arr, old_size * elem_size);
    new_meta->arena = arena;
    new_meta->size = old_size + 1;
    new_meta->capacity = new_cap;
    new_meta->element_size = elem_size;
    memcpy(new_arr + old_size * elem_size, elem, elem_size);

    /* Propagate callbacks from old handle to new handle */
    new_h->copy_callback = arr_h->copy_callback;

    rt_handle_end_transaction(new_h);
    rt_handle_end_transaction(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Push Copy: Create NEW array with element appended (non-mutating) */
RtHandleV2 *rt_array_push_copy_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size) {
    RtArenaV2 *arena = arr_h ? arr_h->arena : NULL;
    if (arena == NULL) return NULL;

    rt_handle_begin_transaction(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    const char *src = len ? (const char *)rt_array_data_v2(arr_h) : NULL;
    size_t new_len = len + 1;

    size_t alloc_size = sizeof(RtArrayMetadataV2) + new_len * elem_size;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) { rt_handle_end_transaction(arr_h); return NULL; }
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = new_len;
    char *dst = (char *)raw + sizeof(RtArrayMetadataV2);

    if (len > 0 && src) memcpy(dst, src, len * elem_size);
    memcpy(dst + len * elem_size, elem, elem_size);
    /* Propagate callbacks from source array */
    h->copy_callback = arr_h->copy_callback;
    rt_handle_end_transaction(h);
    rt_handle_end_transaction(arr_h);
    return h;
}

/* Pop (generic): Remove last element, copy to out pointer */
void rt_array_pop_v2(RtHandleV2 *arr_h, void *out, size_t elem_size) {
    if (arr_h == NULL || out == NULL) return;
    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    if (meta->size == 0) {
        rt_handle_end_transaction(arr_h);
        memset(out, 0, elem_size);
        return;
    }
    char *arr = (char *)raw + sizeof(RtArrayMetadataV2);
    meta->size--;
    memcpy(out, arr + meta->size * elem_size, elem_size);
    rt_handle_end_transaction(arr_h);
}

/* Clear: Set size to 0, keep capacity */
void rt_array_clear_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return;
    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->size = 0;
    rt_handle_end_transaction(arr_h);
}

/* IndexOf: Find first index of element using memcmp */
long rt_array_indexOf_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size) {
    if (arr_h == NULL || elem == NULL) return -1;
    rt_handle_begin_transaction(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    const char *arr = (const char *)rt_array_data_v2(arr_h);
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        if (memcmp(arr + i * elem_size, elem, elem_size) == 0) {
            rt_handle_end_transaction(arr_h);
            return (long)i;
        }
    }
    rt_handle_end_transaction(arr_h);
    return -1;
}

/* Contains: Check if element exists using memcmp */
int rt_array_contains_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size) {
    return rt_array_indexOf_v2(arr_h, elem, elem_size) >= 0;
}

/* Equality: Check if two arrays have same length and elements */
int rt_array_eq_v2(RtHandleV2 *a_h, RtHandleV2 *b_h, size_t elem_size) {
    size_t len_a = rt_array_length_v2(a_h);
    size_t len_b = rt_array_length_v2(b_h);
    if (len_a != len_b) return 0;
    if (len_a == 0) return 1;
    rt_handle_begin_transaction(a_h);
    rt_handle_begin_transaction(b_h);
    const void *a = rt_array_data_v2(a_h);
    const void *b = rt_array_data_v2(b_h);
    int result = memcmp(a, b, len_a * elem_size) == 0;
    rt_handle_end_transaction(b_h);
    rt_handle_end_transaction(a_h);
    return result;
}

/* ============================================================================
 * Array Create Functions
 * ============================================================================ */

/* String array: converts char* pointers to RtHandleV2* elements */
RtHandleV2 *rt_array_create_string_v2(RtArenaV2 *arena, size_t count, const char **data) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        rt_handle_renew_transaction(h);
        arr[i] = rt_arena_v2_strdup(arena, data[i] ? data[i] : "");
    }

    rt_handle_end_transaction(h);
    rt_handle_set_copy_callback(h, rt_array_copy_callback);
    return h;
}

RtHandleV2 *rt_array_create_generic_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data) {
    return array_create_v2(arena, count, elem_size, data);
}

/* Pointer (nested array) create -- elements are RtHandleV2* */
RtHandleV2 *rt_array_create_ptr_v2(RtArenaV2 *arena, size_t count, RtHandleV2 **data) {
    RtHandleV2 *h = array_create_v2(arena, count, sizeof(RtHandleV2 *), data);
    if (h) {
        rt_handle_set_copy_callback(h, rt_array_copy_callback);
    }
    return h;
}

/* ============================================================================
 * Generic Array GC Callbacks
 * ============================================================================
 * Copy callback enables self-describing handles for arrays containing
 * nested handles (str[], T[][], etc.). When set, rt_arena_v2_promote()
 * can deep-copy array contents automatically.
 * ============================================================================ */

/* Copy callback for arrays of handles (str[], T[][], etc.) */
void rt_array_copy_callback(RtArenaV2 *dest, RtHandleV2 *new_handle) {
    void *ptr = new_handle->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    meta->arena = dest;  /* Update arena reference after shallow copy */
    if (meta->element_size == sizeof(RtHandleV2 *)) {
        RtHandleV2 **arr = (RtHandleV2 **)((char *)ptr + sizeof(RtArrayMetadataV2));
        for (size_t i = 0; i < meta->size; i++) {
            if (arr[i] != NULL) arr[i] = rt_arena_v2_promote(dest, arr[i]);
        }
    }
}

