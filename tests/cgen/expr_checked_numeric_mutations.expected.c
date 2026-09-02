#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Counters (as val) */
typedef struct {
    long long __sn__i;
    long long __sn__l;
    int32_t __sn__i32;
    uint64_t __sn__u;
    uint32_t __sn__u32;
    unsigned char __sn__b;
} __sn__Counters;
/* Value operations */
static inline __sn__Counters __sn__Counters_copy(const __sn__Counters *src) {
    __sn__Counters dst;
    dst.__sn__i = src->__sn__i;
    dst.__sn__l = src->__sn__l;
    dst.__sn__i32 = src->__sn__i32;
    dst.__sn__u = src->__sn__u;
    dst.__sn__u32 = src->__sn__u32;
    dst.__sn__b = src->__sn__b;
    return dst;
}

static inline void __sn__Counters_cleanup(__sn__Counters *p) {

}

#define sn_auto_Counters __attribute__((cleanup(__sn__Counters_cleanup)))

static inline void __sn__Counters_cleanup_elem(void *p) { __sn__Counters_cleanup((__sn__Counters *)p); }
static inline void __sn__Counters_copy_into(const void *src, void *dst) { *(__sn__Counters *)dst = __sn__Counters_copy((const __sn__Counters *)src); }

/* Ref/pointer operations */
static inline __sn__Counters *__sn__Counters_alloc(void) {
    return calloc(1, sizeof(__sn__Counters));
}

