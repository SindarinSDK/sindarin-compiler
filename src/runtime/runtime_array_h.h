#ifndef RUNTIME_ARRAY_H_HANDLE
#define RUNTIME_ARRAY_H_HANDLE

#include <stddef.h>
#include <stdint.h>
#include "arena/managed_arena.h"
#include "runtime_any.h"

/* ============================================================================
 * Handle-Based Array Functions
 * ============================================================================
 * RtHandle-returning variants of all allocating array functions.
 * Array handle layout: [RtArrayMetadata][element data...]
 * where RtArrayMetadata is { RtArena *arena; size_t size; size_t capacity; }
 * ============================================================================ */

/* Array creation -- returns handle to [metadata][data] */
RtHandle rt_array_create_long_h(RtManagedArena *arena, size_t count, const long long *data);
RtHandle rt_array_create_double_h(RtManagedArena *arena, size_t count, const double *data);
RtHandle rt_array_create_char_h(RtManagedArena *arena, size_t count, const char *data);
RtHandle rt_array_create_bool_h(RtManagedArena *arena, size_t count, const int *data);
RtHandle rt_array_create_byte_h(RtManagedArena *arena, size_t count, const unsigned char *data);
RtHandle rt_array_create_string_h(RtManagedArena *arena, size_t count, const char **data);
RtHandle rt_array_create_int32_h(RtManagedArena *arena, size_t count, const int32_t *data);
RtHandle rt_array_create_uint32_h(RtManagedArena *arena, size_t count, const uint32_t *data);
RtHandle rt_array_create_uint_h(RtManagedArena *arena, size_t count, const uint64_t *data);
RtHandle rt_array_create_float_h(RtManagedArena *arena, size_t count, const float *data);
RtHandle rt_array_create_generic_h(RtManagedArena *arena, size_t count, size_t elem_size, const void *data);
RtHandle rt_array_create_ptr_h(RtManagedArena *arena, size_t count, void **data);

/* Array push -- may reallocate, returns possibly-new handle */
RtHandle rt_array_push_long_h(RtManagedArena *arena, RtHandle arr_h, long long element);
RtHandle rt_array_push_double_h(RtManagedArena *arena, RtHandle arr_h, double element);
RtHandle rt_array_push_char_h(RtManagedArena *arena, RtHandle arr_h, char element);
RtHandle rt_array_push_bool_h(RtManagedArena *arena, RtHandle arr_h, int element);
RtHandle rt_array_push_byte_h(RtManagedArena *arena, RtHandle arr_h, unsigned char element);
RtHandle rt_array_push_string_h(RtManagedArena *arena, RtHandle arr_h, const char *element);
RtHandle rt_array_push_int32_h(RtManagedArena *arena, RtHandle arr_h, int32_t element);
RtHandle rt_array_push_uint32_h(RtManagedArena *arena, RtHandle arr_h, uint32_t element);
RtHandle rt_array_push_uint_h(RtManagedArena *arena, RtHandle arr_h, uint64_t element);
RtHandle rt_array_push_float_h(RtManagedArena *arena, RtHandle arr_h, float element);
RtHandle rt_array_push_ptr_h(RtManagedArena *arena, RtHandle arr_h, void *element);
RtHandle rt_array_push_voidptr_h(RtManagedArena *arena, RtHandle arr_h, void *element);
RtHandle rt_array_push_any_h(RtManagedArena *arena, RtHandle arr_h, RtAny element);

/* Array pop -- modifies in place, returns popped value */
long long rt_array_pop_long_h(RtManagedArena *arena, RtHandle arr_h);
double rt_array_pop_double_h(RtManagedArena *arena, RtHandle arr_h);
char rt_array_pop_char_h(RtManagedArena *arena, RtHandle arr_h);
int rt_array_pop_bool_h(RtManagedArena *arena, RtHandle arr_h);
unsigned char rt_array_pop_byte_h(RtManagedArena *arena, RtHandle arr_h);
char *rt_array_pop_string_h(RtManagedArena *arena, RtHandle arr_h);
int32_t rt_array_pop_int32_h(RtManagedArena *arena, RtHandle arr_h);
uint32_t rt_array_pop_uint32_h(RtManagedArena *arena, RtHandle arr_h);
uint64_t rt_array_pop_uint_h(RtManagedArena *arena, RtHandle arr_h);
float rt_array_pop_float_h(RtManagedArena *arena, RtHandle arr_h);
void *rt_array_pop_ptr_h(RtManagedArena *arena, RtHandle arr_h);

