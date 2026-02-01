/* ============================================================================
 * Handle-Based Array Functions Implementation - Deep Promotion
 * ============================================================================
 * Promotes arrays and all their handle-type elements from one arena to another.
 * Used for returning arrays from functions to prevent corruption when the
 * callee's arena is destroyed.
 * ============================================================================ */

RtHandle rt_managed_promote_array_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First, get the source array's size while src arena is still valid */
    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each string element FIRST (while src arena is valid) */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        /* Allocate temporary array on stack or heap depending on size */
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Promote each string handle from src to dest arena */
            promoted_handles[i] = rt_managed_promote(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted array with the promoted string handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

/* Deep promotion for 2D string arrays (str[][]) */
RtHandle rt_managed_promote_array2_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner string array using rt_managed_promote_array_string */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Each element is a str[] - use deep string array promotion */
            promoted_handles[i] = rt_managed_promote_array_string(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted outer array with the promoted inner string array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

/* Deep promotion for 3D string arrays (str[][][]) */
RtHandle rt_managed_promote_array3_string(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner 2D string array using rt_managed_promote_array2_string */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            /* Each element is a str[][] - use 2D string array promotion */
            promoted_handles[i] = rt_managed_promote_array2_string(dest, src, src_handles[i]);
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted outer array with the promoted inner array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

/* Internal helper with depth parameter for recursive handle array promotion */
static RtHandle rt_managed_promote_array_handle_depth(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h, int depth) {
    if (dest == NULL || src == NULL || arr_h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    void *src_raw = rt_managed_pin(src, arr_h);
    if (src_raw == NULL) return RT_HANDLE_NULL;
    RtArrayMetadata *src_meta = (RtArrayMetadata *)src_raw;
    size_t count = src_meta->size;
    RtHandle *src_handles = (RtHandle *)((char *)src_raw + sizeof(RtArrayMetadata));

    /* Promote each inner array handle FIRST (while src arena is valid) */
    RtHandle *promoted_handles = NULL;
    if (count > 0) {
        if (count <= 64) {
            promoted_handles = (RtHandle *)alloca(count * sizeof(RtHandle));
        } else {
            promoted_handles = (RtHandle *)malloc(count * sizeof(RtHandle));
        }
        for (size_t i = 0; i < count; i++) {
            if (depth > 1) {
                /* Inner arrays also contain handles - recurse with reduced depth */
                promoted_handles[i] = rt_managed_promote_array_handle_depth(dest, src, src_handles[i], depth - 1);
            } else {
                /* Innermost level - just shallow promote */
                promoted_handles[i] = rt_managed_promote(dest, src, src_handles[i]);
            }
        }
    }
    rt_managed_unpin(src, arr_h);

    /* Now promote the outer array structure itself */
    RtHandle new_arr_h = rt_managed_promote(dest, src, arr_h);
    if (new_arr_h == RT_HANDLE_NULL) {
        if (count > 64 && promoted_handles != NULL) free(promoted_handles);
        return RT_HANDLE_NULL;
    }

    /* Update the promoted array with the promoted inner array handles */
    void *dest_raw = rt_managed_pin(dest, new_arr_h);
    if (dest_raw != NULL && count > 0) {
        RtHandle *dest_handles = (RtHandle *)((char *)dest_raw + sizeof(RtArrayMetadata));
        memcpy(dest_handles, promoted_handles, count * sizeof(RtHandle));
    }
    rt_managed_unpin(dest, new_arr_h);

    if (count > 64 && promoted_handles != NULL) free(promoted_handles);
    return new_arr_h;
}

RtHandle rt_managed_promote_array_handle(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    /* Default depth=1 for 2D arrays (outer array contains handles to 1D arrays) */
    return rt_managed_promote_array_handle_depth(dest, src, arr_h, 1);
}

RtHandle rt_managed_promote_array_handle_3d(RtManagedArena *dest, RtManagedArena *src, RtHandle arr_h) {
    /* Depth=2 for 3D arrays (outer array -> 2D arrays -> 1D arrays) */
    return rt_managed_promote_array_handle_depth(dest, src, arr_h, 2);
}
