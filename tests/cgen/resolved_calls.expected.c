#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Point (as val) */
typedef struct {
    long long __sn__value;
} __sn__Point;
/* Value operations */
static inline __sn__Point __sn__Point_copy(const __sn__Point *src) {
    __sn__Point dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__Point_cleanup(__sn__Point *p) {

}

#define sn_auto_Point __attribute__((cleanup(__sn__Point_cleanup)))

static inline void __sn__Point_cleanup_elem(void *p) { __sn__Point_cleanup((__sn__Point *)p); }
static inline void __sn__Point_copy_into(const void *src, void *dst) { *(__sn__Point *)dst = __sn__Point_copy((const __sn__Point *)src); }

/* Ref/pointer operations */
static inline __sn__Point *__sn__Point_alloc(void) {
    return calloc(1, sizeof(__sn__Point));
}

static inline void __sn__Point_release(__sn__Point **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Point __attribute__((cleanup(__sn__Point_release)))

static inline void __sn__Point_release_elem(void *p) { __sn__Point_release((__sn__Point **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Point_to_string(const __sn__Point *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Point { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Explicit (as val) */
typedef struct {
    long long __sn__value;
} __sn__Explicit;
/* Value operations */
static inline __sn__Explicit __sn__Explicit_copy(const __sn__Explicit *src) {
    __sn__Explicit dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__Explicit_cleanup(__sn__Explicit *p) {

}

#define sn_auto_Explicit __attribute__((cleanup(__sn__Explicit_cleanup)))

static inline void __sn__Explicit_cleanup_elem(void *p) { __sn__Explicit_cleanup((__sn__Explicit *)p); }
static inline void __sn__Explicit_copy_into(const void *src, void *dst) { *(__sn__Explicit *)dst = __sn__Explicit_copy((const __sn__Explicit *)src); }

/* Ref/pointer operations */
static inline __sn__Explicit *__sn__Explicit_alloc(void) {
    return calloc(1, sizeof(__sn__Explicit));
}

static inline void __sn__Explicit_release(__sn__Explicit **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Explicit __attribute__((cleanup(__sn__Explicit_release)))

static inline void __sn__Explicit_release_elem(void *p) { __sn__Explicit_release((__sn__Explicit **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Explicit_to_string(const __sn__Explicit *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Explicit { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Holder (as val) */
typedef struct {
    __sn__Point __sn__point;
} __sn__Holder;
/* Value operations */
static inline __sn__Holder __sn__Holder_copy(const __sn__Holder *src) {
    __sn__Holder dst;
    dst.__sn__point = __sn__Point_copy(&src->__sn__point);
    return dst;
}

static inline void __sn__Holder_cleanup(__sn__Holder *p) {

}

#define sn_auto_Holder __attribute__((cleanup(__sn__Holder_cleanup)))

static inline void __sn__Holder_cleanup_elem(void *p) { __sn__Holder_cleanup((__sn__Holder *)p); }
static inline void __sn__Holder_copy_into(const void *src, void *dst) { *(__sn__Holder *)dst = __sn__Holder_copy((const __sn__Holder *)src); }

/* Ref/pointer operations */
static inline __sn__Holder *__sn__Holder_alloc(void) {
    return calloc(1, sizeof(__sn__Holder));
}

static inline void __sn__Holder_release(__sn__Holder **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Holder __attribute__((cleanup(__sn__Holder_release)))

static inline void __sn__Holder_release_elem(void *p) { __sn__Holder_release((__sn__Holder **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Holder_to_string(const __sn__Holder *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Holder { ");
    off += snprintf(buf + off, sizeof(buf) - off, "point: ");
    { char *__fs__ = __sn__Point_to_string(&p->__sn__point); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: OwnedPoint (as val) */
typedef struct {
    char * __sn__label;
    SnArray * __sn__values;
} __sn__OwnedPoint;
/* Value operations */
static inline __sn__OwnedPoint __sn__OwnedPoint_copy(const __sn__OwnedPoint *src) {
    __sn__OwnedPoint dst;
    dst.__sn__label = src->__sn__label ? strdup(src->__sn__label) : NULL;
    dst.__sn__values = sn_array_copy(src->__sn__values);
    return dst;
}

static inline void __sn__OwnedPoint_cleanup(__sn__OwnedPoint *p) {
    free(p->__sn__label);
    sn_cleanup_array(&p->__sn__values);

}

#define sn_auto_OwnedPoint __attribute__((cleanup(__sn__OwnedPoint_cleanup)))

static inline void __sn__OwnedPoint_cleanup_elem(void *p) { __sn__OwnedPoint_cleanup((__sn__OwnedPoint *)p); }
static inline void __sn__OwnedPoint_copy_into(const void *src, void *dst) { *(__sn__OwnedPoint *)dst = __sn__OwnedPoint_copy((const __sn__OwnedPoint *)src); }

/* Ref/pointer operations */
static inline __sn__OwnedPoint *__sn__OwnedPoint_alloc(void) {
    return calloc(1, sizeof(__sn__OwnedPoint));
}

static inline void __sn__OwnedPoint_release(__sn__OwnedPoint **p) {
    if (*p) {
        free((*p)->__sn__label);
        sn_cleanup_array(&(*p)->__sn__values);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_OwnedPoint __attribute__((cleanup(__sn__OwnedPoint_release)))

static inline void __sn__OwnedPoint_release_elem(void *p) { __sn__OwnedPoint_release((__sn__OwnedPoint **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__OwnedPoint_to_string(const __sn__OwnedPoint *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "OwnedPoint { ");
    off += snprintf(buf + off, sizeof(buf) - off, "label: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__label ? p->__sn__label : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "values: ");
    { char *__fs__ = sn_array_to_string(p->__sn__values); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: CallValues (as val) */
typedef struct {
    char * __sn__label;
} __sn__CallValues;
/* Value operations */
static inline __sn__CallValues __sn__CallValues_copy(const __sn__CallValues *src) {
    __sn__CallValues dst;
    dst.__sn__label = src->__sn__label ? strdup(src->__sn__label) : NULL;
    return dst;
}

static inline void __sn__CallValues_cleanup(__sn__CallValues *p) {
    free(p->__sn__label);

}

#define sn_auto_CallValues __attribute__((cleanup(__sn__CallValues_cleanup)))

static inline void __sn__CallValues_cleanup_elem(void *p) { __sn__CallValues_cleanup((__sn__CallValues *)p); }
static inline void __sn__CallValues_copy_into(const void *src, void *dst) { *(__sn__CallValues *)dst = __sn__CallValues_copy((const __sn__CallValues *)src); }

/* Ref/pointer operations */
static inline __sn__CallValues *__sn__CallValues_alloc(void) {
    return calloc(1, sizeof(__sn__CallValues));
}

static inline void __sn__CallValues_release(__sn__CallValues **p) {
    if (*p) {
        free((*p)->__sn__label);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_CallValues __attribute__((cleanup(__sn__CallValues_release)))

static inline void __sn__CallValues_release_elem(void *p) { __sn__CallValues_release((__sn__CallValues **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__CallValues_to_string(const __sn__CallValues *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "CallValues { ");
    off += snprintf(buf + off, sizeof(buf) - off, "label: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__label ? p->__sn__label : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



__sn__Point __sn__markedPoint(long long *, long long *, long long, long long);
__sn__OwnedPoint __sn__markedOwnedPoint(long long *, long long *, long long, long long);
bool __sn__acceptBool(bool);
bool __sn__returnedComparison(__sn__Point, __sn__Point);
bool __sn__Point_op_eq(__sn__Point *, __sn__Point);
bool __sn__Point_op_lt(__sn__Point *, __sn__Point);
bool __sn__Explicit_op_ne(__sn__Explicit *, __sn__Explicit);
bool __sn__OwnedPoint_op_eq(__sn__OwnedPoint *, __sn__OwnedPoint *);
bool __sn__OwnedPoint_op_lt(__sn__OwnedPoint *, __sn__OwnedPoint *);
bool __sn__CallValues_labelMatches(char *);
char * __sn__CallValues_makeLabel();
long long __sn__CallValues_countNumbers(SnArray *);
SnArray * __sn__CallValues_makeNumbers();
char * __sn__CallValues_joinLabel(__sn__CallValues *, char *);
long long __sn__CallValues_countNumbersAgain(__sn__CallValues *, SnArray *);
SnArray * __sn__CallValues_makeNumbersAgain(__sn__CallValues *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


__sn__Point __sn__markedPoint(long long *__sn__calls, long long *__sn__order, long long __sn__marker, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return (__sn__Point){ .__sn__value = __sn__value };}


__sn__OwnedPoint __sn__markedOwnedPoint(long long *__sn__calls, long long *__sn__order, long long __sn__marker, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return (__sn__OwnedPoint){ .__sn__label = ({
             sn_auto_str char *__is_p0__ = sn_strdup("value-");
             sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__value));
             sn_str_concat_multi(2, __is_p0__, __is_p1__);
         }), .__sn__values = ({
             SnArray *__al__ = sn_array_new(sizeof(long long), 1);
             __al__->elem_tag = SN_TAG_INT;
     
     
             sn_array_push(__al__, &(long long){ __sn__value });
             __al__;
         }) };}


bool __sn__acceptBool(bool __sn__value) {

    return __sn__value;}


bool __sn__returnedComparison(__sn__Point __sn__left, __sn__Point __sn__right) {

    return __sn__Point_op_eq(&__sn__left, __sn__right);}


bool __sn__Point_op_eq(__sn__Point *__sn__self, __sn__Point __sn__other) {

    return (__sn__self->__sn__value == __sn__other.__sn__value);}

bool __sn__Point_op_lt(__sn__Point *__sn__self, __sn__Point __sn__other) {

    return sn_lt_long(__sn__self->__sn__value, __sn__other.__sn__value);}


bool __sn__Explicit_op_ne(__sn__Explicit *__sn__self, __sn__Explicit __sn__other) {

    return (__sn__self->__sn__value != __sn__other.__sn__value);}



bool __sn__OwnedPoint_op_eq(__sn__OwnedPoint *__sn__self, __sn__OwnedPoint *__sn__other) {

    return ((strcmp(__sn__self->__sn__label, (*__sn__other).__sn__label) == 0) && (sn_array_length(__sn__self->__sn__values) == sn_array_length((*__sn__other).__sn__values)));}

bool __sn__OwnedPoint_op_lt(__sn__OwnedPoint *__sn__self, __sn__OwnedPoint *__sn__other) {

    return sn_lt_long((((long long *)__sn__self->__sn__values->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__self->__sn__values->len : __ai__; })]), (((long long *)(*__sn__other).__sn__values->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + (*__sn__other).__sn__values->len : __ai__; })]));}


bool __sn__CallValues_labelMatches(char * __sn__value) {

    return (strcmp(__sn__value, "source") == 0);}

char * __sn__CallValues_makeLabel() {

    return ({
             sn_auto_str char *__is_p0__ = sn_strdup("source");
             sn_str_concat_multi(1, __is_p0__);
         });}

long long __sn__CallValues_countNumbers(SnArray * __sn__values) {

    return sn_array_length(__sn__values);}

SnArray * __sn__CallValues_makeNumbers() {

    return ({
             SnArray *__al__ = sn_array_new(sizeof(long long), 2);
             __al__->elem_tag = SN_TAG_INT;
     
     
             sn_array_push(__al__, &(long long){ 1LL });
     
             sn_array_push(__al__, &(long long){ 2LL });
             __al__;
         });}

char * __sn__CallValues_joinLabel(__sn__CallValues *__sn__self, char * __sn__value) {

    return ({
             sn_auto_str char *__is_p0__ = sn_strdup(__sn__self->__sn__label);
             sn_auto_str char *__is_p1__ = sn_strdup(":");
             sn_auto_str char *__is_p2__ = sn_strdup(__sn__value);
             sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
         });}

long long __sn__CallValues_countNumbersAgain(__sn__CallValues *__sn__self, SnArray * __sn__values) {

    return sn_array_length(__sn__values);}

SnArray * __sn__CallValues_makeNumbersAgain(__sn__CallValues *__sn__self) {

    return ({
             SnArray *__al__ = sn_array_new(sizeof(long long), 2);
             __al__->elem_tag = SN_TAG_INT;
     
     
             sn_array_push(__al__, &(long long){ 3LL });
     
             sn_array_push(__al__, &(long long){ 4LL });
             __al__;
         });}

int main() {
    __sn__Point __sn__left = (__sn__Point){ .__sn__value = 7LL };
    __sn__Point __sn__same = (__sn__Point){ .__sn__value = 7LL };
    __sn__Point __sn__greater = (__sn__Point){ .__sn__value = 9LL };
    __sn__Holder __sn__holder = (__sn__Holder){ .__sn__point = (__sn__Point){ .__sn__value = 7LL } };
    sn_auto_arr SnArray * __sn__points = ({
            SnArray *__al__ = sn_array_new(sizeof(__sn__Point), 0);
            __al__->elem_tag = SN_TAG_STRUCT;
    
            __al__;
        });
    __sn__arr_push(&__sn__points, __sn__left);
    
    __sn__arr_push(&__sn__points, __sn__greater);
    
    __sn__Point_op_eq(&__sn__left, __sn__same);
    
    bool __sn__initialized = __sn__Point_op_eq(&__sn__left, __sn__same);
    bool __sn__argument = __sn__acceptBool((!__sn__Point_op_eq(&__sn__left, __sn__greater)));
    bool __sn__returned = __sn__returnedComparison(__sn__left, __sn__same);
    bool __sn__memberReceiver = __sn__Point_op_eq(&__sn__holder.__sn__point, __sn__same);
    bool __sn__indexedReceiver = __sn__Point_op_lt(&(((__sn__Point *)__sn__points->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__points->len : __ai__; })]), (((__sn__Point *)__sn__points->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__points->len : __ai__; })]));
    sn_auto_Explicit __sn__Explicit __sn____chain_tmp_0 = (__sn__Explicit){ .__sn__value = 1LL };
    bool __sn__explicitNe = __sn__Explicit_op_ne(&__sn____chain_tmp_0, (__sn__Explicit){ .__sn__value = 2LL });
    bool __sn__derivedGe = (!__sn__Point_op_lt(&__sn__greater, __sn__left));
    bool __sn__derivedLe = (!({ __sn__Point __sn_resolved_source_arg__ = __sn__left; __sn__Point *__sn_resolved_source_receiver__ = &__sn__greater; __sn__Point_op_lt(__sn_resolved_source_receiver__, __sn_resolved_source_arg__); }));
    bool __sn__matched = ({
            bool __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                (!__sn__Point_op_eq(&__sn__left, __sn__greater));
    
    __match_result__ = __sn__Point_op_eq(&__sn__left, __sn__same);
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    long long __sn__calls = 0LL;
    long long __sn__order = 0LL;
    long long __sn____sn_resolved_arg_0 = 40LL;
    long long __sn____sn_resolved_receiver_0 = 2LL;
    sn_auto_Point __sn__Point __sn____chain_tmp_1 = __sn__markedPoint(&__sn__calls, &__sn__order, 1LL, 1LL);
    bool __sn__directOrder = __sn__Point_op_lt(&__sn____chain_tmp_1, __sn__markedPoint(&__sn__calls, &__sn__order, 2LL, 2LL));
    bool __sn__swappedOrder = ({ __sn__Point __sn_resolved_source_arg__ = __sn__markedPoint(&__sn__calls, &__sn__order, 3LL, 3LL); __sn__Point __sn_resolved_source_receiver__ = __sn__markedPoint(&__sn__calls, &__sn__order, 4LL, 4LL); __sn__Point_op_lt(&__sn_resolved_source_receiver__, __sn_resolved_source_arg__); });
    sn_auto_OwnedPoint __sn__OwnedPoint __sn__ownedLeft = (__sn__OwnedPoint){ .__sn__label = strdup("owned"), .__sn__values = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 2);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 5LL });
    
            sn_array_push(__al__, &(long long){ 6LL });
            __al__;
        }) };
    sn_auto_OwnedPoint __sn__OwnedPoint __sn__ownedSame = __sn__OwnedPoint_copy(&__sn__ownedLeft);
    bool __sn__ownedEqual = __sn__OwnedPoint_op_eq(&__sn__ownedLeft, &__sn__ownedSame);
    bool __sn__ownedSwapped = ({ sn_auto_OwnedPoint __sn__OwnedPoint __sn_resolved_source_arg__ = __sn__markedOwnedPoint(&__sn__calls, &__sn__order, 5LL, 5LL); sn_auto_OwnedPoint __sn__OwnedPoint __sn_resolved_source_receiver__ = __sn__markedOwnedPoint(&__sn__calls, &__sn__order, 6LL, 6LL); __sn__OwnedPoint_op_lt(&__sn_resolved_source_receiver__, &__sn_resolved_source_arg__); });
    __sn__arr_push(&__sn__ownedSame.__sn__values, 7LL);
    
    sn_auto_str char * __sn__sourceLabel = strdup("source");
    sn_auto_arr SnArray * __sn__sourceNumbers = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 2);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 1LL });
    
            sn_array_push(__al__, &(long long){ 2LL });
            __al__;
        });
    sn_auto_CallValues __sn__CallValues __sn__callValues = (__sn__CallValues){ .__sn__label = strdup("prefix") };
    bool __sn__staticMatch = __sn__CallValues_labelMatches(__sn__sourceLabel);
    sn_auto_str char * __sn__staticLabel = __sn__CallValues_makeLabel();
    sn_auto_str char * __sn__instanceLabel = __sn__CallValues_joinLabel(&__sn__callValues, __sn__sourceLabel);
    long long __sn__staticCount = __sn__CallValues_countNumbers(__sn__sourceNumbers);
    long long __sn__instanceCount = __sn__CallValues_countNumbersAgain(&__sn__callValues, __sn__sourceNumbers);
    sn_auto_arr SnArray * __sn__staticNumbers = __sn__CallValues_makeNumbers();
    sn_auto_arr SnArray * __sn__instanceNumbers = __sn__CallValues_makeNumbersAgain(&__sn__callValues);
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((__sn__initialized) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup("|");
            sn_auto_str char *__is_p2__ = sn_strdup((__sn__argument) ? "true" : "false");
            sn_auto_str char *__is_p3__ = sn_strdup("|");
            sn_auto_str char *__is_p4__ = sn_strdup((__sn__returned) ? "true" : "false");
            sn_auto_str char *__is_p5__ = sn_strdup("|");
            sn_auto_str char *__is_p6__ = sn_strdup((__sn__memberReceiver) ? "true" : "false");
            sn_auto_str char *__is_p7__ = sn_strdup("|");
            sn_auto_str char *__is_p8__ = sn_strdup((__sn__indexedReceiver) ? "true" : "false");
            sn_auto_str char *__is_p9__ = sn_strdup("|");
            sn_auto_str char *__is_p10__ = sn_strdup((__sn__explicitNe) ? "true" : "false");
            sn_auto_str char *__is_p11__ = sn_strdup("|");
            sn_auto_str char *__is_p12__ = sn_strdup((__sn__derivedGe) ? "true" : "false");
            sn_auto_str char *__is_p13__ = sn_strdup("|");
            sn_auto_str char *__is_p14__ = sn_strdup((__sn__derivedLe) ? "true" : "false");
            sn_auto_str char *__is_p15__ = sn_strdup("|");
            sn_auto_str char *__is_p16__ = sn_strdup((__sn__matched) ? "true" : "false");
            sn_str_concat_multi(17, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((__sn__directOrder) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup("|");
            sn_auto_str char *__is_p2__ = sn_strdup((__sn__swappedOrder) ? "true" : "false");
            sn_auto_str char *__is_p3__ = sn_strdup("|");
            sn_auto_str char *__is_p4__ = sn_strdup((__sn__ownedEqual) ? "true" : "false");
            sn_auto_str char *__is_p5__ = sn_strdup("|");
            sn_auto_str char *__is_p6__ = sn_strdup((__sn__ownedSwapped) ? "true" : "false");
            sn_auto_str char *__is_p7__ = sn_strdup("|");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__calls));
            sn_auto_str char *__is_p9__ = sn_strdup("|");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__order));
            sn_auto_str char *__is_p11__ = sn_strdup("|");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(sn_add_long(__sn____sn_resolved_arg_0, __sn____sn_resolved_receiver_0)));
            sn_str_concat_multi(13, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup(__sn__ownedLeft.__sn__label);
            sn_auto_str char *__is_p1__ = sn_strdup("|");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(sn_array_length(__sn__ownedLeft.__sn__values)));
            sn_auto_str char *__is_p3__ = sn_strdup("|");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(sn_array_length(__sn__ownedSame.__sn__values)));
            sn_str_concat_multi(5, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((__sn__staticMatch) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup("|");
            sn_auto_str char *__is_p2__ = sn_strdup(__sn__staticLabel);
            sn_auto_str char *__is_p3__ = sn_strdup("|");
            sn_auto_str char *__is_p4__ = sn_strdup(__sn__instanceLabel);
            sn_auto_str char *__is_p5__ = sn_strdup("|");
            sn_auto_str char *__is_p6__ = sn_strdup(__sn__sourceLabel);
            sn_str_concat_multi(7, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(sn_array_length(__sn__sourceNumbers)));
            sn_auto_str char *__is_p1__ = sn_strdup("|");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__staticCount));
            sn_auto_str char *__is_p3__ = sn_strdup("|");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__instanceCount));
            sn_auto_str char *__is_p5__ = sn_strdup("|");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(sn_array_length(__sn__staticNumbers)));
            sn_auto_str char *__is_p7__ = sn_strdup("|");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(sn_array_length(__sn__instanceNumbers)));
            sn_str_concat_multi(9, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
