#ifndef RUNTIME_ARRAY_H
#define RUNTIME_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include "runtime_arena.h"
#include "runtime_any.h"

/* ============================================================================
 * Array Metadata
 * ============================================================================
 * Array metadata structure - stored immediately before array data in memory.
 * This allows rt_array_length to be inlined for maximum performance.
 *
 * Memory layout for arrays:
 *   [RtArrayMetadata (24 bytes)] [array data...]
 *                                ^
 *                                |-- Array pointer points HERE
 *
 * The metadata is stored BEFORE the array data pointer, so:
 *   metadata = ((RtArrayMetadata *)arr)[-1]
 *
 * On 64-bit systems: sizeof(RtArrayMetadata) = 24 bytes (3 * 8 bytes)
 * On 32-bit systems: sizeof(RtArrayMetadata) = 12 bytes (3 * 4 bytes)
 * ============================================================================ */

typedef struct {
    RtArena *arena;  /* Arena that owns this array (for reallocation) */
    size_t size;     /* Number of elements currently in the array */
    size_t capacity; /* Total allocated space for elements */
} RtArrayMetadata;

/* ============================================================================
 * Array Operations
 * ============================================================================ */

/* Get the length of an array (O(1) operation) */
static inline size_t rt_array_length(void *arr) {
    if (arr == NULL) return 0;
    return ((RtArrayMetadata *)arr)[-1].size;
}

/* Clear all elements from an array (sets size to 0, keeps capacity) */
void rt_array_clear(void *arr);

/* ============================================================================
 * Array Push Functions
 * ============================================================================
 * Push element to end of array, growing capacity if needed.
 */
long long *rt_array_push_long(RtArena *arena, long long *arr, long long element);
double *rt_array_push_double(RtArena *arena, double *arr, double element);
char *rt_array_push_char(RtArena *arena, char *arr, char element);
int *rt_array_push_bool(RtArena *arena, int *arr, int element);
unsigned char *rt_array_push_byte(RtArena *arena, unsigned char *arr, unsigned char element);
void **rt_array_push_ptr(RtArena *arena, void **arr, void *element);
char **rt_array_push_string(RtArena *arena, char **arr, const char *element);
RtAny *rt_array_push_any(RtArena *arena, RtAny *arr, RtAny element);
int32_t *rt_array_push_int32(RtArena *arena, int32_t *arr, int32_t element);
uint32_t *rt_array_push_uint32(RtArena *arena, uint32_t *arr, uint32_t element);
uint64_t *rt_array_push_uint(RtArena *arena, uint64_t *arr, uint64_t element);
float *rt_array_push_float(RtArena *arena, float *arr, float element);

/* ============================================================================
 * Array Pop Functions
 * ============================================================================
 * Remove and return the last element of an array.
 */
long long rt_array_pop_long(long long *arr);
double rt_array_pop_double(double *arr);
char rt_array_pop_char(char *arr);
int rt_array_pop_bool(int *arr);
unsigned char rt_array_pop_byte(unsigned char *arr);
void *rt_array_pop_ptr(void **arr);
char *rt_array_pop_string(char **arr);
int32_t rt_array_pop_int32(int32_t *arr);
uint32_t rt_array_pop_uint32(uint32_t *arr);
uint64_t rt_array_pop_uint(uint64_t *arr);
float rt_array_pop_float(float *arr);

/* ============================================================================
 * Array Concat Functions
 * ============================================================================
 * Return a NEW array containing elements from both arrays (non-mutating).
 */
long long *rt_array_concat_long(RtArena *arena, long long *arr1, long long *arr2);
double *rt_array_concat_double(RtArena *arena, double *arr1, double *arr2);
char *rt_array_concat_char(RtArena *arena, char *arr1, char *arr2);
int *rt_array_concat_bool(RtArena *arena, int *arr1, int *arr2);
unsigned char *rt_array_concat_byte(RtArena *arena, unsigned char *arr1, unsigned char *arr2);
void **rt_array_concat_ptr(RtArena *arena, void **arr1, void **arr2);
char **rt_array_concat_string(RtArena *arena, char **arr1, char **arr2);
int32_t *rt_array_concat_int32(RtArena *arena, int32_t *arr1, int32_t *arr2);
uint32_t *rt_array_concat_uint32(RtArena *arena, uint32_t *arr1, uint32_t *arr2);
uint64_t *rt_array_concat_uint(RtArena *arena, uint64_t *arr1, uint64_t *arr2);
float *rt_array_concat_float(RtArena *arena, float *arr1, float *arr2);

