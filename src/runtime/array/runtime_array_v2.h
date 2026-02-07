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
} RtArrayMetadataV2;

/* ============================================================================
 * Helper: Get array length from a V2 handle
 * ============================================================================ */

static inline size_t rt_array_length_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return 0;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)rt_handle_v2_ptr(arr_h);
    return meta ? meta->size : 0;
}

/* Get pointer to array DATA area (after metadata) for element access.
 * This is what you use for arr[i] indexing. */
static inline void *rt_array_data_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    void *raw = rt_handle_v2_pin(arr_h);
    if (raw == NULL) return NULL;
    return (char *)raw + sizeof(RtArrayMetadataV2);
}

/* ============================================================================
 * String Array Pin for Native Interop
 * ============================================================================
 * Converts a V2 string array (RtHandleV2* containing RtHandleV2* elements)
 * to a legacy char** for use with native C functions.
 * ============================================================================ */

char **rt_pin_string_array_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create array from existing data. Returns handle to [metadata][data].
 * ============================================================================ */

RtHandleV2 *rt_array_create_long_v2(RtArenaV2 *arena, size_t count, const long long *data);
RtHandleV2 *rt_array_create_double_v2(RtArenaV2 *arena, size_t count, const double *data);
RtHandleV2 *rt_array_create_char_v2(RtArenaV2 *arena, size_t count, const char *data);
RtHandleV2 *rt_array_create_bool_v2(RtArenaV2 *arena, size_t count, const int *data);
RtHandleV2 *rt_array_create_byte_v2(RtArenaV2 *arena, size_t count, const unsigned char *data);
RtHandleV2 *rt_array_create_string_v2(RtArenaV2 *arena, size_t count, const char **data);
RtHandleV2 *rt_array_create_int32_v2(RtArenaV2 *arena, size_t count, const int32_t *data);
RtHandleV2 *rt_array_create_uint32_v2(RtArenaV2 *arena, size_t count, const uint32_t *data);
RtHandleV2 *rt_array_create_uint_v2(RtArenaV2 *arena, size_t count, const uint64_t *data);
RtHandleV2 *rt_array_create_float_v2(RtArenaV2 *arena, size_t count, const float *data);
RtHandleV2 *rt_array_create_generic_v2(RtArenaV2 *arena, size_t count, size_t elem_size, const void *data);
RtHandleV2 *rt_array_create_ptr_v2(RtArenaV2 *arena, size_t count, void **data);

/* ============================================================================
 * Array Push Functions
 * ============================================================================
 * Push element to end. May reallocate if capacity exceeded.
 * Returns possibly-new handle (old handle marked dead if reallocated).
 * ============================================================================ */

RtHandleV2 *rt_array_push_long_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, long long element);
RtHandleV2 *rt_array_push_double_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, double element);
RtHandleV2 *rt_array_push_char_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, char element);
RtHandleV2 *rt_array_push_bool_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, int element);
RtHandleV2 *rt_array_push_byte_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, unsigned char element);
RtHandleV2 *rt_array_push_string_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const char *element);
RtHandleV2 *rt_array_push_int32_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, int32_t element);
RtHandleV2 *rt_array_push_uint32_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, uint32_t element);
RtHandleV2 *rt_array_push_uint_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, uint64_t element);
RtHandleV2 *rt_array_push_float_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, float element);
RtHandleV2 *rt_array_push_ptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, void *element);
RtHandleV2 *rt_array_push_voidptr_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, void *element);
RtHandleV2 *rt_array_push_struct_v2(RtArenaV2 *arena, RtHandleV2 *arr_h, const void *element, size_t elem_size);
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
void *rt_array_pop_ptr_v2(RtHandleV2 *arr_h);

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a new array from raw pointer data.
 * ============================================================================ */

