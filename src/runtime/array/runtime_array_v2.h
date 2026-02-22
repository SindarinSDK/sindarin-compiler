/*
 * Runtime Array V2 - Handle-Based Arrays for Arena V2
 * ====================================================
 *
 * V2 arrays use RtHandleV2* (fat pointers) instead of RtHandle (uint32 indices).
 * Key benefits:
 * - No arena parameter needed for pin/unpin operations
 * - Handles are self-describing (carry arena reference)
 * - Simpler promotion (no source arena parameter)
 *
 * Array layout: [RtArrayMetadataV2][element data...]
 * String arrays store RtHandleV2* elements (pointer size, not 4 bytes)
 * Nested arrays store RtHandleV2* elements
 */

#ifndef RUNTIME_ARRAY_V2_H
#define RUNTIME_ARRAY_V2_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "runtime/arena/arena_v2.h"
#include "runtime/runtime_any.h"

/* ============================================================================
 * Array Metadata V2
 * ============================================================================
 * Stored immediately before array data in the handle's allocation.
 * Memory layout: [RtArrayMetadataV2][element data...]
 * ============================================================================ */

typedef struct {
    RtArenaV2 *arena;   /* Arena that owns this array */
    size_t size;        /* Number of elements currently in array */
    size_t capacity;    /* Total allocated space for elements */
    size_t element_size; /* Size of each element in bytes */
} RtArrayMetadataV2;

/* ============================================================================
 * Helper: Get array length from a V2 handle
 * ============================================================================ */

static inline size_t rt_array_length_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return 0;
    rt_handle_begin_transaction(arr_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)(arr_h->ptr);
    size_t len = meta ? meta->size : 0;
    rt_handle_end_transaction(arr_h);
    return len;
}

/* Get array length from a raw data pointer (data area, after metadata).
 * Used when working with unboxed array data that has V2 metadata before it. */
static inline size_t rt_v2_data_array_length(const void *arr) {
    if (arr == NULL) return 0;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)((char *)arr - sizeof(RtArrayMetadataV2));
    return meta->size;
}

/* Get pointer to array DATA area (after metadata) for element access.
 * This is what you use for arr[i] indexing.
 * IMPORTANT: Caller must hold a transaction on arr_h. */
static inline void *rt_array_data_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    void *raw = arr_h->ptr;
    if (raw == NULL) return NULL;
    return (char *)raw + sizeof(RtArrayMetadataV2);
}

/* ============================================================================
 * Typed Element Accessors (transactional)
 * ============================================================================
 * These inline functions read/write individual array elements with automatic
 * transaction bracketing. The transaction ensures the data pointer remains
 * stable during the access (GC cannot compact the block).
 * ============================================================================ */

#define RT_ARRAY_ACCESSOR_DECL(suffix, ctype) \
    static inline ctype rt_array_get_##suffix##_v2(RtHandleV2 *arr_h, long long idx) { \
        rt_handle_begin_transaction(arr_h); \
        ctype *data = (ctype *)((char *)arr_h->ptr + sizeof(RtArrayMetadataV2)); \
        ctype result = data[idx]; \
        rt_handle_end_transaction(arr_h); \
        return result; \
    } \
    static inline ctype rt_array_set_##suffix##_v2(RtHandleV2 *arr_h, long long idx, ctype val) { \
        rt_handle_begin_transaction(arr_h); \
        ctype *data = (ctype *)((char *)arr_h->ptr + sizeof(RtArrayMetadataV2)); \
        data[idx] = val; \
        rt_handle_end_transaction(arr_h); \
        return val; \
    }

RT_ARRAY_ACCESSOR_DECL(long, long long)
RT_ARRAY_ACCESSOR_DECL(int32, int32_t)
RT_ARRAY_ACCESSOR_DECL(uint, uint64_t)
RT_ARRAY_ACCESSOR_DECL(uint32, uint32_t)
RT_ARRAY_ACCESSOR_DECL(double, double)
RT_ARRAY_ACCESSOR_DECL(float, float)
RT_ARRAY_ACCESSOR_DECL(char, char)
RT_ARRAY_ACCESSOR_DECL(bool, int)
RT_ARRAY_ACCESSOR_DECL(byte, unsigned char)

