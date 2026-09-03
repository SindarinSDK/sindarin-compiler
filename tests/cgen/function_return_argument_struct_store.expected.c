#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Holder (as val) */
typedef struct {
    void * __sn__callback;
} __sn__Holder;
/* Value operations */
static inline __sn__Holder __sn__Holder_copy(const __sn__Holder *src) {
    __sn__Holder dst;
    dst.__sn__callback = sn_closure_retain(src->__sn__callback);
    return dst;
}

static inline void __sn__Holder_cleanup(__sn__Holder *p) {
    sn_closure_release((void **)&p->__sn__callback);

}

#define sn_auto_Holder __attribute__((cleanup(__sn__Holder_cleanup)))

static inline void __sn__Holder_cleanup_elem(void *p) { __sn__Holder_cleanup((__sn__Holder *)p); }
static inline void __sn__Holder_copy_into(const void *src, void *dst) { *(__sn__Holder *)dst = __sn__Holder_copy((const __sn__Holder *)src); }

/* Ref/pointer operations */
static inline __sn__Holder *__sn__Holder_alloc(void) {
    return calloc(1, sizeof(__sn__Holder));
}

static inline void __sn__Holder_release(__sn__Holder **p) {
    if (*p) {
        sn_closure_release((void **)&(*p)->__sn__callback);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Holder __attribute__((cleanup(__sn__Holder_release)))

static inline void __sn__Holder_release_elem(void *p) { __sn__Holder_release((__sn__Holder **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Holder_to_string(const __sn__Holder *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Holder { ");
    off += snprintf(buf + off, sizeof(buf) - off, "callback: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__callback);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



void * __sn__makeAdder(long long);
__sn__Holder __sn__store(void *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;

typedef struct __closure_0__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
    long long amount;
} __closure_0__;
static long long __lambda_0__(void *__closure__, long long __sn__value);


void * __sn__makeAdder(long long __sn__amount) {

    return ({
         __closure_0__ *__cl__ = malloc(sizeof(__closure_0__));
         __cl__->fn = (void *)__lambda_0__;
         __cl__->size = sizeof(__closure_0__);
         __cl__->__cleanup__ = NULL;
         __cl__->__rc__ = 1;
         __cl__->amount = __sn__amount;
         __cl__;
     });}


__sn__Holder __sn__store(void * __sn__callback) {

    return (__sn__Holder){ .__sn__callback = __sn__callback };}


int main() {
    void * __sn____chain_tmp_0 = __sn__makeAdder(40LL);
    sn_auto_Holder __sn__Holder __sn__holder = __sn__store(__sn____chain_tmp_0);
    printf("%lld", (long long)(((long long (*)(void *, long long))((__Closure__ *)(__sn__holder.__sn__callback))->fn)(__sn__holder.__sn__callback, 2LL)));
    
    fflush(stdout);
    return 0;
}

static long long __lambda_0__(void *__closure__, long long __sn__value) {

    long long __sn__amount = ((__closure_0__ *)__closure__)->amount;
    return sn_add_long(__sn__value, __sn__amount);
}
