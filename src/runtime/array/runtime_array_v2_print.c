/*
 * Runtime Array V2 Print Implementation
 * ======================================
 * Array print functions for debugging and output.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * Print Functions V2
 * ============================================================================ */

void rt_print_array_long_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        long long *arr = (long long *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%lld", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_double_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        double *arr = (double *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%.5f", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_char_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        char *arr = (char *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("'%c'", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_bool_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        int *arr = (int *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%s", arr[i] ? "true" : "false");
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_byte_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        unsigned char *arr = (unsigned char *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("0x%02X", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_string_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            if (arr[i] != NULL) {
                rt_handle_begin_transaction(arr[i]);
                const char *s = (const char *)arr[i]->ptr;
                printf("\"%s\"", s);
                rt_handle_end_transaction(arr[i]);
            } else {
                printf("null");
            }
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_int32_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        int32_t *arr = (int32_t *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%d", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_uint32_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        uint32_t *arr = (uint32_t *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%u", arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_uint_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        uint64_t *arr = (uint64_t *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%lu", (unsigned long)arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}

void rt_print_array_float_v2(RtHandleV2 *arr_h) {
    printf("[");
    if (arr_h != NULL) {
        rt_handle_begin_transaction(arr_h);
        float *arr = (float *)rt_array_data_v2(arr_h);
        size_t len = rt_array_length_v2(arr_h);
        for (size_t i = 0; i < len; i++) {
            rt_handle_renew_transaction(arr_h);
            if (i > 0) printf(", ");
            printf("%.5f", (double)arr[i]);
        }
        rt_handle_end_transaction(arr_h);
    }
    printf("]");
}
