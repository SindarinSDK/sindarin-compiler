#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Inner (as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn__value;
} __sn__Inner;

static inline __sn__Inner *__sn__Inner_create(void) {
    __sn__Inner *p = calloc(1, sizeof(__sn__Inner));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Inner *__sn__Inner_retain(__sn__Inner *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Inner_release(__sn__Inner **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Inner *__sn__Inner_copy(const __sn__Inner *src) {
    __sn__Inner *dst = calloc(1, sizeof(__sn__Inner));
    dst->__rc__ = 1;
    dst->__sn__value = src->__sn__value;
    return dst;
}

#define sn_auto_Inner __attribute__((cleanup(__sn__Inner_release)))

static inline void __sn__Inner_release_elem(void *p) { __sn__Inner_release((__sn__Inner **)p); }
static inline void __sn__Inner_retain_into(const void *src, void *dst) { *(__sn__Inner **)dst = __sn__Inner_retain(*(__sn__Inner *const *)src); }



/* Struct: Outer (as ref — refcounted) */
typedef struct {
    int __rc__;
    __sn__Inner * __sn__child;
} __sn__Outer;

static inline __sn__Outer *__sn__Outer_create(void) {
    __sn__Outer *p = calloc(1, sizeof(__sn__Outer));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Outer *__sn__Outer_retain(__sn__Outer *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Outer_release(__sn__Outer **p) {
    if (*p && --(*p)->__rc__ == 0) {
        __sn__Inner_release(&(*p)->__sn__child);
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Outer *__sn__Outer_copy(const __sn__Outer *src) {
    __sn__Outer *dst = calloc(1, sizeof(__sn__Outer));
    dst->__rc__ = 1;
    dst->__sn__child = __sn__Inner_retain(src->__sn__child);
    return dst;
}

#define sn_auto_Outer __attribute__((cleanup(__sn__Outer_release)))

static inline void __sn__Outer_release_elem(void *p) { __sn__Outer_release((__sn__Outer **)p); }
static inline void __sn__Outer_retain_into(const void *src, void *dst) { *(__sn__Outer **)dst = __sn__Outer_retain(*(__sn__Outer *const *)src); }



int main() {
    sn_auto_Inner __sn__Inner * __sn__a = ({
        __sn__Inner *__tmp__ = __sn__Inner_create();
        __tmp__->__sn__value = 1LL;
        __tmp__;
    });
    sn_auto_Inner __sn__Inner * __sn__b = ({
        __sn__Inner *__tmp__ = __sn__Inner_create();
        __tmp__->__sn__value = 2LL;
        __tmp__;
    });
    sn_auto_Outer __sn__Outer * __sn__o = ({
        __sn__Outer *__tmp__ = __sn__Outer_create();
        __tmp__->__sn__child = __sn__Inner_retain(__sn__a);
        __tmp__;
    });
    ({
        __sn__Inner *__old__ = __sn__o->__sn__child;
        __sn__o->__sn__child = __sn__Inner_retain(__sn__b);
        __sn__Inner_release(&__old__);
        __sn__o->__sn__child;
    });
    sn_assert((__sn__o->__sn__child->__sn__value == 2LL), "child should be updated");
    return 0;
}
