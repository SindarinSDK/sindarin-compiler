#ifndef RUNTIME_STRING_V2_H
#define RUNTIME_STRING_V2_H

#include <stddef.h>
#include "runtime/arena/arena_v2.h"

/* ============================================================================
 * Handle-Based String Functions V2
 * ============================================================================
 * RtHandleV2*-returning variants of all allocating string functions.
 *
 * Key differences from V1:
 *   - Returns RtHandleV2* instead of RtHandle
 *   - Takes RtArenaV2* instead of RtManagedArena*
 *   - No 'old' handle parameter (caller manages old handle lifetime)
 *   - pin/unpin don't require arena parameter
 * ============================================================================ */

/* String concatenation */
RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, const char *a, const char *b);

/* String append (for += operator) */
RtHandleV2 *rt_str_append_v2(RtArenaV2 *arena, const char *old_str, const char *suffix);

/* Type-to-string conversions */
RtHandleV2 *rt_to_string_long_v2(RtArenaV2 *arena, long long val);
RtHandleV2 *rt_to_string_double_v2(RtArenaV2 *arena, double val);
RtHandleV2 *rt_to_string_char_v2(RtArenaV2 *arena, char val);
RtHandleV2 *rt_to_string_bool_v2(RtArenaV2 *arena, int val);
RtHandleV2 *rt_to_string_byte_v2(RtArenaV2 *arena, unsigned char val);
RtHandleV2 *rt_to_string_string_v2(RtArenaV2 *arena, const char *val);

/* Format functions */
RtHandleV2 *rt_format_long_v2(RtArenaV2 *arena, long long val, const char *fmt);
RtHandleV2 *rt_format_double_v2(RtArenaV2 *arena, double val, const char *fmt);
RtHandleV2 *rt_format_string_v2(RtArenaV2 *arena, const char *val, const char *fmt);

/* String operations */
RtHandleV2 *rt_str_substring_v2(RtArenaV2 *arena, const char *str, long start, long end);
RtHandleV2 *rt_str_toUpper_v2(RtArenaV2 *arena, const char *str);
RtHandleV2 *rt_str_toLower_v2(RtArenaV2 *arena, const char *str);
RtHandleV2 *rt_str_trim_v2(RtArenaV2 *arena, const char *str);
RtHandleV2 *rt_str_replace_v2(RtArenaV2 *arena, const char *str, const char *old_s, const char *new_s);

/* String split -- returns array handle (array of RtHandleV2* string handles)
 * Note: Uses RtArrayMetadataV2 layout from runtime_array_v2.h */
RtHandleV2 *rt_str_split_v2(RtArenaV2 *arena, const char *str, const char *delimiter);
RtHandleV2 *rt_str_split_n_v2(RtArenaV2 *arena, const char *str, const char *delimiter, int limit);
RtHandleV2 *rt_str_split_whitespace_v2(RtArenaV2 *arena, const char *str);
RtHandleV2 *rt_str_split_lines_v2(RtArenaV2 *arena, const char *str);

/* ============================================================================
 * Raw Pointer String Functions V2
 * ============================================================================
 * These return raw char* but allocate from RtArenaV2.
 * Used for simple string interpolation where handle overhead isn't needed.
 * ============================================================================ */

char *rt_str_concat_raw_v2(RtArenaV2 *arena, const char *a, const char *b);
char *rt_to_string_long_raw_v2(RtArenaV2 *arena, long long val);
char *rt_to_string_double_raw_v2(RtArenaV2 *arena, double val);
char *rt_to_string_char_raw_v2(RtArenaV2 *arena, char val);
char *rt_to_string_bool_raw_v2(RtArenaV2 *arena, int val);
char *rt_to_string_byte_raw_v2(RtArenaV2 *arena, unsigned char val);

#endif /* RUNTIME_STRING_V2_H */
