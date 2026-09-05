#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Item (as val) */
typedef struct {
    char * __sn__text;
} __sn__Item;
/* Value operations */
static inline __sn__Item __sn__Item_copy(const __sn__Item *src) {
    __sn__Item dst;
    dst.__sn__text = src->__sn__text ? strdup(src->__sn__text) : NULL;
    return dst;
}

static inline void __sn__Item_cleanup(__sn__Item *p) {
    free(p->__sn__text);

}

#define sn_auto_Item __attribute__((cleanup(__sn__Item_cleanup)))

static inline void __sn__Item_cleanup_elem(void *p) { __sn__Item_cleanup((__sn__Item *)p); }
static inline void __sn__Item_copy_into(const void *src, void *dst) { *(__sn__Item *)dst = __sn__Item_copy((const __sn__Item *)src); }

/* Ref/pointer operations */
static inline __sn__Item *__sn__Item_alloc(void) {
    return calloc(1, sizeof(__sn__Item));
}

static inline void __sn__Item_release(__sn__Item **p) {
    if (*p) {
        free((*p)->__sn__text);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Item __attribute__((cleanup(__sn__Item_release)))

static inline void __sn__Item_release_elem(void *p) { __sn__Item_release((__sn__Item **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Item_to_string(const __sn__Item *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Item { ");
    off += snprintf(buf + off, sizeof(buf) - off, "text: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__text ? p->__sn__text : "nil");
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
    sn_auto_str char * __sn__pattern = strdup("same");
    sn_auto_Item __sn__Item __sn__item = (__sn__Item){ .__sn__text = strdup("same") };
    long long __sn__result = ({
            long long __match_result__;
            char * __match_subject__ = "same";
            if (strcmp(__match_subject__, "miss") == 0 || strcmp(__match_subject__, __sn__item.__sn__text) == 0 || strcmp(__match_subject__, __sn__pattern) == 0) {
                __match_result__ = 1LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    printf("%lld\n", (long long)(__sn__result));
    
    fflush(stdout);
    return 0;
}