/* ============================================================================
 * Array Slice Functions
 * ============================================================================
 * Create a new array from a portion of the source array.
 */
long long *rt_array_slice_long(RtArena *arena, long long *arr, long start, long end, long step);
double *rt_array_slice_double(RtArena *arena, double *arr, long start, long end, long step);
char *rt_array_slice_char(RtArena *arena, char *arr, long start, long end, long step);
int *rt_array_slice_bool(RtArena *arena, int *arr, long start, long end, long step);
unsigned char *rt_array_slice_byte(RtArena *arena, unsigned char *arr, long start, long end, long step);
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step);
int32_t *rt_array_slice_int32(RtArena *arena, int32_t *arr, long start, long end, long step);
uint32_t *rt_array_slice_uint32(RtArena *arena, uint32_t *arr, long start, long end, long step);
uint64_t *rt_array_slice_uint(RtArena *arena, uint64_t *arr, long start, long end, long step);
float *rt_array_slice_float(RtArena *arena, float *arr, long start, long end, long step);

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return a new reversed array - the original array is not modified.
 */
long long *rt_array_rev_long(RtArena *arena, long long *arr);
double *rt_array_rev_double(RtArena *arena, double *arr);
char *rt_array_rev_char(RtArena *arena, char *arr);
int *rt_array_rev_bool(RtArena *arena, int *arr);
unsigned char *rt_array_rev_byte(RtArena *arena, unsigned char *arr);
char **rt_array_rev_string(RtArena *arena, char **arr);
int32_t *rt_array_rev_int32(RtArena *arena, int32_t *arr);
uint32_t *rt_array_rev_uint32(RtArena *arena, uint32_t *arr);
uint64_t *rt_array_rev_uint(RtArena *arena, uint64_t *arr);
float *rt_array_rev_float(RtArena *arena, float *arr);

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return a new array without the element at the specified index.
 */
long long *rt_array_rem_long(RtArena *arena, long long *arr, long index);
double *rt_array_rem_double(RtArena *arena, double *arr, long index);
char *rt_array_rem_char(RtArena *arena, char *arr, long index);
int *rt_array_rem_bool(RtArena *arena, int *arr, long index);
unsigned char *rt_array_rem_byte(RtArena *arena, unsigned char *arr, long index);
char **rt_array_rem_string(RtArena *arena, char **arr, long index);
int32_t *rt_array_rem_int32(RtArena *arena, int32_t *arr, long index);
uint32_t *rt_array_rem_uint32(RtArena *arena, uint32_t *arr, long index);
uint64_t *rt_array_rem_uint(RtArena *arena, uint64_t *arr, long index);
float *rt_array_rem_float(RtArena *arena, float *arr, long index);

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return a new array with the element inserted at the specified index.
 */
long long *rt_array_ins_long(RtArena *arena, long long *arr, long long elem, long index);
double *rt_array_ins_double(RtArena *arena, double *arr, double elem, long index);
char *rt_array_ins_char(RtArena *arena, char *arr, char elem, long index);
int *rt_array_ins_bool(RtArena *arena, int *arr, int elem, long index);
unsigned char *rt_array_ins_byte(RtArena *arena, unsigned char *arr, unsigned char elem, long index);
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index);
int32_t *rt_array_ins_int32(RtArena *arena, int32_t *arr, int32_t elem, long index);
uint32_t *rt_array_ins_uint32(RtArena *arena, uint32_t *arr, uint32_t elem, long index);
uint64_t *rt_array_ins_uint(RtArena *arena, uint64_t *arr, uint64_t elem, long index);
float *rt_array_ins_float(RtArena *arena, float *arr, float elem, long index);

