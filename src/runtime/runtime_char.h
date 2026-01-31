#ifndef RUNTIME_CHAR_H
#define RUNTIME_CHAR_H

/* ============================================================================
 * Sindarin Runtime - Character Functions
 * ============================================================================
 * Functions for character manipulation and inspection.
 * ============================================================================ */

#include "runtime_arena.h"
#include "arena/managed_arena.h"
#include <ctype.h>

/* Convert char to string (single character string) - raw pointer version */
static inline char *rt_char_toString(RtArena *arena, char c) {
    char *result = rt_arena_alloc(arena, 2);
    result[0] = c;
    result[1] = '\0';
    return result;
}

/* Convert char to string (single character string) - handle version */
static inline RtHandle rt_char_toString_h(RtManagedArena *arena, char c) {
    RtHandle handle = rt_managed_alloc(arena, RT_HANDLE_NULL, 2);
    char *result = (char *)rt_managed_pin(arena, handle);
    result[0] = c;
    result[1] = '\0';
    rt_managed_unpin(arena, handle);
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
