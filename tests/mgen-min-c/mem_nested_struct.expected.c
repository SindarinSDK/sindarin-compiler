#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Address (as ref — refcounted) */
typedef struct {
    int __rc__;
    char * __sn__city;
} __sn__Address;

static inline __sn__Address *__sn__Address_create(void) {
    __sn__Address *p = calloc(1, sizeof(__sn__Address));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Address *__sn__Address_retain(__sn__Address *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Address_release(__sn__Address **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free((*p)->__sn__city);
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Address *__sn__Address_copy(const __sn__Address *src) {
    __sn__Address *dst = calloc(1, sizeof(__sn__Address));
    dst->__rc__ = 1;
    dst->__sn__city = src->__sn__city ? strdup(src->__sn__city) : NULL;
    return dst;
}

#define sn_auto_Address __attribute__((cleanup(__sn__Address_release)))

static inline void __sn__Address_release_elem(void *p) { __sn__Address_release((__sn__Address **)p); }
static inline void __sn__Address_retain_into(const void *src, void *dst) { *(__sn__Address **)dst = __sn__Address_retain(*(__sn__Address *const *)src); }



/* Struct: Person (as ref — refcounted) */
typedef struct {
    int __rc__;
    char * __sn__name;
    __sn__Address * __sn__addr;
} __sn__Person;

static inline __sn__Person *__sn__Person_create(void) {
    __sn__Person *p = calloc(1, sizeof(__sn__Person));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Person *__sn__Person_retain(__sn__Person *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Person_release(__sn__Person **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free((*p)->__sn__name);
        __sn__Address_release(&(*p)->__sn__addr);
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Person *__sn__Person_copy(const __sn__Person *src) {
    __sn__Person *dst = calloc(1, sizeof(__sn__Person));
    dst->__rc__ = 1;
    dst->__sn__name = src->__sn__name ? strdup(src->__sn__name) : NULL;
    dst->__sn__addr = __sn__Address_retain(src->__sn__addr);
    return dst;
}

#define sn_auto_Person __attribute__((cleanup(__sn__Person_release)))

static inline void __sn__Person_release_elem(void *p) { __sn__Person_release((__sn__Person **)p); }
static inline void __sn__Person_retain_into(const void *src, void *dst) { *(__sn__Person **)dst = __sn__Person_retain(*(__sn__Person *const *)src); }



int main() {
    sn_auto_Address __sn__Address * __sn__a = ({
        __sn__Address *__tmp__ = __sn__Address_create();
        __tmp__->__sn__city = strdup("NYC");
        __tmp__;
    });
    sn_auto_Person __sn__Person * __sn__p = ({
        __sn__Person *__tmp__ = __sn__Person_create();
        __tmp__->__sn__name = strdup("Alice");
        __tmp__->__sn__addr = __sn__Address_retain(__sn__a);
        __tmp__;
    });
    sn_assert((sn_str_length(__sn__p->__sn__name) == 5LL), "name should be Alice");
    return 0;
}
