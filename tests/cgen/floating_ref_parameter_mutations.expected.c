#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: FloatBox (as val) */
typedef struct {
    float __sn__value;
} __sn__FloatBox;
/* Value operations */
static inline __sn__FloatBox __sn__FloatBox_copy(const __sn__FloatBox *src) {
    __sn__FloatBox dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__FloatBox_cleanup(__sn__FloatBox *p) {

}

#define sn_auto_FloatBox __attribute__((cleanup(__sn__FloatBox_cleanup)))

static inline void __sn__FloatBox_cleanup_elem(void *p) { __sn__FloatBox_cleanup((__sn__FloatBox *)p); }
static inline void __sn__FloatBox_copy_into(const void *src, void *dst) { *(__sn__FloatBox *)dst = __sn__FloatBox_copy((const __sn__FloatBox *)src); }

/* Ref/pointer operations */
static inline __sn__FloatBox *__sn__FloatBox_alloc(void) {
    return calloc(1, sizeof(__sn__FloatBox));
}

static inline void __sn__FloatBox_release(__sn__FloatBox **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_FloatBox __attribute__((cleanup(__sn__FloatBox_release)))

static inline void __sn__FloatBox_release_elem(void *p) { __sn__FloatBox_release((__sn__FloatBox **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__FloatBox_to_string(const __sn__FloatBox *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "FloatBox { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: DoubleBox (as val) */
typedef struct {
    double __sn__value;
} __sn__DoubleBox;
/* Value operations */
static inline __sn__DoubleBox __sn__DoubleBox_copy(const __sn__DoubleBox *src) {
    __sn__DoubleBox dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__DoubleBox_cleanup(__sn__DoubleBox *p) {

}

#define sn_auto_DoubleBox __attribute__((cleanup(__sn__DoubleBox_cleanup)))

static inline void __sn__DoubleBox_cleanup_elem(void *p) { __sn__DoubleBox_cleanup((__sn__DoubleBox *)p); }
static inline void __sn__DoubleBox_copy_into(const void *src, void *dst) { *(__sn__DoubleBox *)dst = __sn__DoubleBox_copy((const __sn__DoubleBox *)src); }

/* Ref/pointer operations */
static inline __sn__DoubleBox *__sn__DoubleBox_alloc(void) {
    return calloc(1, sizeof(__sn__DoubleBox));
}

static inline void __sn__DoubleBox_release(__sn__DoubleBox **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_DoubleBox __attribute__((cleanup(__sn__DoubleBox_release)))

static inline void __sn__DoubleBox_release_elem(void *p) { __sn__DoubleBox_release((__sn__DoubleBox **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__DoubleBox_to_string(const __sn__DoubleBox *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "DoubleBox { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Values (as val) */
typedef struct {
    float __sn__single;
    double __sn__precise;
    __sn__FloatBox __sn__singleBox;
    __sn__DoubleBox __sn__preciseBox;
} __sn__Values;
/* Value operations */
static inline __sn__Values __sn__Values_copy(const __sn__Values *src) {
    __sn__Values dst;
    dst.__sn__single = src->__sn__single;
    dst.__sn__precise = src->__sn__precise;
    dst.__sn__singleBox = __sn__FloatBox_copy(&src->__sn__singleBox);
    dst.__sn__preciseBox = __sn__DoubleBox_copy(&src->__sn__preciseBox);
    return dst;
}

static inline void __sn__Values_cleanup(__sn__Values *p) {

}

#define sn_auto_Values __attribute__((cleanup(__sn__Values_cleanup)))

static inline void __sn__Values_cleanup_elem(void *p) { __sn__Values_cleanup((__sn__Values *)p); }
static inline void __sn__Values_copy_into(const void *src, void *dst) { *(__sn__Values *)dst = __sn__Values_copy((const __sn__Values *)src); }

/* Ref/pointer operations */
static inline __sn__Values *__sn__Values_alloc(void) {
    return calloc(1, sizeof(__sn__Values));
}

static inline void __sn__Values_release(__sn__Values **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Values __attribute__((cleanup(__sn__Values_release)))

static inline void __sn__Values_release_elem(void *p) { __sn__Values_release((__sn__Values **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Values_to_string(const __sn__Values *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Values { ");
    off += snprintf(buf + off, sizeof(buf) - off, "single: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__single);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "precise: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__precise);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "singleBox: ");
    { char *__fs__ = __sn__FloatBox_to_string(&p->__sn__singleBox); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "preciseBox: ");
    { char *__fs__ = __sn__DoubleBox_to_string(&p->__sn__preciseBox); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: SideEffects (as val) */
typedef struct {
} __sn__SideEffects;
/* Value operations */
static inline __sn__SideEffects __sn__SideEffects_copy(const __sn__SideEffects *src) {
    __sn__SideEffects dst;
    return dst;
}

static inline void __sn__SideEffects_cleanup(__sn__SideEffects *p) {

}

#define sn_auto_SideEffects __attribute__((cleanup(__sn__SideEffects_cleanup)))

static inline void __sn__SideEffects_cleanup_elem(void *p) { __sn__SideEffects_cleanup((__sn__SideEffects *)p); }
static inline void __sn__SideEffects_copy_into(const void *src, void *dst) { *(__sn__SideEffects *)dst = __sn__SideEffects_copy((const __sn__SideEffects *)src); }

/* Ref/pointer operations */
static inline __sn__SideEffects *__sn__SideEffects_alloc(void) {
    return calloc(1, sizeof(__sn__SideEffects));
}

static inline void __sn__SideEffects_release(__sn__SideEffects **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_SideEffects __attribute__((cleanup(__sn__SideEffects_release)))

static inline void __sn__SideEffects_release_elem(void *p) { __sn__SideEffects_release((__sn__SideEffects **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__SideEffects_to_string(const __sn__SideEffects *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "SideEffects { ");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: MutationOps (as val) */
typedef struct {
} __sn__MutationOps;
/* Value operations */
static inline __sn__MutationOps __sn__MutationOps_copy(const __sn__MutationOps *src) {
    __sn__MutationOps dst;
    return dst;
}

static inline void __sn__MutationOps_cleanup(__sn__MutationOps *p) {

}

#define sn_auto_MutationOps __attribute__((cleanup(__sn__MutationOps_cleanup)))

static inline void __sn__MutationOps_cleanup_elem(void *p) { __sn__MutationOps_cleanup((__sn__MutationOps *)p); }
static inline void __sn__MutationOps_copy_into(const void *src, void *dst) { *(__sn__MutationOps *)dst = __sn__MutationOps_copy((const __sn__MutationOps *)src); }

/* Ref/pointer operations */
static inline __sn__MutationOps *__sn__MutationOps_alloc(void) {
    return calloc(1, sizeof(__sn__MutationOps));
}

static inline void __sn__MutationOps_release(__sn__MutationOps **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_MutationOps __attribute__((cleanup(__sn__MutationOps_release)))

static inline void __sn__MutationOps_release_elem(void *p) { __sn__MutationOps_release((__sn__MutationOps **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__MutationOps_to_string(const __sn__MutationOps *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "MutationOps { ");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



bool __sn__freeFloat(float *, long long *);
bool __sn__freeDouble(double *, long long *);
bool __sn__forwardFreeFloat(float *, long long *);
bool __sn__forwardFreeDouble(double *, long long *);
__sn__Values __sn__freshValues();
float __sn__SideEffects_floatRhs(long long *, float);
double __sn__SideEffects_doubleRhs(long long *, double);
bool __sn__MutationOps_staticFloat(float *, long long *);
bool __sn__MutationOps_staticDouble(double *, long long *);
bool __sn__MutationOps_instanceFloat(__sn__MutationOps *, float *, long long *);
bool __sn__MutationOps_instanceDouble(__sn__MutationOps *, double *, long long *);
bool __sn__MutationOps_forwardStaticFloat(float *, long long *);
bool __sn__MutationOps_forwardStaticDouble(double *, long long *);
bool __sn__MutationOps_forwardInstanceFloat(__sn__MutationOps *, float *, long long *);
bool __sn__MutationOps_forwardInstanceDouble(__sn__MutationOps *, double *, long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


bool __sn__freeFloat(float *__sn____sn_place, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    float __sn__afterAdd = (*__sn____sn_place) = (*__sn____sn_place) + __sn__SideEffects_floatRhs(&(*__sn__calls), 4.0f);

    (__sn__ok = ((((__sn__afterAdd == 12.0f) && ((*__sn____sn_place) == 12.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    float __sn__afterSubtract = (*__sn____sn_place) = (*__sn____sn_place) - __sn__SideEffects_floatRhs(&(*__sn__calls), 2.0f);

    (__sn__ok = ((((__sn__afterSubtract == 10.0f) && ((*__sn____sn_place) == 10.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    float __sn__afterMultiply = (*__sn____sn_place) = (*__sn____sn_place) * __sn__SideEffects_floatRhs(&(*__sn__calls), 3.0f);

    (__sn__ok = ((((__sn__afterMultiply == 30.0f) && ((*__sn____sn_place) == 30.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    float __sn__afterDivide = (*__sn____sn_place) = (*__sn____sn_place) / __sn__SideEffects_floatRhs(&(*__sn__calls), 5.0f);

    (__sn__ok = ((((__sn__afterDivide == 6.0f) && ((*__sn____sn_place) == 6.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    float __sn__beforeIncrement = (*__sn____sn_place)++;

    (__sn__ok = (((__sn__beforeIncrement == 6.0f) && ((*__sn____sn_place) == 7.0f)) && __sn__ok));
    

    float __sn__beforeDecrement = (*__sn____sn_place)--;

    (__sn__ok = (((__sn__beforeDecrement == 7.0f) && ((*__sn____sn_place) == 6.0f)) && __sn__ok));
    

    return __sn__ok;}


bool __sn__freeDouble(double *__sn____sn_rhs, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    double __sn__afterAdd = (*__sn____sn_rhs) = (*__sn____sn_rhs) + __sn__SideEffects_doubleRhs(&(*__sn__calls), 8.0);

    (__sn__ok = ((((__sn__afterAdd == 24.0) && ((*__sn____sn_rhs) == 24.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    double __sn__afterSubtract = (*__sn____sn_rhs) = (*__sn____sn_rhs) - __sn__SideEffects_doubleRhs(&(*__sn__calls), 4.0);

    (__sn__ok = ((((__sn__afterSubtract == 20.0) && ((*__sn____sn_rhs) == 20.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    double __sn__afterMultiply = (*__sn____sn_rhs) = (*__sn____sn_rhs) * __sn__SideEffects_doubleRhs(&(*__sn__calls), 2.0);

    (__sn__ok = ((((__sn__afterMultiply == 40.0) && ((*__sn____sn_rhs) == 40.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    double __sn__afterDivide = (*__sn____sn_rhs) = (*__sn____sn_rhs) / __sn__SideEffects_doubleRhs(&(*__sn__calls), 5.0);

    (__sn__ok = ((((__sn__afterDivide == 8.0) && ((*__sn____sn_rhs) == 8.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    double __sn__beforeIncrement = (*__sn____sn_rhs)++;

    (__sn__ok = (((__sn__beforeIncrement == 8.0) && ((*__sn____sn_rhs) == 9.0)) && __sn__ok));
    

    double __sn__beforeDecrement = (*__sn____sn_rhs)--;

    (__sn__ok = (((__sn__beforeDecrement == 9.0) && ((*__sn____sn_rhs) == 8.0)) && __sn__ok));
    

    return __sn__ok;}


bool __sn__forwardFreeFloat(float *__sn__value, long long *__sn__calls) {

    return __sn__freeFloat(&(*__sn__value), &(*__sn__calls));}


bool __sn__forwardFreeDouble(double *__sn__value, long long *__sn__calls) {

    return __sn__freeDouble(&(*__sn__value), &(*__sn__calls));}


__sn__Values __sn__freshValues() {

    return (__sn__Values){ .__sn__single = 8.0f, .__sn__precise = 16.0, .__sn__singleBox = (__sn__FloatBox){ .__sn__value = 8.0f }, .__sn__preciseBox = (__sn__DoubleBox){ .__sn__value = 16.0 } };}





float __sn__SideEffects_floatRhs(long long *__sn__calls, float __sn__result) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__result;}

double __sn__SideEffects_doubleRhs(long long *__sn__calls, double __sn__result) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__result;}


bool __sn__MutationOps_staticFloat(float *__sn____sn_next, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    float __sn__afterAdd = (*__sn____sn_next) = (*__sn____sn_next) + __sn__SideEffects_floatRhs(&(*__sn__calls), 4.0f);

    (__sn__ok = ((((__sn__afterAdd == 12.0f) && ((*__sn____sn_next) == 12.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    float __sn__afterSubtract = (*__sn____sn_next) = (*__sn____sn_next) - __sn__SideEffects_floatRhs(&(*__sn__calls), 2.0f);

    (__sn__ok = ((((__sn__afterSubtract == 10.0f) && ((*__sn____sn_next) == 10.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    float __sn__afterMultiply = (*__sn____sn_next) = (*__sn____sn_next) * __sn__SideEffects_floatRhs(&(*__sn__calls), 3.0f);

    (__sn__ok = ((((__sn__afterMultiply == 30.0f) && ((*__sn____sn_next) == 30.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    float __sn__afterDivide = (*__sn____sn_next) = (*__sn____sn_next) / __sn__SideEffects_floatRhs(&(*__sn__calls), 5.0f);

    (__sn__ok = ((((__sn__afterDivide == 6.0f) && ((*__sn____sn_next) == 6.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    float __sn__beforeIncrement = (*__sn____sn_next)++;

    (__sn__ok = (((__sn__beforeIncrement == 6.0f) && ((*__sn____sn_next) == 7.0f)) && __sn__ok));
    

    float __sn__beforeDecrement = (*__sn____sn_next)--;

    (__sn__ok = (((__sn__beforeDecrement == 7.0f) && ((*__sn____sn_next) == 6.0f)) && __sn__ok));
    

    return __sn__ok;}

bool __sn__MutationOps_staticDouble(double *__sn____sn_previous, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    double __sn__afterAdd = (*__sn____sn_previous) = (*__sn____sn_previous) + __sn__SideEffects_doubleRhs(&(*__sn__calls), 8.0);

    (__sn__ok = ((((__sn__afterAdd == 24.0) && ((*__sn____sn_previous) == 24.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    double __sn__afterSubtract = (*__sn____sn_previous) = (*__sn____sn_previous) - __sn__SideEffects_doubleRhs(&(*__sn__calls), 4.0);

    (__sn__ok = ((((__sn__afterSubtract == 20.0) && ((*__sn____sn_previous) == 20.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    double __sn__afterMultiply = (*__sn____sn_previous) = (*__sn____sn_previous) * __sn__SideEffects_doubleRhs(&(*__sn__calls), 2.0);

    (__sn__ok = ((((__sn__afterMultiply == 40.0) && ((*__sn____sn_previous) == 40.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    double __sn__afterDivide = (*__sn____sn_previous) = (*__sn____sn_previous) / __sn__SideEffects_doubleRhs(&(*__sn__calls), 5.0);

    (__sn__ok = ((((__sn__afterDivide == 8.0) && ((*__sn____sn_previous) == 8.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    double __sn__beforeIncrement = (*__sn____sn_previous)++;

    (__sn__ok = (((__sn__beforeIncrement == 8.0) && ((*__sn____sn_previous) == 9.0)) && __sn__ok));
    

    double __sn__beforeDecrement = (*__sn____sn_previous)--;

    (__sn__ok = (((__sn__beforeDecrement == 9.0) && ((*__sn____sn_previous) == 8.0)) && __sn__ok));
    

    return __sn__ok;}

bool __sn__MutationOps_instanceFloat(__sn__MutationOps *__sn__self, float *__sn__value, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    float __sn__afterAdd = (*__sn__value) = (*__sn__value) + __sn__SideEffects_floatRhs(&(*__sn__calls), 4.0f);

    (__sn__ok = ((((__sn__afterAdd == 12.0f) && ((*__sn__value) == 12.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    float __sn__afterSubtract = (*__sn__value) = (*__sn__value) - __sn__SideEffects_floatRhs(&(*__sn__calls), 2.0f);

    (__sn__ok = ((((__sn__afterSubtract == 10.0f) && ((*__sn__value) == 10.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    float __sn__afterMultiply = (*__sn__value) = (*__sn__value) * __sn__SideEffects_floatRhs(&(*__sn__calls), 3.0f);

    (__sn__ok = ((((__sn__afterMultiply == 30.0f) && ((*__sn__value) == 30.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    float __sn__afterDivide = (*__sn__value) = (*__sn__value) / __sn__SideEffects_floatRhs(&(*__sn__calls), 5.0f);

    (__sn__ok = ((((__sn__afterDivide == 6.0f) && ((*__sn__value) == 6.0f)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    float __sn__beforeIncrement = (*__sn__value)++;

    (__sn__ok = (((__sn__beforeIncrement == 6.0f) && ((*__sn__value) == 7.0f)) && __sn__ok));
    

    float __sn__beforeDecrement = (*__sn__value)--;

    (__sn__ok = (((__sn__beforeDecrement == 7.0f) && ((*__sn__value) == 6.0f)) && __sn__ok));
    

    return __sn__ok;}

bool __sn__MutationOps_instanceDouble(__sn__MutationOps *__sn__self, double *__sn__value, long long *__sn__calls) {

    long long __sn__beforeCalls = (*__sn__calls);

    bool __sn__ok = true;

    double __sn__afterAdd = (*__sn__value) = (*__sn__value) + __sn__SideEffects_doubleRhs(&(*__sn__calls), 8.0);

    (__sn__ok = ((((__sn__afterAdd == 24.0) && ((*__sn__value) == 24.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 1LL))) && __sn__ok));
    

    double __sn__afterSubtract = (*__sn__value) = (*__sn__value) - __sn__SideEffects_doubleRhs(&(*__sn__calls), 4.0);

    (__sn__ok = ((((__sn__afterSubtract == 20.0) && ((*__sn__value) == 20.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 2LL))) && __sn__ok));
    

    double __sn__afterMultiply = (*__sn__value) = (*__sn__value) * __sn__SideEffects_doubleRhs(&(*__sn__calls), 2.0);

    (__sn__ok = ((((__sn__afterMultiply == 40.0) && ((*__sn__value) == 40.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 3LL))) && __sn__ok));
    

    double __sn__afterDivide = (*__sn__value) = (*__sn__value) / __sn__SideEffects_doubleRhs(&(*__sn__calls), 5.0);

    (__sn__ok = ((((__sn__afterDivide == 8.0) && ((*__sn__value) == 8.0)) && ((*__sn__calls) == sn_add_long(__sn__beforeCalls, 4LL))) && __sn__ok));
    

    double __sn__beforeIncrement = (*__sn__value)++;

    (__sn__ok = (((__sn__beforeIncrement == 8.0) && ((*__sn__value) == 9.0)) && __sn__ok));
    

    double __sn__beforeDecrement = (*__sn__value)--;

    (__sn__ok = (((__sn__beforeDecrement == 9.0) && ((*__sn__value) == 8.0)) && __sn__ok));
    

    return __sn__ok;}

bool __sn__MutationOps_forwardStaticFloat(float *__sn__value, long long *__sn__calls) {

    return __sn__MutationOps_staticFloat(&(*__sn__value), &(*__sn__calls));}

bool __sn__MutationOps_forwardStaticDouble(double *__sn__value, long long *__sn__calls) {

    return __sn__MutationOps_staticDouble(&(*__sn__value), &(*__sn__calls));}

bool __sn__MutationOps_forwardInstanceFloat(__sn__MutationOps *__sn__self, float *__sn__value, long long *__sn__calls) {

    return __sn__MutationOps_instanceFloat(__sn__self, &(*__sn__value), &(*__sn__calls));}

bool __sn__MutationOps_forwardInstanceDouble(__sn__MutationOps *__sn__self, double *__sn__value, long long *__sn__calls) {

    return __sn__MutationOps_instanceDouble(__sn__self, &(*__sn__value), &(*__sn__calls));}

int main() {
    long long __sn__calls = 0LL;
    float __sn__freeSingle = 8.0f;
    __sn__Values __sn__direct = __sn__freshValues();
    double __sn__staticPrecise = 16.0;
    float __sn__instanceSingle = 8.0f;
    __sn__MutationOps __sn__ops = (__sn__MutationOps){  };
    bool __sn__freeFloatOk = __sn__freeFloat(&__sn__freeSingle, &__sn__calls);
    printf("%s\n", (((__sn__freeFloatOk && (__sn__freeSingle == 6.0f)) && (__sn__calls == 4LL))) ? "true" : "false");
    
    bool __sn__freeDoubleOk = __sn__freeDouble(&__sn__direct.__sn__precise, &__sn__calls);
    printf("%s\n", (((__sn__freeDoubleOk && (__sn__direct.__sn__precise == 8.0)) && (__sn__calls == 8LL))) ? "true" : "false");
    
    bool __sn__staticFloatOk = __sn__MutationOps_staticFloat(&__sn__direct.__sn__singleBox.__sn__value, &__sn__calls);
    printf("%s\n", (((__sn__staticFloatOk && (__sn__direct.__sn__singleBox.__sn__value == 6.0f)) && (__sn__calls == 12LL))) ? "true" : "false");
    
    bool __sn__staticDoubleOk = __sn__MutationOps_staticDouble(&__sn__staticPrecise, &__sn__calls);
    printf("%s\n", (((__sn__staticDoubleOk && (__sn__staticPrecise == 8.0)) && (__sn__calls == 16LL))) ? "true" : "false");
    
    bool __sn__instanceFloatOk = __sn__MutationOps_instanceFloat(&__sn__ops, &__sn__instanceSingle, &__sn__calls);
    printf("%s\n", (((__sn__instanceFloatOk && (__sn__instanceSingle == 6.0f)) && (__sn__calls == 20LL))) ? "true" : "false");
    
    bool __sn__instanceDoubleOk = __sn__MutationOps_instanceDouble(&__sn__ops, &__sn__direct.__sn__preciseBox.__sn__value, &__sn__calls);
    printf("%s\n", (((__sn__instanceDoubleOk && (__sn__direct.__sn__preciseBox.__sn__value == 8.0)) && (__sn__calls == 24LL))) ? "true" : "false");
    
    __sn__Values __sn__forwarded = __sn__freshValues();
    float __sn__forwardedFloat = 8.0f;
    double __sn__forwardedDouble = 16.0;
    bool __sn__forwardFreeFloatOk = __sn__forwardFreeFloat(&__sn__forwarded.__sn__single, &__sn__calls);
    printf("%s\n", (((__sn__forwardFreeFloatOk && (__sn__forwarded.__sn__single == 6.0f)) && (__sn__calls == 28LL))) ? "true" : "false");
    
    bool __sn__forwardFreeDoubleOk = __sn__forwardFreeDouble(&__sn__forwardedDouble, &__sn__calls);
    printf("%s\n", (((__sn__forwardFreeDoubleOk && (__sn__forwardedDouble == 8.0)) && (__sn__calls == 32LL))) ? "true" : "false");
    
    bool __sn__forwardStaticFloatOk = __sn__MutationOps_forwardStaticFloat(&__sn__forwardedFloat, &__sn__calls);
    printf("%s\n", (((__sn__forwardStaticFloatOk && (__sn__forwardedFloat == 6.0f)) && (__sn__calls == 36LL))) ? "true" : "false");
    
    bool __sn__forwardStaticDoubleOk = __sn__MutationOps_forwardStaticDouble(&__sn__forwarded.__sn__preciseBox.__sn__value, &__sn__calls);
    printf("%s\n", (((__sn__forwardStaticDoubleOk && (__sn__forwarded.__sn__preciseBox.__sn__value == 8.0)) && (__sn__calls == 40LL))) ? "true" : "false");
    
    bool __sn__forwardInstanceFloatOk = __sn__MutationOps_forwardInstanceFloat(&__sn__ops, &__sn__forwarded.__sn__singleBox.__sn__value, &__sn__calls);
    printf("%s\n", (((__sn__forwardInstanceFloatOk && (__sn__forwarded.__sn__singleBox.__sn__value == 6.0f)) && (__sn__calls == 44LL))) ? "true" : "false");
    
    bool __sn__forwardInstanceDoubleOk = __sn__MutationOps_forwardInstanceDouble(&__sn__ops, &__sn__forwarded.__sn__precise, &__sn__calls);
    printf("%s\n", (((__sn__forwardInstanceDoubleOk && (__sn__forwarded.__sn__precise == 8.0)) && (__sn__calls == 48LL))) ? "true" : "false");
    
    fflush(stdout);
    return 0;
}
