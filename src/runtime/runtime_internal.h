#ifndef RUNTIME_INTERNAL_H
#define RUNTIME_INTERNAL_H

/* ============================================================================
 * Runtime Internal Header
 * ============================================================================
 * Shared internal declarations used by multiple runtime modules.
 * This header should only be included by runtime .c files, not by user code.
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime_arena.h"

/* ============================================================================
 * Common Macros
 * ============================================================================ */

/* Helper macro for null-checking with error reporting */
#define RT_CHECK_NULL(ptr, func_name, ret_val) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "%s: NULL argument\n", (func_name)); \
            return (ret_val); \
        } \
    } while(0)

/* Helper macro for arena null-checking */
#define RT_CHECK_ARENA(arena, func_name, ret_val) \
    do { \
        if ((arena) == NULL) { \
            fprintf(stderr, "%s: NULL arena\n", (func_name)); \
            return (ret_val); \
        } \
    } while(0)

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/* Helper for string array metadata - used by path and string modules */
typedef struct {
    long length;
    long capacity;
} RtStringArrayMeta;

/* Initialize string array metadata */
static inline void rt_init_string_array_meta(RtStringArrayMeta *meta, long capacity)
{
    meta->length = 0;
    meta->capacity = capacity;
}

/* Get metadata pointer from string array pointer */
static inline RtStringArrayMeta *rt_get_string_array_meta(char **arr)
{
    return (RtStringArrayMeta *)((char *)arr - sizeof(RtStringArrayMeta));
}

#endif /* RUNTIME_INTERNAL_H */
