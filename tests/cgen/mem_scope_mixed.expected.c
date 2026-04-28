#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Tag (as ref — refcounted) */
typedef struct {
    int __rc__;
    char * __sn__label;
} __sn__Tag;



static inline __sn__Tag *__sn__Tag__new(void) {
    __sn__Tag *p = calloc(1, sizeof(__sn__Tag));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Tag *__sn__Tag_retain(__sn__Tag *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Tag_release(__sn__Tag **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free((*p)->__sn__label);
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Tag *__sn__Tag_copy(const __sn__Tag *src) {
    __sn__Tag *dst = calloc(1, sizeof(__sn__Tag));
    dst->__rc__ = 1;
    dst->__sn__label = src->__sn__label ? strdup(src->__sn__label) : NULL;
    return dst;
}

#define sn_auto_Tag __attribute__((cleanup(__sn__Tag_release)))
#define sn_auto_ref_Tag __attribute__((cleanup(__sn__Tag_release)))

static inline void __sn__Tag_release_elem(void *p) { __sn__Tag_release((__sn__Tag **)p); }
static inline void __sn__Tag_retain_into(const void *src, void *dst) { *(__sn__Tag **)dst = __sn__Tag_retain(*(__sn__Tag *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Tag_to_string(const __sn__Tag *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Tag { ");
    off += snprintf(buf + off, sizeof(buf) - off, "label: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__label ? p->__sn__label : "nil");
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
    sn_auto_str char * __sn__s = strdup("hello");
    sn_auto_Tag __sn__Tag * __sn__t = ({
        __sn__Tag *__tmp__ = __sn__Tag__new();
        __tmp__->__sn__label = strdup("tag1");
        __tmp__;
    });
    long long __sn__n = 42LL;
    sn_assert((sn_str_length(__sn__s) == 5LL), "string ok");
    
    sn_assert((sn_str_length(__sn__t->__sn__label) == 4LL), "tag ok");
    
    sn_assert((__sn__n == 42LL), "int ok");
    
    fflush(stdout);
    return 0;
}