/* ============================================================================
 * Array Push Copy Functions
 * ============================================================================
 * Create a NEW array with element appended (non-mutating push).
 */
long long *rt_array_push_copy_long(RtArena *arena, long long *arr, long long elem);
double *rt_array_push_copy_double(RtArena *arena, double *arr, double elem);
char *rt_array_push_copy_char(RtArena *arena, char *arr, char elem);
int *rt_array_push_copy_bool(RtArena *arena, int *arr, int elem);
unsigned char *rt_array_push_copy_byte(RtArena *arena, unsigned char *arr, unsigned char elem);
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem);
int32_t *rt_array_push_copy_int32(RtArena *arena, int32_t *arr, int32_t elem);
uint32_t *rt_array_push_copy_uint32(RtArena *arena, uint32_t *arr, uint32_t elem);
uint64_t *rt_array_push_copy_uint(RtArena *arena, uint64_t *arr, uint64_t elem);
float *rt_array_push_copy_float(RtArena *arena, float *arr, float elem);

/* ============================================================================
 * Array IndexOf Functions
 * ============================================================================
 * Find first index of element, returns -1 if not found.
 */
long rt_array_indexOf_long(long long *arr, long long elem);
long rt_array_indexOf_double(double *arr, double elem);
long rt_array_indexOf_char(char *arr, char elem);
long rt_array_indexOf_bool(int *arr, int elem);
long rt_array_indexOf_byte(unsigned char *arr, unsigned char elem);
long rt_array_indexOf_string(char **arr, const char *elem);
long rt_array_indexOf_int32(int32_t *arr, int32_t elem);
long rt_array_indexOf_uint32(uint32_t *arr, uint32_t elem);
long rt_array_indexOf_uint(uint64_t *arr, uint64_t elem);
long rt_array_indexOf_float(float *arr, float elem);

/* ============================================================================
 * Array Contains Functions
 * ============================================================================
 * Check if element exists in array.
 */
int rt_array_contains_long(long long *arr, long long elem);
int rt_array_contains_double(double *arr, double elem);
int rt_array_contains_char(char *arr, char elem);
int rt_array_contains_bool(int *arr, int elem);
int rt_array_contains_byte(unsigned char *arr, unsigned char elem);
int rt_array_contains_string(char **arr, const char *elem);
int rt_array_contains_int32(int32_t *arr, int32_t elem);
int rt_array_contains_uint32(uint32_t *arr, uint32_t elem);
int rt_array_contains_uint(uint64_t *arr, uint64_t elem);
int rt_array_contains_float(float *arr, float elem);

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a deep copy of the array.
 */
long long *rt_array_clone_long(RtArena *arena, long long *arr);
double *rt_array_clone_double(RtArena *arena, double *arr);
char *rt_array_clone_char(RtArena *arena, char *arr);
int *rt_array_clone_bool(RtArena *arena, int *arr);
unsigned char *rt_array_clone_byte(RtArena *arena, unsigned char *arr);
char **rt_array_clone_string(RtArena *arena, char **arr);
int32_t *rt_array_clone_int32(RtArena *arena, int32_t *arr);
uint32_t *rt_array_clone_uint32(RtArena *arena, uint32_t *arr);
uint64_t *rt_array_clone_uint(RtArena *arena, uint64_t *arr);
float *rt_array_clone_float(RtArena *arena, float *arr);

/* ============================================================================
 * Array Join Functions
 * ============================================================================
 * Join array elements into a string with separator.
 */
char *rt_array_join_long(RtArena *arena, long long *arr, const char *separator);
char *rt_array_join_double(RtArena *arena, double *arr, const char *separator);
char *rt_array_join_char(RtArena *arena, char *arr, const char *separator);
char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator);
char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator);
char *rt_array_join_string(RtArena *arena, char **arr, const char *separator);
char *rt_array_join_int32(RtArena *arena, int32_t *arr, const char *separator);
char *rt_array_join_uint32(RtArena *arena, uint32_t *arr, const char *separator);
char *rt_array_join_uint(RtArena *arena, uint64_t *arr, const char *separator);
char *rt_array_join_float(RtArena *arena, float *arr, const char *separator);

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create runtime array from static C array.
 */
