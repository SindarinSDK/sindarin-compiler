#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Effects (as val) */
typedef struct {
} __sn__Effects;
/* Value operations */
static inline __sn__Effects __sn__Effects_copy(const __sn__Effects *src) {
    __sn__Effects dst;
    return dst;
}

static inline void __sn__Effects_cleanup(__sn__Effects *p) {

}

#define sn_auto_Effects __attribute__((cleanup(__sn__Effects_cleanup)))

static inline void __sn__Effects_cleanup_elem(void *p) { __sn__Effects_cleanup((__sn__Effects *)p); }
static inline void __sn__Effects_copy_into(const void *src, void *dst) { *(__sn__Effects *)dst = __sn__Effects_copy((const __sn__Effects *)src); }

/* Ref/pointer operations */
static inline __sn__Effects *__sn__Effects_alloc(void) {
    return calloc(1, sizeof(__sn__Effects));
}

static inline void __sn__Effects_release(__sn__Effects **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Effects __attribute__((cleanup(__sn__Effects_release)))

static inline void __sn__Effects_release_elem(void *p) { __sn__Effects_release((__sn__Effects **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Effects_to_string(const __sn__Effects *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Effects { ");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: FloatingOps (as val) */
typedef struct {
    long long __sn__marker;
} __sn__FloatingOps;
/* Value operations */
static inline __sn__FloatingOps __sn__FloatingOps_copy(const __sn__FloatingOps *src) {
    __sn__FloatingOps dst;
    dst.__sn__marker = src->__sn__marker;
    return dst;
}

static inline void __sn__FloatingOps_cleanup(__sn__FloatingOps *p) {

}

#define sn_auto_FloatingOps __attribute__((cleanup(__sn__FloatingOps_cleanup)))

static inline void __sn__FloatingOps_cleanup_elem(void *p) { __sn__FloatingOps_cleanup((__sn__FloatingOps *)p); }
static inline void __sn__FloatingOps_copy_into(const void *src, void *dst) { *(__sn__FloatingOps *)dst = __sn__FloatingOps_copy((const __sn__FloatingOps *)src); }

/* Ref/pointer operations */
static inline __sn__FloatingOps *__sn__FloatingOps_alloc(void) {
    return calloc(1, sizeof(__sn__FloatingOps));
}

static inline void __sn__FloatingOps_release(__sn__FloatingOps **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_FloatingOps __attribute__((cleanup(__sn__FloatingOps_release)))

static inline void __sn__FloatingOps_release_elem(void *p) { __sn__FloatingOps_release((__sn__FloatingOps **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__FloatingOps_to_string(const __sn__FloatingOps *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "FloatingOps { ");
    off += snprintf(buf + off, sizeof(buf) - off, "marker: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__marker);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



bool __sn__freeFloat(float, long long *, float);
bool __sn__freeDouble(double, long long *, double);
bool __sn__floatSpecial(float, float, float);
bool __sn__doubleSpecial(double, double, double);
bool __sn__helperNames(float, float, float, float, float);
double __sn__shadowOrder(double);
float __sn__Effects_floatRhs(long long *, float);
double __sn__Effects_doubleRhs(long long *, double);
bool __sn__FloatingOps_staticFloat(float, long long *, float);
bool __sn__FloatingOps_staticDouble(double, long long *, double);
bool __sn__FloatingOps_instanceFloat(__sn__FloatingOps *, float, long long *, float);
bool __sn__FloatingOps_instanceDouble(__sn__FloatingOps *, double, long long *, double);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


bool __sn__freeFloat(float __sn__value, long long *__sn__calls, float __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    float __sn__added = __sn__value = __sn__value + __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 2.0f));

    float __sn__subtracted = __sn__value = __sn__value - __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__multiplied = __sn__value = __sn__value * __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value - 7.0f));

    float __sn__divided = __sn__value = __sn__value / __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__beforeIncrement = __sn__value++;

    float __sn__beforeDecrement = __sn__value--;

    return (((((((((__sn__added == 12.0f) && (__sn__subtracted == 10.0f)) && (__sn__multiplied == 30.0f)) && (__sn__divided == 6.0f)) && (__sn__beforeIncrement == 6.0f)) && (__sn__beforeDecrement == 7.0f)) && (__sn__value == 6.0f)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0f));}


bool __sn__freeDouble(double __sn__value, long long *__sn__calls, double __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    double __sn__added = __sn__value = __sn__value + __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 2.0));

    double __sn__subtracted = __sn__value = __sn__value - __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 6.0));

    double __sn__multiplied = __sn__value = __sn__value * __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 10.0));

    double __sn__divided = __sn__value = __sn__value / __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 8.0));

    double __sn__beforeIncrement = __sn__value++;

    double __sn__beforeDecrement = __sn__value--;

    return (((((((((__sn__added == 24.0) && (__sn__subtracted == 20.0)) && (__sn__multiplied == 40.0)) && (__sn__divided == 8.0)) && (__sn__beforeIncrement == 8.0)) && (__sn__beforeDecrement == 9.0)) && (__sn__value == 8.0)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0));}


bool __sn__floatSpecial(float __sn__positive, float __sn__zero, float __sn__negativeZero) {

    float __sn__infinity = __sn__positive = __sn__positive / 0.0f;

    float __sn__nan = __sn__zero = __sn__zero / __sn__zero;

    float __sn__signed = __sn__negativeZero = __sn__negativeZero * 1.0f;

    return ((((__sn__infinity > 0.0f) && (__sn__nan != __sn__nan)) && (__sn__signed == 0.0f)) && ((1.0f / __sn__signed) < 0.0f));}


bool __sn__doubleSpecial(double __sn__positive, double __sn__zero, double __sn__negativeZero) {

    double __sn__infinity = __sn__positive = __sn__positive / 0.0;

    double __sn__nan = __sn__zero = __sn__zero / __sn__zero;

    double __sn__signed = __sn__negativeZero = __sn__negativeZero * 1.0;

    return ((((__sn__infinity > 0.0) && (__sn__nan != __sn__nan)) && (__sn__signed == 0.0)) && ((1.0 / __sn__signed) < 0.0));}


bool __sn__helperNames(float __sn____sn_rhs, float __sn____sn_place, float __sn____sn_next, float __sn____sn_previous, float __sn__untouched) {

    float __sn__added = __sn____sn_rhs = __sn____sn_rhs + __sn____sn_rhs;

    float __sn__incremented = __sn____sn_place++;

    float __sn__multiplied = __sn____sn_next = __sn____sn_next * 2.0f;

    float __sn__decremented = __sn____sn_previous--;

    return (((((((((__sn__added == 4.0f) && (__sn____sn_rhs == 4.0f)) && (__sn__incremented == 3.0f)) && (__sn____sn_place == 4.0f)) && (__sn__multiplied == 8.0f)) && (__sn____sn_next == 8.0f)) && (__sn__decremented == 5.0f)) && (__sn____sn_previous == 4.0f)) && (__sn__untouched == 6.0f));}


double __sn__shadowOrder(double __sn__value) {

    if (true) {
        __sn__value = __sn__value + 1.0;
        
        double __sn__beforeShadow = __sn__value;
        double __sn__value = __sn__beforeShadow;
        __sn__value++;
        
    }

    return __sn__value;}


float __sn__Effects_floatRhs(long long *__sn__calls, float __sn__value) {

    (*__sn__calls)++;
    

    return __sn__value;}

double __sn__Effects_doubleRhs(long long *__sn__calls, double __sn__value) {

    (*__sn__calls)++;
    

    return __sn__value;}


bool __sn__FloatingOps_staticFloat(float __sn__value, long long *__sn__calls, float __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    float __sn__added = __sn__value = __sn__value + __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 2.0f));

    float __sn__subtracted = __sn__value = __sn__value - __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__multiplied = __sn__value = __sn__value * __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value - 7.0f));

    float __sn__divided = __sn__value = __sn__value / __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__beforeIncrement = __sn__value++;

    float __sn__beforeDecrement = __sn__value--;

    return (((((((((__sn__added == 12.0f) && (__sn__subtracted == 10.0f)) && (__sn__multiplied == 30.0f)) && (__sn__divided == 6.0f)) && (__sn__beforeIncrement == 6.0f)) && (__sn__beforeDecrement == 7.0f)) && (__sn__value == 6.0f)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0f));}

