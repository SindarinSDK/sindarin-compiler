#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Builder (as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn__value;
} __sn__Builder;



static inline __sn__Builder *__sn__Builder__new(void) {
    __sn__Builder *p = calloc(1, sizeof(__sn__Builder));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Builder *__sn__Builder_retain(__sn__Builder *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Builder_release(__sn__Builder **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Builder *__sn__Builder_copy(const __sn__Builder *src) {
    __sn__Builder *dst = calloc(1, sizeof(__sn__Builder));
    dst->__rc__ = 1;
    dst->__sn__value = src->__sn__value;
    return dst;
}

#define sn_auto_Builder __attribute__((cleanup(__sn__Builder_release)))
#define sn_auto_ref_Builder __attribute__((cleanup(__sn__Builder_release)))

static inline void __sn__Builder_release_elem(void *p) { __sn__Builder_release((__sn__Builder **)p); }
static inline void __sn__Builder_retain_into(const void *src, void *dst) { *(__sn__Builder **)dst = __sn__Builder_retain(*(__sn__Builder *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Builder_to_string(const __sn__Builder *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Builder { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}

__sn__Builder * __sn__Builder_setValue(__sn__Builder *, long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


__sn__Builder * __sn__Builder_setValue(__sn__Builder *__sn__self, long long __sn__v) {

    (__sn__self->__sn__value = __sn__v);
    

    return __sn__self;}

int main() {
    return 0LL;    fflush(stdout);
}