static inline void __sn__Counters_release(__sn__Counters **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Counters __attribute__((cleanup(__sn__Counters_release)))

static inline void __sn__Counters_release_elem(void *p) { __sn__Counters_release((__sn__Counters **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Counters_to_string(const __sn__Counters *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Counters { ");
    off += snprintf(buf + off, sizeof(buf) - off, "i: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__i);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "l: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__l);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "i32: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__i32);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "u: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__u);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "u32: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__u32);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "b: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%u", (unsigned)p->__sn__b);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


int main() {
    long long __sn__one_i = 1LL;
    long long __sn__i = 10LL;
    long long __sn__add_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__sub_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__mul_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_mul_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__div_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_div_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__mod_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_mod_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__old_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__dec_i = ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__l = 10LL;
    long long __sn__one_l = 1LL;
    long long __sn__add_l = ({
        long long *__sn_place__ = &(__sn__l);
        long long __sn_rhs__ = __sn__one_l;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__inc_l = ({
        long long *__sn_place__ = &(__sn__l);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__dec_l = ({
        long long *__sn_place__ = &(__sn__l);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });
    int32_t __sn__i32 = 10LL;
    int32_t __sn__one_i32 = 1LL;
    int32_t __sn__add_i32 = ({
        int32_t *__sn_place__ = &(__sn__i32);
        int32_t __sn_rhs__ = __sn__one_i32;
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    int32_t __sn__inc_i32 = ({
        int32_t *__sn_place__ = &(__sn__i32);
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_int32(__sn_previous__, 1);
        __sn_previous__;
    });
    int32_t __sn__dec_i32 = ({
        int32_t *__sn_place__ = &(__sn__i32);
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_int32(__sn_previous__, 1);
        __sn_previous__;
    });
    uint64_t __sn__u = 10LL;
    uint64_t __sn__one_u = 1LL;
    uint64_t __sn__add_u = ({
        uint64_t *__sn_place__ = &(__sn__u);
        uint64_t __sn_rhs__ = __sn__one_u;
        *__sn_place__ = sn_add_uint(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    uint64_t __sn__inc_u = ({
        uint64_t *__sn_place__ = &(__sn__u);
        uint64_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint(__sn_previous__, 1);
        __sn_previous__;
    });
    uint64_t __sn__dec_u = ({
        uint64_t *__sn_place__ = &(__sn__u);
        uint64_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint(__sn_previous__, 1);
        __sn_previous__;
    });
    uint32_t __sn__u32 = 10LL;
    uint32_t __sn__one_u32 = 1LL;
    uint32_t __sn__add_u32 = ({
        uint32_t *__sn_place__ = &(__sn__u32);
        uint32_t __sn_rhs__ = __sn__one_u32;
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    uint32_t __sn__inc_u32 = ({
        uint32_t *__sn_place__ = &(__sn__u32);
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint32(__sn_previous__, 1);
        __sn_previous__;
    });
    uint32_t __sn__dec_u32 = ({
        uint32_t *__sn_place__ = &(__sn__u32);
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint32(__sn_previous__, 1);
        __sn_previous__;
    });
    unsigned char __sn__b = (unsigned char)10;
    unsigned char __sn__one_b = (unsigned char)1;
    unsigned char __sn__add_b = ({
        unsigned char *__sn_place__ = &(__sn__b);
        unsigned char __sn_rhs__ = __sn__one_b;
        *__sn_place__ = sn_add_byte(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    unsigned char __sn__inc_b = ({
        unsigned char *__sn_place__ = &(__sn__b);
        unsigned char __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_byte(__sn_previous__, 1);
        __sn_previous__;
    });
    unsigned char __sn__dec_b = ({
        unsigned char *__sn_place__ = &(__sn__b);
        unsigned char __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_byte(__sn_previous__, 1);
        __sn_previous__;
    });
    __sn__Counters __sn__fields = (__sn__Counters){ .__sn__i = 10LL, .__sn__l = 10LL, .__sn__i32 = 10LL, .__sn__u = 10LL, .__sn__u32 = 10LL, .__sn__b = (unsigned char)10 };
    long long __sn__add_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__sub_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__mul_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_mul_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__div_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_div_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__mod_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_rhs__ = __sn__one_i;
        *__sn_place__ = sn_mod_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__old_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__dec_field_i = ({
        long long *__sn_place__ = &(__sn__fields.__sn__i);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__add_field_l = ({
        long long *__sn_place__ = &(__sn__fields.__sn__l);
        long long __sn_rhs__ = __sn__one_l;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    long long __sn__inc_field_l = ({
        long long *__sn_place__ = &(__sn__fields.__sn__l);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    long long __sn__dec_field_l = ({
        long long *__sn_place__ = &(__sn__fields.__sn__l);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });
    int32_t __sn__add_field_i32 = ({
        int32_t *__sn_place__ = &(__sn__fields.__sn__i32);
        int32_t __sn_rhs__ = __sn__one_i32;
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    int32_t __sn__inc_field_i32 = ({
        int32_t *__sn_place__ = &(__sn__fields.__sn__i32);
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_int32(__sn_previous__, 1);
        __sn_previous__;
    });
    int32_t __sn__dec_field_i32 = ({
        int32_t *__sn_place__ = &(__sn__fields.__sn__i32);
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_int32(__sn_previous__, 1);
        __sn_previous__;
    });
    uint64_t __sn__add_field_u = ({
        uint64_t *__sn_place__ = &(__sn__fields.__sn__u);
        uint64_t __sn_rhs__ = __sn__one_u;
        *__sn_place__ = sn_add_uint(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    uint64_t __sn__inc_field_u = ({
        uint64_t *__sn_place__ = &(__sn__fields.__sn__u);
        uint64_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint(__sn_previous__, 1);
        __sn_previous__;
    });
    uint64_t __sn__dec_field_u = ({
        uint64_t *__sn_place__ = &(__sn__fields.__sn__u);
        uint64_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint(__sn_previous__, 1);
        __sn_previous__;
    });
    uint32_t __sn__add_field_u32 = ({
        uint32_t *__sn_place__ = &(__sn__fields.__sn__u32);
        uint32_t __sn_rhs__ = __sn__one_u32;
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    uint32_t __sn__inc_field_u32 = ({
        uint32_t *__sn_place__ = &(__sn__fields.__sn__u32);
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint32(__sn_previous__, 1);
        __sn_previous__;
    });
    uint32_t __sn__dec_field_u32 = ({
        uint32_t *__sn_place__ = &(__sn__fields.__sn__u32);
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint32(__sn_previous__, 1);
        __sn_previous__;
    });
    unsigned char __sn__add_field_b = ({
        unsigned char *__sn_place__ = &(__sn__fields.__sn__b);
        unsigned char __sn_rhs__ = __sn__one_b;
        *__sn_place__ = sn_add_byte(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    unsigned char __sn__inc_field_b = ({
        unsigned char *__sn_place__ = &(__sn__fields.__sn__b);
        unsigned char __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_byte(__sn_previous__, 1);
        __sn_previous__;
    });
    unsigned char __sn__dec_field_b = ({
        unsigned char *__sn_place__ = &(__sn__fields.__sn__b);
        unsigned char __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_byte(__sn_previous__, 1);
        __sn_previous__;
    });
    fflush(stdout);
    return 0;
}