long long *rt_array_create_long(RtArena *arena, size_t count, const long long *data);
double *rt_array_create_double(RtArena *arena, size_t count, const double *data);
char *rt_array_create_char(RtArena *arena, size_t count, const char *data);
int *rt_array_create_bool(RtArena *arena, size_t count, const int *data);
unsigned char *rt_array_create_byte(RtArena *arena, size_t count, const unsigned char *data);
char **rt_array_create_string(RtArena *arena, size_t count, const char **data);
void **rt_array_create_ptr(RtArena *arena, size_t count, void **data);
RtAny *rt_array_create_any(RtArena *arena, size_t count, const RtAny *data);
unsigned char *rt_array_create_byte_uninit(RtArena *arena, size_t count);
int32_t *rt_array_create_int32(RtArena *arena, size_t count, const int32_t *data);
uint32_t *rt_array_create_uint32(RtArena *arena, size_t count, const uint32_t *data);
uint64_t *rt_array_create_uint(RtArena *arena, size_t count, const uint64_t *data);
float *rt_array_create_float(RtArena *arena, size_t count, const float *data);

/* Create str[] array from command-line arguments (argc/argv).
 * Used by main() when it accepts a str[] parameter. */
char **rt_args_create(RtArena *arena, int argc, char **argv);

/* ============================================================================
 * Array Alloc Functions
 * ============================================================================
 * Create array of count elements filled with default_value.
 */
long long *rt_array_alloc_long(RtArena *arena, size_t count, long long default_value);
double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value);
char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value);
int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value);
unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value);
char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value);
int32_t *rt_array_alloc_int32(RtArena *arena, size_t count, int32_t default_value);
uint32_t *rt_array_alloc_uint32(RtArena *arena, size_t count, uint32_t default_value);
uint64_t *rt_array_alloc_uint(RtArena *arena, size_t count, uint64_t default_value);
float *rt_array_alloc_float(RtArena *arena, size_t count, float default_value);

/* ============================================================================
 * Array Equality Functions
 * ============================================================================
 * Check if two arrays are equal (same length and all elements equal).
 */
int rt_array_eq_long(long long *a, long long *b);
int rt_array_eq_double(double *a, double *b);
int rt_array_eq_char(char *a, char *b);
int rt_array_eq_bool(int *a, int *b);
int rt_array_eq_byte(unsigned char *a, unsigned char *b);
int rt_array_eq_string(char **a, char **b);
int rt_array_eq_int32(int32_t *a, int32_t *b);
int rt_array_eq_uint32(uint32_t *a, uint32_t *b);
int rt_array_eq_uint(uint64_t *a, uint64_t *b);
int rt_array_eq_float(float *a, float *b);

/* ============================================================================
 * Array Range Function
 * ============================================================================
 * Create array of longs from start to end (exclusive).
 */
long long *rt_array_range(RtArena *arena, long long start, long long end);

/* ============================================================================
 * Array Print Functions
 * ============================================================================
 * Print array contents to stdout.
 */
