#ifndef RUNTIME_STRING_V2_H
#define RUNTIME_STRING_V2_H

#include <stddef.h>
#include "runtime/arena/arena_v2.h"

/* ============================================================================
 * Handle-Based String Functions
 * ============================================================================
 * All string functions accept RtHandleV2* for string parameters and manage
 * transactions internally. Callers must never extract raw char* from handles
 * to pass to these functions — pass the handle directly.
 *
 * String literals must be wrapped in rt_arena_v2_strdup() before being passed
 * to these functions.
 * ============================================================================ */

/* String concatenation */
RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, RtHandleV2 *a, RtHandleV2 *b);

/* String append (for += operator) */
RtHandleV2 *rt_str_append_v2(RtArenaV2 *arena, RtHandleV2 *old_str, RtHandleV2 *suffix);

/* Type-to-string conversions (primitive inputs — no change needed) */
RtHandleV2 *rt_to_string_long_v2(RtArenaV2 *arena, long long val);
RtHandleV2 *rt_to_string_double_v2(RtArenaV2 *arena, double val);
RtHandleV2 *rt_to_string_char_v2(RtArenaV2 *arena, char val);
RtHandleV2 *rt_to_string_bool_v2(RtArenaV2 *arena, int val);
RtHandleV2 *rt_to_string_byte_v2(RtArenaV2 *arena, unsigned char val);
RtHandleV2 *rt_to_string_string_v2(RtArenaV2 *arena, RtHandleV2 *val);

/* Format functions (fmt is a compile-time format string, stays const char*) */
RtHandleV2 *rt_format_long_v2(RtArenaV2 *arena, long long val, const char *fmt);
RtHandleV2 *rt_format_double_v2(RtArenaV2 *arena, double val, const char *fmt);
RtHandleV2 *rt_format_string_v2(RtArenaV2 *arena, RtHandleV2 *val, const char *fmt);

/* String operations */
RtHandleV2 *rt_str_substring_v2(RtArenaV2 *arena, RtHandleV2 *str, long start, long end);
RtHandleV2 *rt_str_toUpper_v2(RtArenaV2 *arena, RtHandleV2 *str);
RtHandleV2 *rt_str_toLower_v2(RtArenaV2 *arena, RtHandleV2 *str);
RtHandleV2 *rt_str_trim_v2(RtArenaV2 *arena, RtHandleV2 *str);
RtHandleV2 *rt_str_replace_v2(RtArenaV2 *arena, RtHandleV2 *str, RtHandleV2 *old_s, RtHandleV2 *new_s);

/* String split -- returns array handle (array of RtHandleV2* string handles)
 * Note: Uses RtArrayMetadataV2 layout from runtime_array_v2.h */
RtHandleV2 *rt_str_split_v2(RtArenaV2 *arena, RtHandleV2 *str, RtHandleV2 *delimiter);
RtHandleV2 *rt_str_split_n_v2(RtArenaV2 *arena, RtHandleV2 *str, RtHandleV2 *delimiter, int limit);
RtHandleV2 *rt_str_split_whitespace_v2(RtArenaV2 *arena, RtHandleV2 *str);
RtHandleV2 *rt_str_split_lines_v2(RtArenaV2 *arena, RtHandleV2 *str);

/* String query functions - handle-based */
long rt_str_length_v2(RtHandleV2 *str);
long rt_str_indexOf_v2(RtHandleV2 *str, RtHandleV2 *search);
int rt_str_contains_v2(RtHandleV2 *str, RtHandleV2 *search);
long rt_str_charAt_v2(RtHandleV2 *str, long index);
int rt_str_startsWith_v2(RtHandleV2 *str, RtHandleV2 *prefix);
int rt_str_endsWith_v2(RtHandleV2 *str, RtHandleV2 *suffix);
int rt_str_region_equals_v2(RtHandleV2 *str, long start, long end, RtHandleV2 *pattern);
int rt_str_is_blank_v2(RtHandleV2 *str);

/* String parse functions - handle-based */
long long rt_str_to_int_v2(RtHandleV2 *str);
long long rt_str_to_long_v2(RtHandleV2 *str);
double rt_str_to_double_v2(RtHandleV2 *str);

#endif /* RUNTIME_STRING_V2_H */