/* Handle accessor for string/array elements stored as RtHandleV2* */
static inline RtHandleV2 *rt_array_get_handle_v2(RtHandleV2 *arr_h, long long idx) {
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **data = (RtHandleV2 **)((char *)arr_h->ptr + sizeof(RtArrayMetadataV2));
    RtHandleV2 *result = data[idx];
    rt_handle_end_transaction(arr_h);
    return result;
}
static inline RtHandleV2 *rt_array_set_handle_v2(RtHandleV2 *arr_h, long long idx, RtHandleV2 *val) {
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **data = (RtHandleV2 **)((char *)arr_h->ptr + sizeof(RtArrayMetadataV2));
    /* Free old handle on reassignment to prevent leaks in long-lived arenas */
    if (data[idx] != NULL && data[idx] != val) {
        rt_arena_v2_free(data[idx]);
    }
    data[idx] = val;
    rt_handle_end_transaction(arr_h);
    return val;
}

/* Transaction-based data access for struct/complex element types.
 * Caller must pair begin/end around all ptr accesses. */
static inline void *rt_array_data_begin_v2(RtHandleV2 *arr_h) {
    rt_handle_begin_transaction(arr_h);
    if (arr_h->ptr == NULL) { rt_handle_end_transaction(arr_h); return NULL; }
    return (char *)arr_h->ptr + sizeof(RtArrayMetadataV2);
}
static inline void rt_array_data_end_v2(RtHandleV2 *arr_h) {
    rt_handle_end_transaction(arr_h);
}

/* ============================================================================
 * String Array Pin for Native Interop
 * ============================================================================
 * Converts a V2 string array (RtHandleV2* containing RtHandleV2* elements)
 * to a legacy char** for use with native C functions.
 * ============================================================================ */

char **rt_pin_string_array_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Generic Array Operations
 * ============================================================================
 * These functions work with any element type by taking elem_size parameter.
 * Use these instead of type-specific variants for cleaner code.
 * String arrays need special handling (strdup/strcmp) - use _string variants.
 * ============================================================================ */

/* Clone: Create deep copy of array */
RtHandleV2 *rt_array_clone_v2(RtHandleV2 *arr_h, size_t elem_size);

/* Concat: Create new array from two arrays */
RtHandleV2 *rt_array_concat_v2(RtHandleV2 *a_h, RtHandleV2 *b_h, size_t elem_size);

/* Slice: Create new array from portion of source */
RtHandleV2 *rt_array_slice_v2(RtHandleV2 *arr_h, long start, long end, long step, size_t elem_size);

/* Reverse: Create new reversed array */
RtHandleV2 *rt_array_rev_v2(RtHandleV2 *arr_h, size_t elem_size);

/* Remove: Create new array without element at index */
RtHandleV2 *rt_array_rem_v2(RtHandleV2 *arr_h, long index, size_t elem_size);

/* Insert: Create new array with element inserted at index */
RtHandleV2 *rt_array_ins_v2(RtHandleV2 *arr_h, const void *elem, long index, size_t elem_size);

/* Push (generic): Append element to array, may reallocate */
RtHandleV2 *rt_array_push_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const void *elem, size_t elem_size);

/* Push Copy: Create NEW array with element appended (non-mutating) */
RtHandleV2 *rt_array_push_copy_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size);

/* Pop (generic): Remove last element, copy to out pointer */
void rt_array_pop_v2(RtHandleV2 *arr_h, void *out, size_t elem_size);

/* Clear: Set size to 0, keep capacity */
void rt_array_clear_v2(RtHandleV2 *arr_h);

/* IndexOf: Find first index of element using memcmp, returns -1 if not found */
long rt_array_indexOf_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size);

/* Contains: Check if element exists using memcmp */
int rt_array_contains_v2(RtHandleV2 *arr_h, const void *elem, size_t elem_size);

/* Equality: Check if two arrays have same length and elements (memcmp) */
int rt_array_eq_v2(RtHandleV2 *a_h, RtHandleV2 *b_h, size_t elem_size);

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create array from existing data. Returns handle to [metadata][data].
 * Use rt_array_create_generic_v2 for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_create_string_v2(RtArenaV2 *arena, size_t count, const char **data);
RtHandleV2 *rt_array_create_generic_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data);
RtHandleV2 *rt_array_create_ptr_v2(RtArenaV2 *arena, size_t count, RtHandleV2 **data);

/* ============================================================================
 * Array Push Functions
 * ============================================================================
 * Push element to end. May reallocate if capacity exceeded.
 * Returns possibly-new handle (old handle marked dead if reallocated).
 * Use rt_array_push_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_push_string_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element);
RtHandleV2 *rt_array_push_ptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element);
RtHandleV2 *rt_array_push_voidptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtHandleV2 *element);
RtHandleV2 *rt_array_push_any_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, RtAny element);

/* ============================================================================
 * Array Pop Functions
 * ============================================================================
 * Remove and return last element. Modifies array in place.
 * ============================================================================ */