/* Array clone */
RtHandle rt_array_clone_long_h(RtManagedArena *arena, RtHandle old, const long long *src);
RtHandle rt_array_clone_double_h(RtManagedArena *arena, RtHandle old, const double *src);
RtHandle rt_array_clone_char_h(RtManagedArena *arena, RtHandle old, const char *src);
RtHandle rt_array_clone_bool_h(RtManagedArena *arena, RtHandle old, const int *src);
RtHandle rt_array_clone_byte_h(RtManagedArena *arena, RtHandle old, const unsigned char *src);
RtHandle rt_array_clone_string_h(RtManagedArena *arena, RtHandle old, const char **src);
RtHandle rt_array_clone_int32_h(RtManagedArena *arena, RtHandle old, const int32_t *src);
RtHandle rt_array_clone_uint32_h(RtManagedArena *arena, RtHandle old, const uint32_t *src);
RtHandle rt_array_clone_uint_h(RtManagedArena *arena, RtHandle old, const uint64_t *src);
RtHandle rt_array_clone_float_h(RtManagedArena *arena, RtHandle old, const float *src);
RtHandle rt_array_clone_ptr_h(RtManagedArena *arena, RtHandle old, void **src);
RtHandle rt_array_clone_void_h(RtManagedArena *arena, RtHandle old, const RtAny *src);

/* Array type conversion */
RtAny *rt_array_to_any_string_h(RtManagedArena *arena, RtHandle arr_h);
RtHandle rt_array_from_legacy_string_h(RtManagedArena *arena, char **src);
char **rt_managed_pin_string_array(RtManagedArena *arena, RtHandle arr_h);

/* 2D array to any[][] conversion (handle-based) */
RtHandle rt_array2_to_any_long_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array2_to_any_double_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array2_to_any_char_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array2_to_any_bool_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array2_to_any_byte_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array2_to_any_string_h(RtManagedArena *arena, RtHandle outer);

/* 3D array to any[][][] conversion (handle-based) */
RtHandle rt_array3_to_any_long_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array3_to_any_double_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array3_to_any_char_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array3_to_any_bool_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array3_to_any_byte_h(RtManagedArena *arena, RtHandle outer);
RtHandle rt_array3_to_any_string_h(RtManagedArena *arena, RtHandle outer);

/* Array concat */
RtHandle rt_array_concat_long_h(RtManagedArena *arena, RtHandle old, const long long *a, const long long *b);
RtHandle rt_array_concat_double_h(RtManagedArena *arena, RtHandle old, const double *a, const double *b);
RtHandle rt_array_concat_char_h(RtManagedArena *arena, RtHandle old, const char *a, const char *b);
RtHandle rt_array_concat_bool_h(RtManagedArena *arena, RtHandle old, const int *a, const int *b);
RtHandle rt_array_concat_byte_h(RtManagedArena *arena, RtHandle old, const unsigned char *a, const unsigned char *b);
RtHandle rt_array_concat_string_h(RtManagedArena *arena, RtHandle old, const char **a, const char **b);
RtHandle rt_array_concat_int32_h(RtManagedArena *arena, RtHandle old, const int32_t *a, const int32_t *b);
RtHandle rt_array_concat_uint32_h(RtManagedArena *arena, RtHandle old, const uint32_t *a, const uint32_t *b);
RtHandle rt_array_concat_uint_h(RtManagedArena *arena, RtHandle old, const uint64_t *a, const uint64_t *b);
RtHandle rt_array_concat_float_h(RtManagedArena *arena, RtHandle old, const float *a, const float *b);
RtHandle rt_array_concat_ptr_h(RtManagedArena *arena, RtHandle old, void **a, void **b);

/* Array slice */
RtHandle rt_array_slice_long_h(RtManagedArena *arena, const long long *arr, long start, long end, long step);
RtHandle rt_array_slice_double_h(RtManagedArena *arena, const double *arr, long start, long end, long step);
RtHandle rt_array_slice_char_h(RtManagedArena *arena, const char *arr, long start, long end, long step);
RtHandle rt_array_slice_bool_h(RtManagedArena *arena, const int *arr, long start, long end, long step);
RtHandle rt_array_slice_byte_h(RtManagedArena *arena, const unsigned char *arr, long start, long end, long step);
RtHandle rt_array_slice_string_h(RtManagedArena *arena, const char **arr, long start, long end, long step);
RtHandle rt_array_slice_int32_h(RtManagedArena *arena, const int32_t *arr, long start, long end, long step);
RtHandle rt_array_slice_uint32_h(RtManagedArena *arena, const uint32_t *arr, long start, long end, long step);
RtHandle rt_array_slice_uint_h(RtManagedArena *arena, const uint64_t *arr, long start, long end, long step);
RtHandle rt_array_slice_float_h(RtManagedArena *arena, const float *arr, long start, long end, long step);

