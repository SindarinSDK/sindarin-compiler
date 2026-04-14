#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Config (as val) */
typedef struct {
    char * __sn__name;
    long long __sn__value;
} __sn__Config;
/* Value operations */
static inline __sn__Config __sn__Config_copy(const __sn__Config *src) {
    __sn__Config dst;
    dst.__sn__name = src->__sn__name ? strdup(src->__sn__name) : NULL;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__Config_cleanup(__sn__Config *p) {
    free(p->__sn__name);

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
        free((*p)->__sn__name);
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
    off += snprintf(buf + off, sizeof(buf) - off, "name: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__name ? p->__sn__name : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__processConfig(__sn__Config *);

typedef struct {
    __sn__Config arg0;
    int _padding;
} __ThreadArgs_0__;

static void *__thread_wrapper_0__(void *arg) {
    SnThread *__th__ = (SnThread *)arg;
    __ThreadArgs_0__ *args = (__ThreadArgs_0__ *)__th__->result;
    __ThreadArgs_0__ __args_copy__ = *args;
    args = &__args_copy__;

    long long __result__ = __sn__processConfig(&args->arg0);
    __sn__Config_cleanup(&args->arg0);
    free(__th__->result); __th__->result = NULL;
    if (!__th__->result) __th__->result = calloc(1, sizeof(long long));
    *(long long *)__th__->result = __result__;
    sn_thread_release(__th__);
    return NULL;
}
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


long long __sn__processConfig(__sn__Config *__sn__config) {

    return (*__sn__config).__sn__value;}


int main() {
    sn_auto_Config __sn__Config __sn__c = (__sn__Config){ .__sn__name = strdup("test"), .__sn__value = 42LL };
    long long __sn__handle = 0; sn_auto_thread SnThread * __sn__handle__th__ = ({
        SnThread *__th__ = sn_thread_create();
    
        __ThreadArgs_0__ *__args__ = malloc(sizeof(__ThreadArgs_0__));
        __args__->arg0 = __sn__Config_copy(&(__sn__c));
        __th__->result = __args__;
        __th__->result_size = sizeof(long long);
        pthread_create(&__th__->thread, NULL, __thread_wrapper_0__, __th__);
        __th__;
    });
    long long __sn__result = ({
        sn_auto_thread SnThread *__sync_th__ = __sn__handle__th__; __sn__handle__th__ = NULL;
        if (__sync_th__) { sn_thread_join(__sync_th__); __sn__handle = *(long long *)__sync_th__->result; }
        __sn__handle; })
    ;
    sn_assert((__sn__result == 42LL), "expected 42");
    
    fflush(stdout);
    return 0;
}