bool __sn__FloatingOps_staticDouble(double __sn__value, long long *__sn__calls, double __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    double __sn__added = __sn__value = __sn__value + __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 2.0));

    double __sn__subtracted = __sn__value = __sn__value - __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 6.0));

    double __sn__multiplied = __sn__value = __sn__value * __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 10.0));

    double __sn__divided = __sn__value = __sn__value / __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 8.0));

    double __sn__beforeIncrement = __sn__value++;

    double __sn__beforeDecrement = __sn__value--;

    return (((((((((__sn__added == 24.0) && (__sn__subtracted == 20.0)) && (__sn__multiplied == 40.0)) && (__sn__divided == 8.0)) && (__sn__beforeIncrement == 8.0)) && (__sn__beforeDecrement == 9.0)) && (__sn__value == 8.0)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0));}

bool __sn__FloatingOps_instanceFloat(__sn__FloatingOps *__sn__self, float __sn__value, long long *__sn__calls, float __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    float __sn__added = __sn__value = __sn__value + __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 2.0f));

    float __sn__subtracted = __sn__value = __sn__value - __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__multiplied = __sn__value = __sn__value * __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value - 7.0f));

    float __sn__divided = __sn__value = __sn__value / __sn__Effects_floatRhs(&(*__sn__calls), (__sn__value / 6.0f));

    float __sn__beforeIncrement = __sn__value++;

    float __sn__beforeDecrement = __sn__value--;

    return ((((((((((__sn__self->__sn__marker == 1LL) && (__sn__added == 12.0f)) && (__sn__subtracted == 10.0f)) && (__sn__multiplied == 30.0f)) && (__sn__divided == 6.0f)) && (__sn__beforeIncrement == 6.0f)) && (__sn__beforeDecrement == 7.0f)) && (__sn__value == 6.0f)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0f));}

