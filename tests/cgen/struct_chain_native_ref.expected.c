#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Stmt (native, as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn___ptr;
} __sn__Stmt;



static inline __sn__Stmt *__sn__Stmt__new(void) {
    __sn__Stmt *p = calloc(1, sizeof(__sn__Stmt));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Stmt *__sn__Stmt_retain(__sn__Stmt *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Stmt_release(__sn__Stmt **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}


#define sn_auto_Stmt __attribute__((cleanup(__sn__Stmt_release)))
#define sn_auto_ref_Stmt __attribute__((cleanup(__sn__Stmt_release)))

static inline void __sn__Stmt_release_elem(void *p) { __sn__Stmt_release((__sn__Stmt **)p); }
static inline void __sn__Stmt_retain_into(const void *src, void *dst) { *(__sn__Stmt **)dst = __sn__Stmt_retain(*(__sn__Stmt *const *)src); }


__sn__Stmt * __sn__Stmt_bind(__sn__Stmt *, long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


__sn__Stmt * __sn__Stmt_bind(__sn__Stmt *__sn__self, long long __sn__v) {

    return __sn__Stmt_retain(__sn__self);}

int main() {
    sn_auto_Stmt __sn__Stmt * __sn__s = NULL;
    sn_auto_Stmt __sn__Stmt * __sn____chain_tmp_0 = __sn__Stmt_bind(__sn__s, 1LL);
    { __sn__Stmt *__discard__ = __sn__Stmt_bind(__sn____chain_tmp_0, 2LL); __sn__Stmt_release(&__discard__); }
    
    fflush(stdout);
    return 0;
}