RtHandleV2 *rt_array_clone_long_v2(RtArenaV2 *arena, const long long *src);
RtHandleV2 *rt_array_clone_double_v2(RtArenaV2 *arena, const double *src);
RtHandleV2 *rt_array_clone_char_v2(RtArenaV2 *arena, const char *src);
RtHandleV2 *rt_array_clone_bool_v2(RtArenaV2 *arena, const int *src);
RtHandleV2 *rt_array_clone_byte_v2(RtArenaV2 *arena, const unsigned char *src);
RtHandleV2 *rt_array_clone_string_v2(RtArenaV2 *arena, const char **src);
RtHandleV2 *rt_array_clone_string_handle_v2(RtArenaV2 *arena, RtHandleV2 *src_arr);
RtHandleV2 *rt_array_clone_int32_v2(RtArenaV2 *arena, const int32_t *src);
RtHandleV2 *rt_array_clone_uint32_v2(RtArenaV2 *arena, const uint32_t *src);
RtHandleV2 *rt_array_clone_uint_v2(RtArenaV2 *arena, const uint64_t *src);
RtHandleV2 *rt_array_clone_float_v2(RtArenaV2 *arena, const float *src);
RtHandleV2 *rt_array_clone_ptr_v2(RtArenaV2 *arena, void **src);
RtHandleV2 *rt_array_clone_void_v2(RtArenaV2 *arena, const RtAny *src);

/* ============================================================================
 * Array Concat Functions
 * ============================================================================
 * Create new array containing elements from both arrays.
 * ============================================================================ */

RtHandleV2 *rt_array_concat_long_v2(RtArenaV2 *arena, const long long *a, const long long *b);
RtHandleV2 *rt_array_concat_double_v2(RtArenaV2 *arena, const double *a, const double *b);
RtHandleV2 *rt_array_concat_char_v2(RtArenaV2 *arena, const char *a, const char *b);
RtHandleV2 *rt_array_concat_bool_v2(RtArenaV2 *arena, const int *a, const int *b);
RtHandleV2 *rt_array_concat_byte_v2(RtArenaV2 *arena, const unsigned char *a, const unsigned char *b);
RtHandleV2 *rt_array_concat_string_v2(RtArenaV2 *arena, RtHandleV2 **a, RtHandleV2 **b);
RtHandleV2 *rt_array_concat_int32_v2(RtArenaV2 *arena, const int32_t *a, const int32_t *b);
RtHandleV2 *rt_array_concat_uint32_v2(RtArenaV2 *arena, const uint32_t *a, const uint32_t *b);
RtHandleV2 *rt_array_concat_uint_v2(RtArenaV2 *arena, const uint64_t *a, const uint64_t *b);
RtHandleV2 *rt_array_concat_float_v2(RtArenaV2 *arena, const float *a, const float *b);
RtHandleV2 *rt_array_concat_ptr_v2(RtArenaV2 *arena, void **a, void **b);

/* ============================================================================
 * Array Slice Functions
 * ============================================================================
 * Create new array from portion of source array.
 * ============================================================================ */

