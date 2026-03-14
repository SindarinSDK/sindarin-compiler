#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Counter (as val) */
typedef struct {
    long long __sn__value;
} __sn__Counter;
/* Value operations */
static inline __sn__Counter __sn__Counter_copy(const __sn__Counter *src) {
    __sn__Counter dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__Counter_cleanup(__sn__Counter *p) {

}

#define sn_auto_Counter __attribute__((cleanup(__sn__Counter_cleanup)))

static inline void __sn__Counter_cleanup_elem(void *p) { __sn__Counter_cleanup((__sn__Counter *)p); }
static inline void __sn__Counter_copy_into(const void *src, void *dst) { *(__sn__Counter *)dst = __sn__Counter_copy((const __sn__Counter *)src); }

/* Ref/pointer operations */
static inline __sn__Counter *__sn__Counter_alloc(void) {
    return calloc(1, sizeof(__sn__Counter));
}

static inline void __sn__Counter_release(__sn__Counter **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Counter __attribute__((cleanup(__sn__Counter_release)))

static inline void __sn__Counter_release_elem(void *p) { __sn__Counter_release((__sn__Counter **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Counter_to_string(const __sn__Counter *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Counter { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__Counter_zero();
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


long long __sn__Counter_zero() {

    return 0LL;}

int main() {
    long long __sn__z = __sn__Counter_zero();
    return __sn__z;}
