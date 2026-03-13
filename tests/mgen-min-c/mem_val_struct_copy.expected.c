#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Person (as val) */
typedef struct {
    char * __sn__name;
    long long __sn__age;
} __sn__Person;
/* Value operations */
static inline __sn__Person __sn__Person_copy(const __sn__Person *src) {
    __sn__Person dst;
    dst.__sn__name = src->__sn__name ? strdup(src->__sn__name) : NULL;
    dst.__sn__age = src->__sn__age;
    return dst;
}

static inline void __sn__Person_cleanup(__sn__Person *p) {
    free(p->__sn__name);

}

#define sn_auto_Person __attribute__((cleanup(__sn__Person_cleanup)))

static inline void __sn__Person_cleanup_elem(void *p) { __sn__Person_cleanup((__sn__Person *)p); }
static inline void __sn__Person_copy_into(const void *src, void *dst) { *(__sn__Person *)dst = __sn__Person_copy((const __sn__Person *)src); }

/* Ref/pointer operations */
static inline __sn__Person *__sn__Person_alloc(void) {
    return calloc(1, sizeof(__sn__Person));
}

static inline void __sn__Person_release(__sn__Person **p) {
    if (*p) {
        free((*p)->__sn__name);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Person __attribute__((cleanup(__sn__Person_release)))

static inline void __sn__Person_release_elem(void *p) { __sn__Person_release((__sn__Person **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Person_to_string(const __sn__Person *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Person { ");
    off += snprintf(buf + off, sizeof(buf) - off, "name: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__name ? p->__sn__name : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "age: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__age);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    sn_auto_Person __sn__Person __sn__a = (__sn__Person){ .__sn__name = strdup("Alice"), .__sn__age = 30LL };
    sn_auto_Person __sn__Person __sn__b = __sn__Person_copy(&__sn__a);
    (__sn__b.__sn__age = 99LL);
    sn_assert((__sn__a.__sn__age == 30LL), "original age should be unchanged");
    sn_assert((__sn__b.__sn__age == 99LL), "copy should have new age");
    return 0;
}
