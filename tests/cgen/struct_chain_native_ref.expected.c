#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Stmt (native) */
typedef struct {
    long long __sn___ptr;
} __sn__Stmt;

static inline __sn__Stmt *__sn__Stmt_alloc(void) {
    return calloc(1, sizeof(__sn__Stmt));
}

static inline void __sn__Stmt_release(__sn__Stmt **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_Stmt __attribute__((cleanup(__sn__Stmt_release)))
#define sn_auto_ref_Stmt __attribute__((cleanup(__sn__Stmt_release)))


__sn__Stmt * __sn__Stmt_bind(__sn__Stmt *, long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


__sn__Stmt * __sn__Stmt_bind(__sn__Stmt *__sn__self, long long __sn__v) {

    return __sn__self;}

int main() {
    sn_auto_Stmt __sn__Stmt * __sn__s = NULL;
    __sn__Stmt * __sn____chain_tmp_0 = __sn__Stmt_bind(__sn__s, 1LL);
    __sn__Stmt_bind(__sn____chain_tmp_0, 2LL);
    
    fflush(stdout);
    return 0;
}
