#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Entry (as val) */
typedef struct {
    char * __sn__key;
    char * __sn__value;
} __sn__Entry;
/* Value operations */
static inline __sn__Entry __sn__Entry_copy(const __sn__Entry *src) {
    __sn__Entry dst;
    dst.__sn__key = src->__sn__key ? strdup(src->__sn__key) : NULL;
    dst.__sn__value = src->__sn__value ? strdup(src->__sn__value) : NULL;
    return dst;
}

static inline void __sn__Entry_cleanup(__sn__Entry *p) {
    free(p->__sn__key);
    free(p->__sn__value);

}

#define sn_auto_Entry __attribute__((cleanup(__sn__Entry_cleanup)))

static inline void __sn__Entry_cleanup_elem(void *p) { __sn__Entry_cleanup((__sn__Entry *)p); }
static inline void __sn__Entry_copy_into(const void *src, void *dst) { *(__sn__Entry *)dst = __sn__Entry_copy((const __sn__Entry *)src); }

/* Ref/pointer operations */
static inline __sn__Entry *__sn__Entry_alloc(void) {
    return calloc(1, sizeof(__sn__Entry));
}

static inline void __sn__Entry_release(__sn__Entry **p) {
    if (*p) {
        free((*p)->__sn__key);
        free((*p)->__sn__value);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Entry __attribute__((cleanup(__sn__Entry_release)))

static inline void __sn__Entry_release_elem(void *p) { __sn__Entry_release((__sn__Entry **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Entry_to_string(const __sn__Entry *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Entry { ");
    off += snprintf(buf + off, sizeof(buf) - off, "key: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__key ? p->__sn__key : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__value ? p->__sn__value : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    sn_auto_Entry __sn__Entry __sn__e = (__sn__Entry){ .__sn__key = strdup("name"), .__sn__value = strdup("Alice") };
    return 0LL;    fflush(stdout);
}
