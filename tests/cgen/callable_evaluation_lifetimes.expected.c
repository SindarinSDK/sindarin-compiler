#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Holder (as val) */
typedef struct {
    void * __sn__action;
} __sn__Holder;
/* Value operations */
static inline __sn__Holder __sn__Holder_copy(const __sn__Holder *src) {
    __sn__Holder dst;
    dst.__sn__action = sn_closure_retain(src->__sn__action);
    return dst;
}

static inline void __sn__Holder_cleanup(__sn__Holder *p) {
    sn_closure_release((void **)&p->__sn__action);

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
        sn_closure_release((void **)&(*p)->__sn__action);
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
    off += snprintf(buf + off, sizeof(buf) - off, "action: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__action);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__increment(long long);
void * __sn__identity(void *);
long long __sn__index();
__sn__Holder __sn__factory();
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;

static long long __lambda_0__(void *__closure__);

static long long __lambda_1__(void *__closure__);

static long long __lambda_2__(void *__closure__);

static long long __lambda_3__(void *__closure__);

static long long __fn_wrap_0__(void *__closure__, long long __p0__) {
    (void)__closure__;
    return __sn__increment(__p0__);
 }


long long __sn__increment(long long __sn__value) {

    return sn_add_long(__sn__value, 1LL);}


void * __sn__identity(void * __sn__action) {

    return sn_closure_retain(__sn__action);}


long long __sn__index() {

    sn_println("index");
    

    return 0LL;}


__sn__Holder __sn__factory() {

    sn_auto_Holder __sn__Holder __sn__result = (__sn__Holder){ .__sn__action = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__Closure__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__;
    }) };

    {
        __sn__Holder __ret__ = __sn__result;
        memset(&__sn__result, 0, sizeof(__sn__result));
        return __ret__;
    }}


int main() {
    sn_auto_fn void * __sn__named = ({ __Closure__ *__cl__ = malloc(sizeof(__Closure__)); __cl__->fn = (void *)__fn_wrap_0__; __cl__->size = sizeof(__Closure__); __cl__->__cleanup__ = NULL; __cl__->__rc__ = 1; __cl__; });
    printf("%lld\n", (long long)(({ sn_auto_fn void *__callee__ = sn_closure_retain(__sn__named); ((long long (*)(void *, long long))((__Closure__ *)__callee__)->fn)(__callee__, 1LL); })));
    
    sn_auto_fn void * __sn__first = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_1__;
        __cl__->size = sizeof(__Closure__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__;
    });
    sn_auto_fn void * __sn__copied = __sn__identity(__sn__first);
    ({
        void *__old_cl__ = __sn__first;
        __sn__first = sn_closure_retain(__sn__first);
        sn_closure_release(&__old_cl__);
        __sn__first;
    });
    
    ({
        void *__old_cl__ = __sn__first;
        __sn__first = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_2__;
        __cl__->size = sizeof(__Closure__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__;
    });
        sn_closure_release(&__old_cl__);
        __sn__first;
    });
    
    printf("%lld\n", (long long)(({ sn_auto_fn void *__callee__ = sn_closure_retain(__sn__copied); ((long long (*)(void *))((__Closure__ *)__callee__)->fn)(__callee__); })));
    
    sn_auto_Holder __sn__Holder __sn__holder = (__sn__Holder){ .__sn__action = sn_closure_retain(__sn__copied) };
    ({
        void **__slot__ = &(__sn__holder.__sn__action);
        void *__new_cl__ = sn_closure_retain(__sn__holder.__sn__action);
        void *__old_cl__ = *__slot__;
        *__slot__ = __new_cl__;
        sn_closure_release(&__old_cl__);
        *__slot__;
    });
    
    printf("%lld\n", (long long)(({ sn_auto_fn void *__callee__ = sn_closure_retain(__sn__holder.__sn__action); ((long long (*)(void *))((__Closure__ *)__callee__)->fn)(__callee__); })));
    
    sn_auto_arr SnArray * __sn__actions = ({
            SnArray *__al__ = sn_array_new(sizeof(void *), 1);
    
            __al__->elem_release = sn_release_closure_elem;
    
            __al__->elem_copy = sn_copy_closure;
    
            sn_array_push(__al__, &(void *){ sn_closure_retain(__sn__copied) });
            __al__;
        });
    ({
        SnArray *__ia_array__ = __sn__actions;
        long long __ai__ = 0LL; if (__ai__ < 0) __ai__ += __ia_array__->len;
        void *__new_cl__ = sn_closure_retain((((void * *)__sn__actions->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__actions->len : __ai__; })]));
        void **__slot__ = &((void **)__ia_array__->data)[__ai__];
        void *__old_cl__ = *__slot__;
        *__slot__ = __new_cl__;
        sn_closure_release(&__old_cl__);
        *__slot__;
    });
    
    ({
        SnArray *__ia_array__ = __sn__actions;
        long long __ai__ = 0LL; if (__ai__ < 0) __ai__ += __ia_array__->len;
        void *__new_cl__ = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_3__;
        __cl__->size = sizeof(__Closure__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__;
    });
        void **__slot__ = &((void **)__ia_array__->data)[__ai__];
        void *__old_cl__ = *__slot__;
        *__slot__ = __new_cl__;
        sn_closure_release(&__old_cl__);
        *__slot__;
    });
    
    printf("%lld\n", (long long)(({ sn_auto_fn void *__callee__ = sn_closure_retain((((void * *)__sn__actions->data)[({ long long __ai__ = __sn__index(); __ai__ < 0 ? __ai__ + __sn__actions->len : __ai__; })])); ((long long (*)(void *))((__Closure__ *)__callee__)->fn)(__callee__); })));
    
    sn_auto_Holder __sn__Holder __sn____chain_tmp_0 = __sn__factory();
    printf("%lld\n", (long long)(({ sn_auto_fn void *__callee__ = sn_closure_retain(__sn____chain_tmp_0.__sn__action); ((long long (*)(void *))((__Closure__ *)__callee__)->fn)(__callee__); })));
    
    fflush(stdout);
    return 0;
}

static long long __lambda_0__(void *__closure__) {
    return 7LL;
}

static long long __lambda_1__(void *__closure__) {
    return 3LL;
}

static long long __lambda_2__(void *__closure__) {
    return 4LL;
}

static long long __lambda_3__(void *__closure__) {
    return 5LL;
}
