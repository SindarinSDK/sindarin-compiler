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



static inline __sn__Inner *__sn__Inner__new(void) {
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
#define sn_auto_ref_Inner __attribute__((cleanup(__sn__Inner_release)))

static inline void __sn__Inner_release_elem(void *p) { __sn__Inner_release((__sn__Inner **)p); }
static inline void __sn__Inner_retain_into(const void *src, void *dst) { *(__sn__Inner **)dst = __sn__Inner_retain(*(__sn__Inner *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Inner_to_string(const __sn__Inner *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Inner { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}

/* Struct: Outer (as ref — refcounted) */
typedef struct {
    int __rc__;
    __sn__Inner * __sn__child;
} __sn__Outer;



static inline __sn__Outer *__sn__Outer__new(void) {
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
#define sn_auto_ref_Outer __attribute__((cleanup(__sn__Outer_release)))

static inline void __sn__Outer_release_elem(void *p) { __sn__Outer_release((__sn__Outer **)p); }
static inline void __sn__Outer_retain_into(const void *src, void *dst) { *(__sn__Outer **)dst = __sn__Outer_retain(*(__sn__Outer *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Outer_to_string(const __sn__Outer *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Outer { ");
    off += snprintf(buf + off, sizeof(buf) - off, "child: ");
    { char *__fs__ = __sn__Inner_to_string(&p->__sn__child); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;



int main() {
    sn_auto_Inner __sn__Inner * __sn__a = ({
        __sn__Inner *__tmp__ = __sn__Inner__new();
        __tmp__->__sn__value = 1LL;
        __tmp__;
    });
    sn_auto_Inner __sn__Inner * __sn__b = ({
        __sn__Inner *__tmp__ = __sn__Inner__new();
        __tmp__->__sn__value = 2LL;
        __tmp__;
    });
    sn_auto_Outer __sn__Outer * __sn__o = ({
        __sn__Outer *__tmp__ = __sn__Outer__new();
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
    
    fflush(stdout);
    return 0;
}
