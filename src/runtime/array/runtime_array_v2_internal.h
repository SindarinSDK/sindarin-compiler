/*
 * Runtime Array V2 Internal Header
 * =================================
 * Shared declarations for runtime_array_v2_*.c split files.
 * This header is NOT meant to be included by external code.
 */

#ifndef RUNTIME_ARRAY_V2_INTERNAL_H
#define RUNTIME_ARRAY_V2_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "runtime_array_v2.h"

/* Helper to get length from raw V2 array data pointer */
static inline size_t get_array_len_from_data(const void *arr) {
    if (arr == NULL) return 0;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)((char *)arr - sizeof(RtArrayMetadataV2));
    return meta->size;
}

/* Normalize negative index to positive */
static inline long normalize_index(long idx, size_t len) {
    if (idx < 0) {
        idx = (long)len + idx;
        if (idx < 0) idx = 0;
    }
    return idx;
}

#endif /* RUNTIME_ARRAY_V2_INTERNAL_H */
