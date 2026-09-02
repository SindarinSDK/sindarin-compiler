#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: RefOps (as val) */
typedef struct {
} __sn__RefOps;
/* Value operations */
static inline __sn__RefOps __sn__RefOps_copy(const __sn__RefOps *src) {
    __sn__RefOps dst;
    return dst;
}

static inline void __sn__RefOps_cleanup(__sn__RefOps *p) {

}

#define sn_auto_RefOps __attribute__((cleanup(__sn__RefOps_cleanup)))

static inline void __sn__RefOps_cleanup_elem(void *p) { __sn__RefOps_cleanup((__sn__RefOps *)p); }
static inline void __sn__RefOps_copy_into(const void *src, void *dst) { *(__sn__RefOps *)dst = __sn__RefOps_copy((const __sn__RefOps *)src); }

/* Ref/pointer operations */
static inline __sn__RefOps *__sn__RefOps_alloc(void) {
    return calloc(1, sizeof(__sn__RefOps));
}

static inline void __sn__RefOps_release(__sn__RefOps **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_RefOps __attribute__((cleanup(__sn__RefOps_release)))

static inline void __sn__RefOps_release_elem(void *p) { __sn__RefOps_release((__sn__RefOps **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__RefOps_to_string(const __sn__RefOps *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "RefOps { ");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__oldThenIncrement(long long *);
long long __sn__intOps(long long *);
long long __sn__RefOps_longOps(long long *);
long long __sn__RefOps_intPostfix(__sn__RefOps *, long long *);
long long __sn__RefOps_longPostfix(__sn__RefOps *, long long *);
long long __sn__RefOps_intCompound(__sn__RefOps *, long long *);
long long __sn__RefOps_longCompound(__sn__RefOps *, long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__oldThenIncrement(long long *__sn__value) {

    return ({
         long long *__sn_place__ = &((*__sn__value));
         long long __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_add_long(__sn_previous__, 1);
         __sn_previous__;
     });}


long long __sn__intOps(long long *__sn__value) {

    long long __sn__add = ({
        long long __sn_rhs__ = __sn__oldThenIncrement(&(*__sn__value));
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__sub = ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__mul = ({
        long long __sn_rhs__ = 2LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__div = ({
        long long __sn_rhs__ = 2LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__rem = ({
        long long __sn_rhs__ = 3LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__old_inc = ({
        long long *__sn_place__ = &((*__sn__value));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });

    long long __sn__old_dec = ({
        long long *__sn_place__ = &((*__sn__value));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}


long long __sn__RefOps_longOps(long long *__sn__value) {

    long long __sn__add = ({
        long long __sn_rhs__ = 3LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__sub = ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__mul = ({
        long long __sn_rhs__ = 2LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__div = ({
        long long __sn_rhs__ = 2LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__rem = ({
        long long __sn_rhs__ = 3LL;
        long long *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    long long __sn__old_inc = ({
        long long *__sn_place__ = &((*__sn__value));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });

    long long __sn__old_dec = ({
        long long *__sn_place__ = &((*__sn__value));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_long(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}

long long __sn__RefOps_intPostfix(__sn__RefOps *__sn__self, long long *__sn__value) {

    return ({
         long long *__sn_place__ = &((*__sn__value));
         long long __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_sub_long(__sn_previous__, 1);
         __sn_previous__;
     });}

long long __sn__RefOps_longPostfix(__sn__RefOps *__sn__self, long long *__sn__value) {

    return ({
         long long *__sn_place__ = &((*__sn__value));
         long long __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_add_long(__sn_previous__, 1);
         __sn_previous__;
     });}

long long __sn__RefOps_intCompound(__sn__RefOps *__sn__self, long long *__sn__value) {

    return ({
         long long __sn_rhs__ = 1LL;
         long long *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}

long long __sn__RefOps_longCompound(__sn__RefOps *__sn__self, long long *__sn__value) {

    return ({
         long long __sn_rhs__ = 2LL;
         long long *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_mul_long(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}

int main() {
    long long __sn__integer = 2LL;
    long long __sn__long_value = 10LL;
    __sn__RefOps __sn__ops = (__sn__RefOps){  };
    printf("%lld\n", (long long)(__sn__intOps(&__sn__integer)));
    
    printf("%lld\n", (long long)(__sn__integer));
    
    printf("%lld\n", (long long)(__sn__RefOps_longOps(&__sn__long_value)));
    
    printf("%lld\n", (long long)(__sn__long_value));
    
    printf("%lld\n", (long long)(__sn__RefOps_intPostfix(&__sn__ops, &__sn__integer)));
    
    printf("%lld\n", (long long)(__sn__integer));
    
    printf("%lld\n", (long long)(__sn__RefOps_longPostfix(&__sn__ops, &__sn__long_value)));
    
    printf("%lld\n", (long long)(__sn__long_value));
    
    printf("%lld\n", (long long)(__sn__RefOps_intCompound(&__sn__ops, &__sn__integer)));
    
    printf("%lld\n", (long long)(__sn__integer));
    
    printf("%lld\n", (long long)(__sn__RefOps_longCompound(&__sn__ops, &__sn__long_value)));
    
    printf("%lld\n", (long long)(__sn__long_value));
    
    fflush(stdout);
    return 0;
}
