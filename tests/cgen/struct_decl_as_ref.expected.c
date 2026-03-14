#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Vec2 (native) */
typedef struct {
    double x;
    double y;
} __sn__Vec2;

static inline __sn__Vec2 *__sn__Vec2_alloc(void) {
    return calloc(1, sizeof(__sn__Vec2));
}

static inline void __sn__Vec2_release(__sn__Vec2 **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_Vec2 __attribute__((cleanup(__sn__Vec2_release)))
#define sn_auto_ref_Vec2 __attribute__((cleanup(__sn__Vec2_release)))


typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    return 0LL;}
