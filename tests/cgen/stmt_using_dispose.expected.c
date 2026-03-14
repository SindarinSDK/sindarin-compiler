#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Resource (as val) */
typedef struct {
    char * __sn__name;
} __sn__Resource;
/* Value operations */
static inline __sn__Resource __sn__Resource_copy(const __sn__Resource *src) {
    __sn__Resource dst;
    dst.__sn__name = src->__sn__name ? strdup(src->__sn__name) : NULL;
    return dst;
}

static inline void __sn__Resource_cleanup(__sn__Resource *p) {
    free(p->__sn__name);

}

#define sn_auto_Resource __attribute__((cleanup(__sn__Resource_cleanup)))

static inline void __sn__Resource_cleanup_elem(void *p) { __sn__Resource_cleanup((__sn__Resource *)p); }
static inline void __sn__Resource_copy_into(const void *src, void *dst) { *(__sn__Resource *)dst = __sn__Resource_copy((const __sn__Resource *)src); }

/* Ref/pointer operations */
static inline __sn__Resource *__sn__Resource_alloc(void) {
    return calloc(1, sizeof(__sn__Resource));
}

static inline void __sn__Resource_release(__sn__Resource **p) {
    if (*p) {
        free((*p)->__sn__name);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Resource __attribute__((cleanup(__sn__Resource_release)))

static inline void __sn__Resource_release_elem(void *p) { __sn__Resource_release((__sn__Resource **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Resource_to_string(const __sn__Resource *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Resource { ");
    off += snprintf(buf + off, sizeof(buf) - off, "name: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__name ? p->__sn__name : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



bool __sn__disposed = false;

void __sn__Resource_dispose(__sn__Resource *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


void __sn__Resource_dispose(__sn__Resource *__sn__self) {

    (__sn__disposed = true);
}

int main() {
    {
        sn_auto_Resource __sn__Resource __sn__r = (__sn__Resource){ .__sn__name = strdup("test") };
    {
        sn_assert((strcmp(__sn__r.__sn__name, "test") == 0), "expected resource name to be test");
    }
        __sn__Resource_dispose(&__sn__r);
    }
    sn_assert(__sn__disposed, "expected dispose to be called after using block");
    return 0;
}
