#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: TraceIter (as val) */
typedef struct {
    long long __sn__current;
    long long __sn__remaining;
    long long __sn__has_next_calls;
    long long __sn__next_calls;
} __sn__TraceIter;
/* Value operations */
static inline __sn__TraceIter __sn__TraceIter_copy(const __sn__TraceIter *src) {
    __sn__TraceIter dst;
    dst.__sn__current = src->__sn__current;
    dst.__sn__remaining = src->__sn__remaining;
    dst.__sn__has_next_calls = src->__sn__has_next_calls;
    dst.__sn__next_calls = src->__sn__next_calls;
    return dst;
}

static inline void __sn__TraceIter_cleanup(__sn__TraceIter *p) {

}

#define sn_auto_TraceIter __attribute__((cleanup(__sn__TraceIter_cleanup)))

static inline void __sn__TraceIter_cleanup_elem(void *p) { __sn__TraceIter_cleanup((__sn__TraceIter *)p); }
static inline void __sn__TraceIter_copy_into(const void *src, void *dst) { *(__sn__TraceIter *)dst = __sn__TraceIter_copy((const __sn__TraceIter *)src); }

/* Ref/pointer operations */
static inline __sn__TraceIter *__sn__TraceIter_alloc(void) {
    return calloc(1, sizeof(__sn__TraceIter));
}

