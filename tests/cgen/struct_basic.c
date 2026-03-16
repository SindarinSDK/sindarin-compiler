#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Point (as val) */
typedef struct {
    double __sn__x;
    double __sn__y;
} __sn__Point;
/* Value operations */
static inline __sn__Point __sn__Point_copy(const __sn__Point *src) {
    __sn__Point dst;
    dst.__sn__x = src->__sn__x;
    dst.__sn__y = src->__sn__y;
    return dst;
}

static inline void __sn__Point_cleanup(__sn__Point *p) {

}

#define sn_auto_Point __attribute__((cleanup(__sn__Point_cleanup)))

static inline void __sn__Point_cleanup_elem(void *p) { __sn__Point_cleanup((__sn__Point *)p); }
static inline void __sn__Point_copy_into(const void *src, void *dst) { *(__sn__Point *)dst = __sn__Point_copy((const __sn__Point *)src); }

/* Ref/pointer operations */
static inline __sn__Point *__sn__Point_alloc(void) {
    return calloc(1, sizeof(__sn__Point));
}

static inline void __sn__Point_release(__sn__Point **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Point __attribute__((cleanup(__sn__Point_release)))

static inline void __sn__Point_release_elem(void *p) { __sn__Point_release((__sn__Point **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Point_to_string(const __sn__Point *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Point { ");
    off += snprintf(buf + off, sizeof(buf) - off, "x: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__x);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "y: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__y);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    __sn__Point __sn__p = (__sn__Point){ .__sn__x = 1.0, .__sn__y = 2.0 };
    return 0LL;    fflush(stdout);
}