long long rt_array_pop_long_v2(RtHandleV2 *arr_h);
double rt_array_pop_double_v2(RtHandleV2 *arr_h);
char rt_array_pop_char_v2(RtHandleV2 *arr_h);
int rt_array_pop_bool_v2(RtHandleV2 *arr_h);
unsigned char rt_array_pop_byte_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_pop_string_v2(RtHandleV2 *arr_h);
int32_t rt_array_pop_int32_v2(RtHandleV2 *arr_h);
uint32_t rt_array_pop_uint32_v2(RtHandleV2 *arr_h);
uint64_t rt_array_pop_uint_v2(RtHandleV2 *arr_h);
float rt_array_pop_float_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_pop_ptr_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a deep copy of the array. Arena derived from input handle.
 * Use rt_array_clone_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_clone_string_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_clone_ptr_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_clone_any_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Array Concat Functions
 * ============================================================================
 * Create new array containing elements from both arrays.
 * Arena derived from first handle.
 * Use rt_array_concat_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_concat_string_v2(RtHandleV2 *a_h, RtHandleV2 *b_h);
RtHandleV2 *rt_array_concat_ptr_v2(RtHandleV2 *a_h, RtHandleV2 *b_h);

/* ============================================================================
 * Array Slice Functions
 * ============================================================================
 * Create new array from portion of source array.
 * Arena derived from input handle.
 * Use rt_array_slice_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_slice_string_v2(RtHandleV2 *arr_h, long start, long end, long step);

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return new reversed array. Arena derived from input handle.
 * Use rt_array_rev_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_rev_string_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return new array without element at index. Arena derived from input handle.
 * Use rt_array_rem_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_rem_string_v2(RtHandleV2 *arr_h, long index);

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return new array with element inserted at index. Arena derived from input handle.
 * Use rt_array_ins_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_ins_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem, long index);

/* ============================================================================
 * Array Push Copy Functions
 * ============================================================================
 * Create NEW array with element appended (non-mutating push).
 * Arena derived from input handle.
 * Use rt_array_push_copy_v2 (generic) for primitive types.
 * ============================================================================ */

RtHandleV2 *rt_array_push_copy_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem);

/* ============================================================================
 * Array Alloc Functions
 * ============================================================================
 * Create array of count elements filled with default_value.
 * ============================================================================ */

RtHandleV2 *rt_array_alloc_long_v2(RtArenaV2 *arena, size_t count, long long default_value);
RtHandleV2 *rt_array_alloc_double_v2(RtArenaV2 *arena, size_t count, double default_value);
RtHandleV2 *rt_array_alloc_char_v2(RtArenaV2 *arena, size_t count, char default_value);
RtHandleV2 *rt_array_alloc_bool_v2(RtArenaV2 *arena, size_t count, int default_value);
RtHandleV2 *rt_array_alloc_byte_v2(RtArenaV2 *arena, size_t count, unsigned char default_value);
RtHandleV2 *rt_array_alloc_string_v2(RtArenaV2 *arena, size_t count, RtHandleV2 *default_value);
RtHandleV2 *rt_array_alloc_int32_v2(RtArenaV2 *arena, size_t count, int32_t default_value);
RtHandleV2 *rt_array_alloc_uint32_v2(RtArenaV2 *arena, size_t count, uint32_t default_value);
RtHandleV2 *rt_array_alloc_uint_v2(RtArenaV2 *arena, size_t count, uint64_t default_value);
RtHandleV2 *rt_array_alloc_float_v2(RtArenaV2 *arena, size_t count, float default_value);

/* ============================================================================
 * Array Range Function
 * ============================================================================
 * Create array of longs from start to end (exclusive).
 * ============================================================================ */

RtHandleV2 *rt_array_range_v2(RtArenaV2 *arena, long long start, long long end);

/* ============================================================================
 * Array Type Conversion
 * ============================================================================ */

RtHandleV2 *rt_array_from_legacy_string_v2(RtArenaV2 *arena, char **src);
char **rt_pin_string_array_v2(RtHandleV2 *arr_h);

/* Note: 2D/3D array to any[][] conversion functions will be added
 * when RtAny integration is complete. */

/* ============================================================================
 * String Array Equality
 * ============================================================================ */

int rt_array_eq_string_v2(RtHandleV2 *a_h, RtHandleV2 *b_h);

/* ============================================================================
 * Args Creation
 * ============================================================================ */

RtHandleV2 *rt_args_create_v2(RtArenaV2 *arena, int argc, char **argv);

/* ============================================================================
 * Generic Array GC Callbacks
 * ============================================================================
 * Self-describing handles: set these on arrays containing nested handles
 * so rt_arena_v2_promote() and GC sweep work automatically.
 * ============================================================================ */