static inline void __sn__TraceIter_release(__sn__TraceIter **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_TraceIter __attribute__((cleanup(__sn__TraceIter_release)))

static inline void __sn__TraceIter_release_elem(void *p) { __sn__TraceIter_release((__sn__TraceIter **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__TraceIter_to_string(const __sn__TraceIter *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "TraceIter { ");
    off += snprintf(buf + off, sizeof(buf) - off, "current: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__current);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "has_next_calls: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__has_next_calls);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "next_calls: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__next_calls);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: TraceSource (as val) */
typedef struct {
    long long __sn__start;
    long long __sn__count;
} __sn__TraceSource;
/* Value operations */
static inline __sn__TraceSource __sn__TraceSource_copy(const __sn__TraceSource *src) {
    __sn__TraceSource dst;
    dst.__sn__start = src->__sn__start;
    dst.__sn__count = src->__sn__count;
    return dst;
}

static inline void __sn__TraceSource_cleanup(__sn__TraceSource *p) {

}

#define sn_auto_TraceSource __attribute__((cleanup(__sn__TraceSource_cleanup)))

static inline void __sn__TraceSource_cleanup_elem(void *p) { __sn__TraceSource_cleanup((__sn__TraceSource *)p); }
static inline void __sn__TraceSource_copy_into(const void *src, void *dst) { *(__sn__TraceSource *)dst = __sn__TraceSource_copy((const __sn__TraceSource *)src); }

/* Ref/pointer operations */
static inline __sn__TraceSource *__sn__TraceSource_alloc(void) {
    return calloc(1, sizeof(__sn__TraceSource));
}

static inline void __sn__TraceSource_release(__sn__TraceSource **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_TraceSource __attribute__((cleanup(__sn__TraceSource_release)))

static inline void __sn__TraceSource_release_elem(void *p) { __sn__TraceSource_release((__sn__TraceSource **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__TraceSource_to_string(const __sn__TraceSource *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "TraceSource { ");
    off += snprintf(buf + off, sizeof(buf) - off, "start: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__start);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "count: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__count);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__selectSource(long long *);
bool __sn__TraceIter_hasNext(__sn__TraceIter *);
long long __sn__TraceIter_next(__sn__TraceIter *);
__sn__TraceIter __sn__TraceSource_iter(__sn__TraceSource *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__selectSource(long long *__sn__calls) {

    (*__sn__calls = sn_add_long((*__sn__calls), 1LL));
    

    return 0LL;}


bool __sn__TraceIter_hasNext(__sn__TraceIter *__sn__self) {

    (__sn__self->__sn__has_next_calls = sn_add_long(__sn__self->__sn__has_next_calls, 1LL));
    

    return sn_lt_long(__sn__self->__sn__next_calls, __sn__self->__sn__remaining);}

long long __sn__TraceIter_next(__sn__TraceIter *__sn__self) {

    long long __sn__value = sn_add_long(sn_add_long(sn_mul_long(__sn__self->__sn__has_next_calls, 100LL), sn_mul_long(__sn__self->__sn__next_calls, 10LL)), __sn__self->__sn__current);

    (__sn__self->__sn__current = sn_add_long(__sn__self->__sn__current, 1LL));
    

    (__sn__self->__sn__next_calls = sn_add_long(__sn__self->__sn__next_calls, 1LL));
    

    return __sn__value;}


__sn__TraceIter __sn__TraceSource_iter(__sn__TraceSource *__sn__self) {

    return (__sn__TraceIter){ .__sn__current = __sn__self->__sn__start, .__sn__remaining = __sn__self->__sn__count, .__sn__has_next_calls = 0LL, .__sn__next_calls = 0LL };}

int main() {
    sn_auto_arr SnArray * __sn__sources = ({
            SnArray *__al__ = sn_array_new(sizeof(__sn__TraceSource), 1);
            __al__->elem_tag = SN_TAG_STRUCT;
    
    
            sn_array_push(__al__, &((__sn__TraceSource){ .__sn__start = 7LL, .__sn__count = 4LL }));
            __al__;
        });
    long long __sn__evaluations = 0LL;
    long long __sn__sum = 0LL;
    {
        __sn__TraceIter __sn_iter__ = __sn__TraceSource_iter(&((((__sn__TraceSource *)__sn__sources->data)[({ long long __ai__ = __sn__selectSource(&__sn__evaluations); __ai__ < 0 ? __ai__ + __sn__sources->len : __ai__; })])));
        while (__sn__TraceIter_hasNext(&__sn_iter__)) {
            long long __sn__value = __sn__TraceIter_next(&__sn_iter__);
            {
                long long __sn__produced = __sn__value;
                (__sn__value = (-1LL));
                
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("value=");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__produced));
                        sn_auto_str char *__is_p2__ = sn_strdup(" binding=");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(4, __is_p0__, __is_p1__, __is_p2__, __is_p3__);
                    }); sn_println(__ps__); };
                
                if ((__sn__produced == 218LL)) {
                    continue;
                }
                (__sn__sum = sn_add_long(__sn__sum, __sn__produced));
                
            }
        }
    }
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("natural evaluations=");
            sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__evaluations));
            sn_auto_str char *__is_p2__ = sn_strdup(" sum=");
            sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__sum));
            sn_str_concat_multi(4, __is_p0__, __is_p1__, __is_p2__, __is_p3__);
        }); sn_println(__ps__); };
    
    {
        __sn__TraceIter __sn_iter__ = __sn__TraceSource_iter(&((((__sn__TraceSource *)__sn__sources->data)[({ long long __ai__ = __sn__selectSource(&__sn__evaluations); __ai__ < 0 ? __ai__ + __sn__sources->len : __ai__; })])));
        while (__sn__TraceIter_hasNext(&__sn_iter__)) {
            long long __sn__value = __sn__TraceIter_next(&__sn_iter__);
            {
                if ((__sn__value == 329LL)) {
                    break;
                }
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("before-break=");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(2, __is_p0__, __is_p1__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("total evaluations=");
            sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__evaluations));
            sn_auto_str char *__is_p2__ = sn_strdup(" source=");
            sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)((((__sn__TraceSource *)__sn__sources->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__sources->len : __ai__; })]).__sn__start));
            sn_str_concat_multi(4, __is_p0__, __is_p1__, __is_p2__, __is_p3__);
        }); sn_println(__ps__); };
    
    long long __sn__outer = 0LL;
    {
        for (long long __sn__loop = 0LL; sn_lt_long(__sn__outer, 1LL); ({
        long long *__sn_place__ = &(__sn__outer);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    })) {
            {
                __sn__TraceIter __sn_iter__ = __sn__TraceSource_iter(&((((__sn__TraceSource *)__sn__sources->data)[({ long long __ai__ = __sn__selectSource(&__sn__evaluations); __ai__ < 0 ? __ai__ + __sn__sources->len : __ai__; })])));
                while (__sn__TraceIter_hasNext(&__sn_iter__)) {
                    long long __sn__value = __sn__TraceIter_next(&__sn_iter__);
                    {
                        if ((__sn__value == 107LL)) {
                            continue;
                        }
                    }
                }
            }
        }
    }
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("nested outer=");
            sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__outer));
            sn_auto_str char *__is_p2__ = sn_strdup(" evaluations=");
            sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__evaluations));
            sn_str_concat_multi(4, __is_p0__, __is_p1__, __is_p2__, __is_p3__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