bool __sn__FloatingOps_instanceDouble(__sn__FloatingOps *__sn__self, double __sn__value, long long *__sn__calls, double __sn__untouched) {

    long long __sn__beforeCalls = (*__sn__calls);

    double __sn__added = __sn__value = __sn__value + __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 2.0));

    double __sn__subtracted = __sn__value = __sn__value - __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 6.0));

    double __sn__multiplied = __sn__value = __sn__value * __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 10.0));

    double __sn__divided = __sn__value = __sn__value / __sn__Effects_doubleRhs(&(*__sn__calls), (__sn__value / 8.0));

    double __sn__beforeIncrement = __sn__value++;

    double __sn__beforeDecrement = __sn__value--;

    return ((((((((((__sn__self->__sn__marker == 1LL) && (__sn__added == 24.0)) && (__sn__subtracted == 20.0)) && (__sn__multiplied == 40.0)) && (__sn__divided == 8.0)) && (__sn__beforeIncrement == 8.0)) && (__sn__beforeDecrement == 9.0)) && (__sn__value == 8.0)) && ((*__sn__calls) == (__sn__beforeCalls + 4LL))) && (__sn__untouched == 99.0));}

int main() {
    long long __sn__calls = 0LL;
    float __sn__freeFloatCaller = 8.0f;
    double __sn__freeDoubleCaller = 16.0;
    float __sn__staticFloatCaller = 8.0f;
    double __sn__staticDoubleCaller = 16.0;
    float __sn__instanceFloatCaller = 8.0f;
    double __sn__instanceDoubleCaller = 16.0;
    float __sn__specialFloatOne = 1.0f;
    float __sn__specialFloatZero = 0.0f;
    float __sn__specialFloatNegativeZero = (-0.0f);
    double __sn__specialDoubleOne = 1.0;
    double __sn__specialDoubleZero = 0.0;
    double __sn__specialDoubleNegativeZero = (-0.0);
    double __sn__orderCaller = 4.0;
    __sn__FloatingOps __sn__ops = (__sn__FloatingOps){ .__sn__marker = 1LL };
    printf("%s\n", (__sn__freeFloat(__sn__freeFloatCaller, &__sn__calls, 99.0f)) ? "true" : "false");
    
    printf("%s\n", (__sn__freeDouble(__sn__freeDoubleCaller, &__sn__calls, 99.0)) ? "true" : "false");
    
    printf("%s\n", (__sn__FloatingOps_staticFloat(__sn__staticFloatCaller, &__sn__calls, 99.0f)) ? "true" : "false");
    
    printf("%s\n", (__sn__FloatingOps_staticDouble(__sn__staticDoubleCaller, &__sn__calls, 99.0)) ? "true" : "false");
    
    printf("%s\n", (__sn__FloatingOps_instanceFloat(&__sn__ops, __sn__instanceFloatCaller, &__sn__calls, 99.0f)) ? "true" : "false");
    
    printf("%s\n", (__sn__FloatingOps_instanceDouble(&__sn__ops, __sn__instanceDoubleCaller, &__sn__calls, 99.0)) ? "true" : "false");
    
    printf("%s\n", ((__sn__calls == 24LL)) ? "true" : "false");
    
    printf("%s\n", (((((((__sn__freeFloatCaller == 8.0f) && (__sn__freeDoubleCaller == 16.0)) && (__sn__staticFloatCaller == 8.0f)) && (__sn__staticDoubleCaller == 16.0)) && (__sn__instanceFloatCaller == 8.0f)) && (__sn__instanceDoubleCaller == 16.0))) ? "true" : "false");
    
    printf("%s\n", (__sn__floatSpecial(__sn__specialFloatOne, __sn__specialFloatZero, __sn__specialFloatNegativeZero)) ? "true" : "false");
    
    printf("%s\n", (__sn__doubleSpecial(__sn__specialDoubleOne, __sn__specialDoubleZero, __sn__specialDoubleNegativeZero)) ? "true" : "false");
    
    printf("%s\n", ((((__sn__specialFloatOne == 1.0f) && (__sn__specialFloatZero == 0.0f)) && ((1.0f / __sn__specialFloatNegativeZero) < 0.0f))) ? "true" : "false");
    
    printf("%s\n", ((((__sn__specialDoubleOne == 1.0) && (__sn__specialDoubleZero == 0.0)) && ((1.0 / __sn__specialDoubleNegativeZero) < 0.0))) ? "true" : "false");
    
    printf("%s\n", (__sn__helperNames(2.0f, 3.0f, 4.0f, 5.0f, 6.0f)) ? "true" : "false");
    
    printf("%s\n", (((__sn__shadowOrder(__sn__orderCaller) == 5.0) && (__sn__orderCaller == 4.0))) ? "true" : "false");
    
    fflush(stdout);
    return 0;
}
