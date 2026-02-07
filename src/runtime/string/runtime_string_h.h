#ifndef RUNTIME_STRING_H_HANDLE
#define RUNTIME_STRING_H_HANDLE

#include "runtime/arena/arena_v2.h"

/* ============================================================================
 * Handle-Based String Functions
 * ============================================================================
 * RtHandle-returning variants of all allocating string functions.
 * Pattern: takes raw pointer inputs (already pinned by caller),
 * returns RtHandle. Internally allocates via rt_managed_alloc, pins,
 * writes data, unpins, returns handle.
 * ============================================================================ */

/* String concatenation */
RtHandle rt_str_concat_h(RtManagedArena *arena, RtHandle old, const char *a, const char *b);

/* String append (for += operator) */
RtHandle rt_str_append_h(RtManagedArena *arena, RtHandle old, const char *old_str, const char *suffix);

/* Type-to-string conversions */
RtHandle rt_to_string_long_h(RtManagedArena *arena, long long val);
RtHandle rt_to_string_double_h(RtManagedArena *arena, double val);
RtHandle rt_to_string_char_h(RtManagedArena *arena, char val);
RtHandle rt_to_string_bool_h(RtManagedArena *arena, int val);
RtHandle rt_to_string_byte_h(RtManagedArena *arena, unsigned char val);
RtHandle rt_to_string_string_h(RtManagedArena *arena, const char *val);

/* Format functions */
RtHandle rt_format_long_h(RtManagedArena *arena, long long val, const char *fmt);
RtHandle rt_format_double_h(RtManagedArena *arena, double val, const char *fmt);
RtHandle rt_format_string_h(RtManagedArena *arena, const char *val, const char *fmt);

/* String operations */
RtHandle rt_str_substring_h(RtManagedArena *arena, const char *str, long start, long end);
RtHandle rt_str_toUpper_h(RtManagedArena *arena, const char *str);
RtHandle rt_str_toLower_h(RtManagedArena *arena, const char *str);
RtHandle rt_str_trim_h(RtManagedArena *arena, const char *str);
RtHandle rt_str_replace_h(RtManagedArena *arena, const char *str, const char *old_s, const char *new_s);

/* String split -- returns array handle (array of string pointers) */
RtHandle rt_str_split_h(RtManagedArena *arena, const char *str, const char *delimiter);
RtHandle rt_str_split_n_h(RtManagedArena *arena, const char *str, const char *delimiter, int limit);

#endif
