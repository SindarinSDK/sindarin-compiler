// src/runtime/runtime_thread_promote.c
// Thread Result Promotion

/* ============================================================================
 * Thread Result Promotion
 * ============================================================================ */

/* Promote a thread result value to a destination arena
 * This handles all types appropriately:
 * - Primitives (int, long, double, bool, byte, char): copied by value
 * - Strings: promoted using rt_arena_promote_string
 * - Arrays: cloned using appropriate rt_array_clone_* function
 * - Files: promoted using rt_text_file_promote or rt_binary_file_promote
 */
void *rt_thread_promote_result(RtArena *dest, RtArena *src_arena,
                                void *value, RtResultType type, size_t value_size)
{
    if (dest == NULL) {
        fprintf(stderr, "rt_thread_promote_result: NULL dest arena\n");
        return NULL;
    }

    if (value == NULL) {
        /* NULL values are valid for void and reference types */
        return NULL;
    }

    switch (type) {
        case RT_TYPE_VOID:
            /* Void has no value to promote */
            return NULL;

        /* Primitive types - copy by value
         * Note: Sindarin int is 64-bit, stored as long long in C code
         * (on Windows/MinGW, long is 32-bit, so we must use long long) */
        case RT_TYPE_INT:
        case RT_TYPE_LONG: {
            long long *promoted = rt_arena_alloc(dest, sizeof(long long));
            if (promoted != NULL) {
                *promoted = *(long long *)value;
            }
            return promoted;
        }

        case RT_TYPE_DOUBLE: {
            double *promoted = rt_arena_alloc(dest, sizeof(double));
            if (promoted != NULL) {
                *promoted = *(double *)value;
            }
            return promoted;
        }

        case RT_TYPE_BOOL: {
            int *promoted = rt_arena_alloc(dest, sizeof(int));
            if (promoted != NULL) {
                *promoted = *(int *)value;
            }
            return promoted;
        }

        case RT_TYPE_BYTE: {
            unsigned char *promoted = rt_arena_alloc(dest, sizeof(unsigned char));
            if (promoted != NULL) {
                *promoted = *(unsigned char *)value;
            }
            return promoted;
        }

        case RT_TYPE_CHAR: {
            char *promoted = rt_arena_alloc(dest, sizeof(char));
            if (promoted != NULL) {
                *promoted = *(char *)value;
            }
            return promoted;
        }

        /* String and array types use handle-based promotion.
         * value points to an RtHandle (uint32_t) stored by the thread.
         * We promote the handle from src_arena to dest arena. */
        case RT_TYPE_STRING:
        case RT_TYPE_ARRAY_INT:
        case RT_TYPE_ARRAY_LONG:
        case RT_TYPE_ARRAY_DOUBLE:
        case RT_TYPE_ARRAY_BOOL:
        case RT_TYPE_ARRAY_BYTE:
        case RT_TYPE_ARRAY_CHAR: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            RtHandle promoted_handle = rt_managed_promote(dest, src_arena, src_handle);
            /* Allocate space in dest arena to store the promoted handle
             * so the caller can dereference to get the value */
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* String arrays need deep promotion to handle nested string handles */
        case RT_TYPE_ARRAY_STRING: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            /* Use specialized function that promotes all string elements too */
            RtHandle promoted_handle = rt_managed_promote_array_string(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* 2D arrays: outer array contains RtHandle elements pointing to inner arrays */
        case RT_TYPE_ARRAY_HANDLE: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            /* Use specialized function that promotes all inner array handles too */
            RtHandle promoted_handle = rt_managed_promote_array_handle(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* 3D arrays: outer -> middle -> inner, all contain RtHandle elements */
        case RT_TYPE_ARRAY_HANDLE_3D: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            /* Use 3D promotion function for extra level of depth */
            RtHandle promoted_handle = rt_managed_promote_array_handle_3d(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* 2D string arrays need deepest promotion - each inner array contains string handles */
        case RT_TYPE_ARRAY2_STRING: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            /* Use specialized function that promotes outer array, then each inner string array */
            RtHandle promoted_handle = rt_managed_promote_array2_string(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* 3D string arrays need three levels of promotion */
        case RT_TYPE_ARRAY3_STRING: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;
            /* Use specialized function for 3D string array promotion */
            RtHandle promoted_handle = rt_managed_promote_array3_string(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        /* any[] arrays contain RtAny elements which may reference heap data */
        case RT_TYPE_ARRAY_ANY: {
            RtHandle src_handle = *(RtHandle *)value;
            if (src_handle == RT_HANDLE_NULL) return NULL;

            /* For any arrays, we need to promote any string/array handles within the RtAny elements.
             * For now, use simple shallow promotion - this works for primitive any values. */
            RtHandle promoted_handle = rt_managed_promote(dest, src_arena, src_handle);
            RtHandle *result = rt_arena_alloc(dest, sizeof(RtHandle));
            if (result != NULL) {
                *result = promoted_handle;
            }
            return result;
        }

        case RT_TYPE_STRUCT: {
            /* Structs are stored by value in the thread's arena.
             * Copy the struct data to the destination arena and return the new pointer. */
            if (value_size == 0) {
                fprintf(stderr, "rt_thread_promote_result: RT_TYPE_STRUCT with zero size\n");
                return NULL;
            }
            void *promoted = rt_arena_alloc(dest, value_size);
            if (promoted != NULL) {
                memcpy(promoted, value, value_size);
            }
            return promoted;
        }

        default:
            fprintf(stderr, "rt_thread_promote_result: unknown type %d\n", type);
            return NULL;
    }
}