void rt_print_array_long(long long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_byte(unsigned char *arr);
void rt_print_array_string(char **arr);
void rt_print_array_int32(int32_t *arr);
void rt_print_array_uint32(uint32_t *arr);
void rt_print_array_uint(uint64_t *arr);
void rt_print_array_float(float *arr);

/* ============================================================================
 * Array ToString Functions
 * ============================================================================
 * Convert array to string representation for interpolation.
 * Format: {elem1, elem2, elem3}
 */
char *rt_to_string_array_long(RtArena *arena, long long *arr);
char *rt_to_string_array_double(RtArena *arena, double *arr);
char *rt_to_string_array_char(RtArena *arena, char *arr);
char *rt_to_string_array_bool(RtArena *arena, int *arr);
char *rt_to_string_array_byte(RtArena *arena, unsigned char *arr);
char *rt_to_string_array_string(RtArena *arena, char **arr);
char *rt_to_string_array_any(RtArena *arena, RtAny *arr);
char *rt_to_string_array_int32(RtArena *arena, int32_t *arr);
char *rt_to_string_array_uint32(RtArena *arena, uint32_t *arr);
char *rt_to_string_array_uint(RtArena *arena, uint64_t *arr);
char *rt_to_string_array_float(RtArena *arena, float *arr);

/* ============================================================================
 * Array to Any Conversion Functions
 * ============================================================================
 * Convert typed arrays to any[] by boxing each element.
 */
RtAny *rt_array_to_any_long(RtArena *arena, long long *arr);
RtAny *rt_array_to_any_double(RtArena *arena, double *arr);
RtAny *rt_array_to_any_char(RtArena *arena, char *arr);
RtAny *rt_array_to_any_bool(RtArena *arena, int *arr);
RtAny *rt_array_to_any_byte(RtArena *arena, unsigned char *arr);
RtAny *rt_array_to_any_string(RtArena *arena, char **arr);
RtAny *rt_array_to_any_int32(RtArena *arena, int32_t *arr);
RtAny *rt_array_to_any_uint32(RtArena *arena, uint32_t *arr);
RtAny *rt_array_to_any_uint(RtArena *arena, uint64_t *arr);
RtAny *rt_array_to_any_float(RtArena *arena, float *arr);

/* 2D array to any[][] conversion functions */
RtAny **rt_array2_to_any_long(RtArena *arena, long long **arr);
RtAny **rt_array2_to_any_double(RtArena *arena, double **arr);
RtAny **rt_array2_to_any_char(RtArena *arena, char **arr);
RtAny **rt_array2_to_any_bool(RtArena *arena, int **arr);
RtAny **rt_array2_to_any_byte(RtArena *arena, unsigned char **arr);
RtAny **rt_array2_to_any_string(RtArena *arena, char ***arr);

/* 3D array to any[][][] conversion functions */
RtAny ***rt_array3_to_any_long(RtArena *arena, long long ***arr);
RtAny ***rt_array3_to_any_double(RtArena *arena, double ***arr);
RtAny ***rt_array3_to_any_char(RtArena *arena, char ***arr);
RtAny ***rt_array3_to_any_bool(RtArena *arena, int ***arr);
RtAny ***rt_array3_to_any_byte(RtArena *arena, unsigned char ***arr);
RtAny ***rt_array3_to_any_string(RtArena *arena, char ****arr);

/* ============================================================================
 * Any Array to Typed Array Conversion Functions
 * ============================================================================
 * Convert any[] to typed arrays by unboxing each element.
 */
long long *rt_array_from_any_long(RtArena *arena, RtAny *arr);
double *rt_array_from_any_double(RtArena *arena, RtAny *arr);
char *rt_array_from_any_char(RtArena *arena, RtAny *arr);
int *rt_array_from_any_bool(RtArena *arena, RtAny *arr);
unsigned char *rt_array_from_any_byte(RtArena *arena, RtAny *arr);
char **rt_array_from_any_string(RtArena *arena, RtAny *arr);
int32_t *rt_array_from_any_int32(RtArena *arena, RtAny *arr);
uint32_t *rt_array_from_any_uint32(RtArena *arena, RtAny *arr);
uint64_t *rt_array_from_any_uint(RtArena *arena, RtAny *arr);
float *rt_array_from_any_float(RtArena *arena, RtAny *arr);

/* ============================================================================
 * Nested Array ToString Functions (2D arrays)
 * ============================================================================
 * Convert nested arrays to string representation.
 * Format: {{elem1, elem2}, {elem3, elem4}}
 */
char *rt_to_string_array2_long(RtArena *arena, long long **arr);
char *rt_to_string_array2_double(RtArena *arena, double **arr);
char *rt_to_string_array2_char(RtArena *arena, char **arr);
char *rt_to_string_array2_bool(RtArena *arena, int **arr);
char *rt_to_string_array2_byte(RtArena *arena, unsigned char **arr);
char *rt_to_string_array2_string(RtArena *arena, char ***arr);
char *rt_to_string_array2_any(RtArena *arena, RtAny **arr);
char *rt_to_string_array3_any(RtArena *arena, RtAny ***arr);

#endif /* RUNTIME_ARRAY_H */
