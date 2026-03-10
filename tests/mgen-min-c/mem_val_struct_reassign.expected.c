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


int main() {
    sn_auto_Person __sn__Person __sn__a = (__sn__Person){ .__sn__name = strdup("Alice"), .__sn__age = 30LL };
    sn_auto_Person __sn__Person __sn__b = (__sn__Person){ .__sn__name = strdup("Bob"), .__sn__age = 25LL };
    ({
        __sn__Person_cleanup(&__sn__a);
        __sn__a = __sn__Person_copy(&__sn__b);
        __sn__a;
    });
    (__sn__a.__sn__age = 99LL);
    sn_assert((__sn__b.__sn__age == 25LL), "original b should be unchanged");
    sn_assert((__sn__a.__sn__age == 99LL), "a should have new age");
    return 0;
}
