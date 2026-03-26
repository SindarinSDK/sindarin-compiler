#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Node (as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn__value;
} __sn__Node;



static inline __sn__Node *__sn__Node__new(void) {
    __sn__Node *p = calloc(1, sizeof(__sn__Node));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Node *__sn__Node_retain(__sn__Node *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Node_release(__sn__Node **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Node *__sn__Node_copy(const __sn__Node *src) {
    __sn__Node *dst = calloc(1, sizeof(__sn__Node));
    dst->__rc__ = 1;
    dst->__sn__value = src->__sn__value;
    return dst;
}

#define sn_auto_Node __attribute__((cleanup(__sn__Node_release)))
#define sn_auto_ref_Node __attribute__((cleanup(__sn__Node_release)))

static inline void __sn__Node_release_elem(void *p) { __sn__Node_release((__sn__Node **)p); }
static inline void __sn__Node_retain_into(const void *src, void *dst) { *(__sn__Node **)dst = __sn__Node_retain(*(__sn__Node *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Node_to_string(const __sn__Node *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Node { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    sn_auto_Node __sn__Node * __sn__a = ({
        __sn__Node *__tmp__ = __sn__Node__new();
        __tmp__->__sn__value = 10LL;
        __tmp__;
    });
    sn_auto_Node __sn__Node * __sn__b = __sn__Node_copy(__sn__a);
    (__sn__b->__sn__value = 20LL);
    
    sn_assert((__sn__a->__sn__value == 10LL), "original should be unchanged");
    
    sn_assert((__sn__b->__sn__value == 20LL), "copy should be 20");
    
    fflush(stdout);
    return 0;
}
