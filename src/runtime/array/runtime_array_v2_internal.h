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
#include "runtime/runtime_any.h"

/* ============================================================================
 * Raw Array Metadata (for compatibility with V1 raw arrays)
 * ============================================================================
 * Raw arrays (non-handle) have metadata stored immediately before the data.
 * This is compatible with V1 format: [RtArrayMetadata][data...]
 * ============================================================================ */

typedef struct {
    void *arena;        /* Arena that owns this array (unused in V2 context) */
    size_t size;        /* Number of elements currently in the array */
    size_t capacity;    /* Total allocated space for elements */
} RtArrayMetadataRaw;

/* Get length of a raw array (V1-compatible format) */
static inline size_t rt_raw_array_length(const void *arr) {
    if (arr == NULL) return 0;
    return ((const RtArrayMetadataRaw *)arr)[-1].size;
}

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