/* Array reverse */
RtHandle rt_array_rev_long_h(RtManagedArena *arena, const long long *arr);
RtHandle rt_array_rev_double_h(RtManagedArena *arena, const double *arr);
RtHandle rt_array_rev_char_h(RtManagedArena *arena, const char *arr);
RtHandle rt_array_rev_bool_h(RtManagedArena *arena, const int *arr);
RtHandle rt_array_rev_byte_h(RtManagedArena *arena, const unsigned char *arr);
RtHandle rt_array_rev_string_h(RtManagedArena *arena, const char **arr);

/* Array remove at index */
RtHandle rt_array_rem_long_h(RtManagedArena *arena, const long long *arr, long index);
RtHandle rt_array_rem_double_h(RtManagedArena *arena, const double *arr, long index);
RtHandle rt_array_rem_char_h(RtManagedArena *arena, const char *arr, long index);
RtHandle rt_array_rem_bool_h(RtManagedArena *arena, const int *arr, long index);
RtHandle rt_array_rem_byte_h(RtManagedArena *arena, const unsigned char *arr, long index);
RtHandle rt_array_rem_string_h(RtManagedArena *arena, const char **arr, long index);

/* Array insert at index */
RtHandle rt_array_ins_long_h(RtManagedArena *arena, const long long *arr, long long elem, long index);
RtHandle rt_array_ins_double_h(RtManagedArena *arena, const double *arr, double elem, long index);
RtHandle rt_array_ins_char_h(RtManagedArena *arena, const char *arr, char elem, long index);
RtHandle rt_array_ins_bool_h(RtManagedArena *arena, const int *arr, int elem, long index);
RtHandle rt_array_ins_byte_h(RtManagedArena *arena, const unsigned char *arr, unsigned char elem, long index);
RtHandle rt_array_ins_string_h(RtManagedArena *arena, const char **arr, const char *elem, long index);

/* Array push copy (non-mutating) */
RtHandle rt_array_push_copy_long_h(RtManagedArena *arena, const long long *arr, long long elem);
RtHandle rt_array_push_copy_double_h(RtManagedArena *arena, const double *arr, double elem);
RtHandle rt_array_push_copy_char_h(RtManagedArena *arena, const char *arr, char elem);
RtHandle rt_array_push_copy_bool_h(RtManagedArena *arena, const int *arr, int elem);
RtHandle rt_array_push_copy_byte_h(RtManagedArena *arena, const unsigned char *arr, unsigned char elem);
RtHandle rt_array_push_copy_string_h(RtManagedArena *arena, const char **arr, const char *elem);

/* Array alloc (with default value) */
RtHandle rt_array_alloc_long_h(RtManagedArena *arena, size_t count, long long default_value);
RtHandle rt_array_alloc_double_h(RtManagedArena *arena, size_t count, double default_value);
RtHandle rt_array_alloc_char_h(RtManagedArena *arena, size_t count, char default_value);
RtHandle rt_array_alloc_bool_h(RtManagedArena *arena, size_t count, int default_value);
RtHandle rt_array_alloc_byte_h(RtManagedArena *arena, size_t count, unsigned char default_value);
RtHandle rt_array_alloc_string_h(RtManagedArena *arena, size_t count, const char *default_value);

/* Array range */
RtHandle rt_array_range_h(RtManagedArena *arena, long long start, long long end);

/* Convert legacy char** array (raw string pointers) to handle-based string array */
RtHandle rt_array_from_raw_strings_h(RtManagedArena *arena, RtHandle old, const char **src);

/* String array equality */
int rt_array_eq_string_h(RtManagedArena *arena, RtHandle a_h, RtHandle b_h);

/* Args creation */
RtHandle rt_args_create_h(RtManagedArena *arena, int argc, char **argv);

/* Handle-aware to-string for 1D string arrays (RtHandle elements) */
char *rt_to_string_array_string_h(RtManagedArena *arena, RtHandle *arr);

/* Handle-aware join for string arrays (RtHandle elements) */
char *rt_array_join_string_h(RtManagedArena *arena, RtHandle *arr, const char *separator);

/* Handle-aware print for string arrays (RtHandle elements) */
void rt_print_array_string_h(RtManagedArena *arena, RtHandle *arr);

/* Handle-aware indexOf/contains for string arrays (RtHandle elements) */
long rt_array_indexOf_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem);
int rt_array_contains_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem);

/* Handle-aware to-string for 2D arrays (outer stores RtHandle elements) */
char *rt_to_string_array2_long_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_double_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_char_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_bool_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_byte_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_string_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array2_any_h(RtManagedArena *arena, RtHandle *outer);
char *rt_to_string_array3_any_h(RtManagedArena *arena, RtHandle *outer);

#endif