RtHandleV2 *rt_array_slice_long_v2(RtArenaV2 *arena, const long long *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_double_v2(RtArenaV2 *arena, const double *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_char_v2(RtArenaV2 *arena, const char *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_bool_v2(RtArenaV2 *arena, const int *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_byte_v2(RtArenaV2 *arena, const unsigned char *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_string_v2(RtArenaV2 *arena, RtHandleV2 **arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_int32_v2(RtArenaV2 *arena, const int32_t *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_uint32_v2(RtArenaV2 *arena, const uint32_t *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_uint_v2(RtArenaV2 *arena, const uint64_t *arr, long start, long end, long step);
RtHandleV2 *rt_array_slice_float_v2(RtArenaV2 *arena, const float *arr, long start, long end, long step);

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return new reversed array.
 * ============================================================================ */

RtHandleV2 *rt_array_rev_long_v2(RtArenaV2 *arena, const long long *arr);
RtHandleV2 *rt_array_rev_double_v2(RtArenaV2 *arena, const double *arr);
RtHandleV2 *rt_array_rev_char_v2(RtArenaV2 *arena, const char *arr);
RtHandleV2 *rt_array_rev_bool_v2(RtArenaV2 *arena, const int *arr);
RtHandleV2 *rt_array_rev_byte_v2(RtArenaV2 *arena, const unsigned char *arr);
RtHandleV2 *rt_array_rev_string_v2(RtArenaV2 *arena, const char **arr);
RtHandleV2 *rt_array_rev_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr);
RtHandleV2 *rt_array_clone_string_data_v2(RtArenaV2 *arena, RtHandleV2 **arr);

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return new array without element at index.
 * ============================================================================ */

RtHandleV2 *rt_array_rem_long_v2(RtArenaV2 *arena, const long long *arr, long index);
RtHandleV2 *rt_array_rem_double_v2(RtArenaV2 *arena, const double *arr, long index);
RtHandleV2 *rt_array_rem_char_v2(RtArenaV2 *arena, const char *arr, long index);
RtHandleV2 *rt_array_rem_bool_v2(RtArenaV2 *arena, const int *arr, long index);
RtHandleV2 *rt_array_rem_byte_v2(RtArenaV2 *arena, const unsigned char *arr, long index);
RtHandleV2 *rt_array_rem_string_v2(RtArenaV2 *arena, const char **arr, long index);
RtHandleV2 *rt_array_rem_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr, long index);

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return new array with element inserted at index.
 * ============================================================================ */

RtHandleV2 *rt_array_ins_long_v2(RtArenaV2 *arena, const long long *arr, long long elem, long index);
RtHandleV2 *rt_array_ins_double_v2(RtArenaV2 *arena, const double *arr, double elem, long index);
RtHandleV2 *rt_array_ins_char_v2(RtArenaV2 *arena, const char *arr, char elem, long index);
RtHandleV2 *rt_array_ins_bool_v2(RtArenaV2 *arena, const int *arr, int elem, long index);
RtHandleV2 *rt_array_ins_byte_v2(RtArenaV2 *arena, const unsigned char *arr, unsigned char elem, long index);
RtHandleV2 *rt_array_ins_string_v2(RtArenaV2 *arena, const char **arr, const char *elem, long index);
RtHandleV2 *rt_array_ins_string_handle_v2(RtArenaV2 *arena, RtHandleV2 **arr, const char *elem, long index);

/* ============================================================================
 * Array Push Copy Functions
 * ============================================================================
 * Create NEW array with element appended (non-mutating push).
 * ============================================================================ */

RtHandleV2 *rt_array_push_copy_long_v2(RtArenaV2 *arena, const long long *arr, long long elem);
RtHandleV2 *rt_array_push_copy_double_v2(RtArenaV2 *arena, const double *arr, double elem);
RtHandleV2 *rt_array_push_copy_char_v2(RtArenaV2 *arena, const char *arr, char elem);
RtHandleV2 *rt_array_push_copy_bool_v2(RtArenaV2 *arena, const int *arr, int elem);
RtHandleV2 *rt_array_push_copy_byte_v2(RtArenaV2 *arena, const unsigned char *arr, unsigned char elem);
RtHandleV2 *rt_array_push_copy_string_v2(RtArenaV2 *arena, const char **arr, const char *elem);

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
RtHandleV2 *rt_array_alloc_string_v2(RtArenaV2 *arena, size_t count, const char *default_value);

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
 * Deep Array Promotion
 * ============================================================================
 * V2 promotion is simpler - no source arena parameter needed!
 * Handle carries its arena reference.
 * ============================================================================ */

/* Promote str[] - promotes array AND all string elements */
RtHandleV2 *rt_promote_array_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h);

/* Promote T[][] - promotes outer array AND all inner array handles */
RtHandleV2 *rt_promote_array_handle_v2(RtArenaV2 *dest, RtHandleV2 *arr_h);

/* Promote T[][][] - promotes outer, middle, and inner array handles */
RtHandleV2 *rt_promote_array_handle_3d_v2(RtArenaV2 *dest, RtHandleV2 *arr_h);

/* Promote str[][] - promotes outer, inner arrays, AND strings */
RtHandleV2 *rt_promote_array2_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h);

/* Promote str[][][] - promotes all three levels AND strings */
RtHandleV2 *rt_promote_array3_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h);

/* ============================================================================
 * Array Join Functions (V2)
 * ============================================================================ */

char *rt_array_join_long_v2(RtArenaV2 *arena, const long long *arr, const char *separator);
char *rt_array_join_double_v2(RtArenaV2 *arena, const double *arr, const char *separator);
char *rt_array_join_char_v2(RtArenaV2 *arena, const char *arr, const char *separator);
char *rt_array_join_bool_v2(RtArenaV2 *arena, const int *arr, const char *separator);
char *rt_array_join_byte_v2(RtArenaV2 *arena, const unsigned char *arr, const char *separator);
char *rt_array_join_int32_v2(RtArenaV2 *arena, const int32_t *arr, const char *separator);
char *rt_array_join_uint32_v2(RtArenaV2 *arena, const uint32_t *arr, const char *separator);
char *rt_array_join_uint_v2(RtArenaV2 *arena, const uint64_t *arr, const char *separator);
char *rt_array_join_float_v2(RtArenaV2 *arena, const float *arr, const char *separator);

/* ============================================================================
 * Handle-aware ToString for String Arrays
 * ============================================================================ */

char *rt_to_string_array_string_v2(RtArenaV2 *arena, RtHandleV2 **arr);
char *rt_array_join_string_v2(RtArenaV2 *arena, RtHandleV2 **arr, const char *separator);
void rt_print_array_string_v2(RtHandleV2 **arr);
long rt_array_indexOf_string_v2(RtHandleV2 **arr, const char *elem);
int rt_array_contains_string_v2(RtHandleV2 **arr, const char *elem);

/* 2D array toString */
char *rt_to_string_array2_long_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array2_double_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array2_char_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array2_bool_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array2_byte_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array2_string_v2(RtArenaV2 *arena, RtHandleV2 **outer);

/* 3D array toString */
char *rt_to_string_array3_long_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_double_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_char_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_bool_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_byte_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_string_v2(RtArenaV2 *arena, RtHandleV2 **outer);

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
 * ============================================================================ */

RtHandleV2 *rt_array_to_any_long_v2(RtArenaV2 *arena, const long long *arr);
RtHandleV2 *rt_array_to_any_double_v2(RtArenaV2 *arena, const double *arr);
RtHandleV2 *rt_array_to_any_char_v2(RtArenaV2 *arena, const char *arr);
RtHandleV2 *rt_array_to_any_bool_v2(RtArenaV2 *arena, const int *arr);
RtHandleV2 *rt_array_to_any_byte_v2(RtArenaV2 *arena, const unsigned char *arr);
RtHandleV2 *rt_array_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 **arr);

/* ============================================================================
 * 2D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 2D V2 arrays to any[][] handles.
 * ============================================================================ */

RtHandleV2 *rt_array2_to_any_long_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array2_to_any_double_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array2_to_any_char_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array2_to_any_bool_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array2_to_any_byte_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array2_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 *outer);

/* ============================================================================
 * 3D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 3D V2 arrays to any[][][] handles.
 * ============================================================================ */

RtHandleV2 *rt_array3_to_any_long_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array3_to_any_double_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array3_to_any_char_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array3_to_any_bool_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array3_to_any_byte_v2(RtArenaV2 *arena, RtHandleV2 *outer);
RtHandleV2 *rt_array3_to_any_string_v2(RtArenaV2 *arena, RtHandleV2 *outer);

/* ============================================================================
 * Any Array toString V2 Functions
 * ============================================================================ */

char *rt_to_string_array_any_v2(RtArenaV2 *arena, const RtAny *arr);
char *rt_to_string_array2_any_v2(RtArenaV2 *arena, RtHandleV2 **outer);
char *rt_to_string_array3_any_v2(RtArenaV2 *arena, RtHandleV2 **outer);

#endif /* RUNTIME_ARRAY_V2_H */
