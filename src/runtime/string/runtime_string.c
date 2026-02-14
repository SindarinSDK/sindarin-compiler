#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "runtime_string.h"

/* ============================================================================
 * String Operations Implementation
 * ============================================================================
 * This module provides string manipulation functions for the Sn runtime:
 * - Immutable string concatenation (rt_str_concat)
 * - Mutable string operations (rt_string_with_capacity, rt_string_from, etc.)
 * - Type-to-string conversion (rt_to_string_*)
 * - Format functions (rt_format_*)
 * - Print functions (rt_print_*)
 * ============================================================================ */

/* ============================================================================
 * Immutable String Concatenation
 * ============================================================================
 * Creates a new immutable string from concatenating two strings.
 * Returns NULL on allocation failure or if result would exceed 1GB.
 *
 * This is the appropriate behavior for functional-style string operations.
 *
 * TODO: Future optimization - if left string is mutable and has enough
 * capacity, could append in-place using rt_string_append() instead. */
RtHandleV2 *rt_str_concat(RtArenaV2 *arena, const char *left, const char *right) {
    const char *l = left ? left : "";
    const char *r = right ? right : "";
    size_t left_len = strlen(l);
    size_t right_len = strlen(r);
    size_t new_len = left_len + right_len;
    if (new_len > (1UL << 30) - 1) {
        return NULL;
    }
    RtHandleV2 *new_str_h = rt_arena_v2_alloc(arena, new_len + 1);
    if (new_str_h == NULL) {
        return NULL;
    }
    rt_handle_begin_transaction(new_str_h);
    char *new_str = (char *)new_str_h->ptr;
    memcpy(new_str, l, left_len);
    memcpy(new_str + left_len, r, right_len + 1);
    rt_handle_end_transaction(new_str_h);
    return new_str_h;
}

/* Include split modules */
#include "runtime_string_mutable.c"
#include "runtime_string_convert.c"
#include "runtime_string_format.c"
#include "runtime_string_print.c"
#include "runtime_string_query.c"
#include "runtime_string_split.c"
#include "runtime_string_parse.c"
#include "runtime_string_array.c"
