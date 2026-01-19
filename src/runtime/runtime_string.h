#ifndef RUNTIME_STRING_H
#define RUNTIME_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "runtime_arena.h"
#include "runtime_array.h"

/* ============================================================================
 * String Types and Operations
 * ============================================================================
 * Sn has two types of strings:
 *
 * 1. IMMUTABLE STRINGS (no metadata):
 *    - Created by rt_str_concat(): returns new immutable string each time
 *    - String literals: compile-time constants
 *    - Have NO metadata prefix - just raw string data
 *
 * 2. MUTABLE STRINGS (with RtStringMeta):
 *    - Created with rt_string_with_capacity(): has RtStringMeta before data
 *    - Created with rt_string_from(): copies source into mutable string
 *    - Modified with rt_string_append(): efficient O(1) amortized append
 *    - Use RT_STR_META(s)->length for O(1) length access
 *
 * Only mutable strings have valid metadata. NEVER call RT_STR_META() on:
 *   - String literals
 *   - Strings from rt_str_concat()
 *
 * IMPORTANT: RT_STR_META(s) must ONLY be called on mutable strings created
 * with rt_string_with_capacity() or rt_string_from(). Using it on literal
 * strings or strings from rt_str_concat() will access invalid memory!
 * ============================================================================ */

/* String metadata - stored immediately before string data for mutable strings */
typedef struct {
    RtArena *arena;  /* Arena that owns this string (for reallocation) */
    size_t length;   /* Number of characters in the string (excluding null) */
    size_t capacity; /* Total allocated space for characters */
} RtStringMeta;

/* STRUCTURE SIZE AND MEMORY LAYOUT:
 *
 * RtStringMeta has the same memory layout philosophy as RtArrayMetadata:
 * - Both contain: arena pointer + size/length + capacity
 * - On 64-bit systems: sizeof(RtStringMeta) = 24 bytes (3 * 8 bytes)
 * - On 32-bit systems: sizeof(RtStringMeta) = 12 bytes (3 * 4 bytes)
 *
 * Memory layout for mutable strings:
 *   [RtStringMeta (24 bytes)] [string data...] [null terminator]
 *                             ^
 *                             |-- String pointer points HERE
 *
 * The metadata is stored BEFORE the string data pointer, so:
 *   RT_STR_META(s) = (RtStringMeta*)((char*)s - sizeof(RtStringMeta))
 *
 * This matches the array metadata pattern where ArrayMetadata[-1] accesses
 * the metadata stored before the array data.
 */

/* Macro to access string metadata - stored immediately before string data.
 * WARNING: Only use on mutable strings created with rt_string_with_capacity()
 * or rt_string_from(). Using on immutable strings is undefined behavior! */
#define RT_STR_META(s) ((RtStringMeta*)((char*)(s) - sizeof(RtStringMeta)))

/* ============================================================================
 * String Function Declarations
 * ============================================================================ */

/* String concatenation - allocates from arena (IMMUTABLE string, no metadata) */
char *rt_str_concat(RtArena *arena, const char *left, const char *right);

/* Create mutable string with pre-allocated capacity */
char *rt_string_with_capacity(RtArena *arena, size_t capacity);

/* Create mutable string from source (copies source into new mutable string) */
char *rt_string_from(RtArena *arena, const char *src);

/* Ensure string is mutable - converts immutable to mutable if needed */
char *rt_string_ensure_mutable(RtArena *arena, char *str);

/* Append source to mutable destination string (in-place modification) */
char *rt_string_append(char *dest, const char *src);

/* ============================================================================
 * Type-to-String Conversion Functions
 * ============================================================================ */

char *rt_to_string_long(RtArena *arena, long long val);
char *rt_to_string_double(RtArena *arena, double val);
char *rt_to_string_char(RtArena *arena, char val);
char *rt_to_string_bool(RtArena *arena, int val);
char *rt_to_string_byte(RtArena *arena, unsigned char val);
char *rt_to_string_string(RtArena *arena, const char *val);
char *rt_to_string_void(RtArena *arena);
char *rt_to_string_pointer(RtArena *arena, void *p);

/* ============================================================================
 * Format Functions (with format specifiers)
 * ============================================================================ */

char *rt_format_long(RtArena *arena, long long val, const char *fmt);
char *rt_format_double(RtArena *arena, double val, const char *fmt);
char *rt_format_string(RtArena *arena, const char *val, const char *fmt);

/* ============================================================================
 * Print Functions
 * ============================================================================ */

void rt_print_long(long long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_string(const char *s);
void rt_print_bool(long b);
void rt_print_byte(unsigned char b);

/* Array print functions (declared in runtime_array.h, included here for convenience) */
void rt_print_array_long(long long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_byte(unsigned char *arr);
void rt_print_array_string(char **arr);

/* ============================================================================
 * String Query Functions
 * ============================================================================ */

long rt_str_length(const char *str);
long rt_str_indexOf(const char *str, const char *search);
int rt_str_contains(const char *str, const char *search);
long rt_str_charAt(const char *str, long index);
char *rt_str_substring(RtArena *arena, const char *str, long start, long end);
char *rt_str_toUpper(RtArena *arena, const char *str);
char *rt_str_toLower(RtArena *arena, const char *str);
int rt_str_startsWith(const char *str, const char *prefix);
int rt_str_endsWith(const char *str, const char *suffix);
char *rt_str_trim(RtArena *arena, const char *str);
char *rt_str_replace(RtArena *arena, const char *str, const char *old, const char *new_str);
char **rt_str_split(RtArena *arena, const char *str, const char *delimiter);

/* ============================================================================
 * Inline String Functions
 * ============================================================================ */

/* Check if string is mutable - inlined for fast path in append loops */
static inline int rt_string_is_mutable(RtArena *arena, char *str) {
    if (str == NULL) return 0;
    RtStringMeta *meta = RT_STR_META(str);
    return (meta->arena == arena &&
            meta->capacity > 0 &&
            meta->capacity < (1UL << 30) &&
            meta->length <= meta->capacity);
}

/* Fast inline ensure_mutable - avoids function call when already mutable */
static inline char *rt_string_ensure_mutable_inline(RtArena *arena, char *str) {
    if (str != NULL && rt_string_is_mutable(arena, str)) {
        return str;  /* Already mutable, fast path */
    }
    return rt_string_ensure_mutable(arena, str);  /* Slow path */
}

/* Compare a region of a string with a pattern without allocating.
 * Returns 1 if str[start:end] equals pattern, 0 otherwise. */
static inline int rt_str_region_equals(const char *str, long start, long end, const char *pattern) {
    if (str == NULL || pattern == NULL) return 0;
    long pat_len = 0;
    while (pattern[pat_len]) pat_len++;
    if (end - start != pat_len) return 0;
    for (long i = 0; i < pat_len; i++) {
        if (str[start + i] != pattern[i]) return 0;
    }
    return 1;
}

/* ============================================================================
 * String Array Helpers
 * ============================================================================ */

/* Create a new string array with initial capacity */
char **rt_create_string_array(RtArena *arena, size_t initial_capacity);

/* Push a string to the array, growing if necessary */
char **rt_push_string_to_array(RtArena *arena, char **arr, char *str);

#endif /* RUNTIME_STRING_H */
