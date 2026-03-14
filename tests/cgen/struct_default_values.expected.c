#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Config (as val) */
typedef struct {
    long long __sn__width;
    long long __sn__height;
    double __sn__scale;
} __sn__Config;
/* Value operations */
static inline __sn__Config __sn__Config_copy(const __sn__Config *src) {
    __sn__Config dst;
    dst.__sn__width = src->__sn__width;
    dst.__sn__height = src->__sn__height;
    dst.__sn__scale = src->__sn__scale;
    return dst;
}

static inline void __sn__Config_cleanup(__sn__Config *p) {

}

#define sn_auto_Config __attribute__((cleanup(__sn__Config_cleanup)))

static inline void __sn__Config_cleanup_elem(void *p) { __sn__Config_cleanup((__sn__Config *)p); }
static inline void __sn__Config_copy_into(const void *src, void *dst) { *(__sn__Config *)dst = __sn__Config_copy((const __sn__Config *)src); }

/* Ref/pointer operations */
static inline __sn__Config *__sn__Config_alloc(void) {
    return calloc(1, sizeof(__sn__Config));
}

static inline void __sn__Config_release(__sn__Config **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Config __attribute__((cleanup(__sn__Config_release)))

static inline void __sn__Config_release_elem(void *p) { __sn__Config_release((__sn__Config **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Config_to_string(const __sn__Config *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Config { ");
    off += snprintf(buf + off, sizeof(buf) - off, "width: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__width);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "height: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__height);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "scale: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__scale);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    __sn__Config __sn__c = (__sn__Config){ .__sn__width = 800LL, .__sn__height = 600LL, .__sn__scale = 1.0 };
    sn_assert((__sn__c.__sn__width == 800LL), "expected default width to be 800");
    sn_assert((__sn__c.__sn__height == 600LL), "expected default height to be 600");
    return 0;
}
