#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Vec2 (native, as ref — refcounted) */
typedef struct {
    int __rc__;
    double x;
    double y;
} __sn__Vec2;



static inline __sn__Vec2 *__sn__Vec2__new(void) {
    __sn__Vec2 *p = calloc(1, sizeof(__sn__Vec2));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Vec2 *__sn__Vec2_retain(__sn__Vec2 *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Vec2_release(__sn__Vec2 **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}


#define sn_auto_Vec2 __attribute__((cleanup(__sn__Vec2_release)))
#define sn_auto_ref_Vec2 __attribute__((cleanup(__sn__Vec2_release)))

static inline void __sn__Vec2_release_elem(void *p) { __sn__Vec2_release((__sn__Vec2 **)p); }
static inline void __sn__Vec2_retain_into(const void *src, void *dst) { *(__sn__Vec2 **)dst = __sn__Vec2_retain(*(__sn__Vec2 *const *)src); }


typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


int main() {
    return 0LL;    fflush(stdout);
}