/* Copy callback for arrays of handles (str[], T[][], etc.) */
void rt_array_copy_callback(RtArenaV2 *dest, void *ptr);

/* Copy callback for any[] arrays */
void rt_array_any_copy_callback(RtArenaV2 *dest, void *ptr);

/* ============================================================================
 * Array Join Functions (V2)
 * ============================================================================
 * Join array elements into a string. Arena derived from input handle.
 * ============================================================================ */

RtHandleV2 *rt_array_join_long_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_double_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_char_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_bool_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_byte_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_string_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_int32_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_uint32_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_uint_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);
RtHandleV2 *rt_array_join_float_v2(RtHandleV2 *arr_h, RtHandleV2 *separator);

/* ============================================================================
 * Array ToString Functions (1D)
 * ============================================================================
 * Convert array to string representation. Arena derived from input handle.
 * ============================================================================ */

RtHandleV2 *rt_to_string_array_long_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_double_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_char_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_bool_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_byte_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_string_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_int32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_uint32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_uint_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array_float_v2(RtHandleV2 *arr_h);

/* Print functions - take handle, use V2 metadata */
void rt_print_array_long_v2(RtHandleV2 *arr_h);
void rt_print_array_double_v2(RtHandleV2 *arr_h);
void rt_print_array_char_v2(RtHandleV2 *arr_h);
void rt_print_array_bool_v2(RtHandleV2 *arr_h);
void rt_print_array_byte_v2(RtHandleV2 *arr_h);
void rt_print_array_string_v2(RtHandleV2 *arr_h);
void rt_print_array_int32_v2(RtHandleV2 *arr_h);
void rt_print_array_uint32_v2(RtHandleV2 *arr_h);
void rt_print_array_uint_v2(RtHandleV2 *arr_h);
void rt_print_array_float_v2(RtHandleV2 *arr_h);

/* String array utilities */
long rt_array_indexOf_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem);
int rt_array_contains_string_v2(RtHandleV2 *arr_h, RtHandleV2 *elem);

/* 2D array toString - arena derived from outer handle */
RtHandleV2 *rt_to_string_array2_long_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array2_double_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array2_char_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array2_bool_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array2_byte_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array2_string_v2(RtHandleV2 *outer_h);

/* 3D array toString - arena derived from outer handle */
RtHandleV2 *rt_to_string_array3_long_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_double_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_char_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_bool_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_byte_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_string_v2(RtHandleV2 *outer_h);

/* ============================================================================
 * Convert Legacy Array to V2 Handle
 * ============================================================================
 * Convert raw string array to handle-based string array.
 * ============================================================================ */

RtHandleV2 *rt_array_from_raw_strings_v2(RtArenaV2 *arena, const char **src);

/* ============================================================================
 * 1D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed V2 arrays to any[] handles by boxing each element.
 * Arena derived from input handle.
 * ============================================================================ */

RtHandleV2 *rt_array_to_any_long_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_double_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_char_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_bool_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_byte_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_string_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_int32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_uint32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_uint_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_to_any_float_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * 2D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 2D V2 arrays to any[][] handles.
 * Arena derived from outer handle.
 * ============================================================================ */

RtHandleV2 *rt_array2_to_any_long_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array2_to_any_double_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array2_to_any_char_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array2_to_any_bool_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array2_to_any_byte_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array2_to_any_string_v2(RtHandleV2 *outer_h);

/* ============================================================================
 * 3D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 3D V2 arrays to any[][][] handles.
 * Arena derived from outer handle.
 * ============================================================================ */

RtHandleV2 *rt_array3_to_any_long_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array3_to_any_double_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array3_to_any_char_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array3_to_any_bool_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array3_to_any_byte_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_array3_to_any_string_v2(RtHandleV2 *outer_h);

/* ============================================================================
 * Convert any[] to Typed Array V2 Functions
 * ============================================================================
 * Convert any[] handles to typed array handles.
 * Arena derived from input handle.
 * ============================================================================ */

RtHandleV2 *rt_array_from_any_long_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_int32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_uint_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_uint32_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_double_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_float_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_char_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_bool_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_byte_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_array_from_any_string_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Any Array toString V2 Functions
 * ============================================================================
 * Arena derived from input handle.
 * ============================================================================ */

RtHandleV2 *rt_to_string_array_any_v2(RtHandleV2 *arr_h);
RtHandleV2 *rt_to_string_array2_any_v2(RtHandleV2 *outer_h);
RtHandleV2 *rt_to_string_array3_any_v2(RtHandleV2 *outer_h);

#endif /* RUNTIME_ARRAY_V2_H */
