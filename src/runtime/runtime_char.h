#ifndef RUNTIME_CHAR_H
#define RUNTIME_CHAR_H

/* ============================================================================
 * Sindarin Runtime - Character Functions
 * ============================================================================
 * Functions for character manipulation and inspection.
 * ============================================================================ */

#include "arena/arena_v2.h"
#include <ctype.h>

/* Convert char to string (single character string) - V2 handle version */
static inline RtHandleV2 *rt_char_toString_v2(RtArenaV2 *arena, char c) {
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, 2);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    result[0] = c;
    result[1] = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

/* Convert char to uppercase */
static inline char rt_char_toUpper(char c) {
    return (char)toupper((unsigned char)c);
}

/* Convert char to lowercase */
static inline char rt_char_toLower(char c) {
    return (char)tolower((unsigned char)c);
}

/* Check if char is a digit (0-9) */
static inline int rt_char_isDigit(char c) {
    return isdigit((unsigned char)c) != 0;
}

/* Check if char is alphabetic (a-z, A-Z) */
static inline int rt_char_isAlpha(char c) {
    return isalpha((unsigned char)c) != 0;
}

/* Check if char is whitespace (space, tab, newline, etc.) */
static inline int rt_char_isWhitespace(char c) {
    return isspace((unsigned char)c) != 0;
}

/* Check if char is alphanumeric (a-z, A-Z, 0-9) */
static inline int rt_char_isAlnum(char c) {
    return isalnum((unsigned char)c) != 0;
}

#endif /* RUNTIME_CHAR_H */
