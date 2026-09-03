#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: FloatingPostfixValues (as val) */
typedef struct {
    float __sn__single;
    double __sn__precise;
} __sn__FloatingPostfixValues;
/* Value operations */
static inline __sn__FloatingPostfixValues __sn__FloatingPostfixValues_copy(const __sn__FloatingPostfixValues *src) {
    __sn__FloatingPostfixValues dst;
    dst.__sn__single = src->__sn__single;
    dst.__sn__precise = src->__sn__precise;
    return dst;
}

static inline void __sn__FloatingPostfixValues_cleanup(__sn__FloatingPostfixValues *p) {

}

#define sn_auto_FloatingPostfixValues __attribute__((cleanup(__sn__FloatingPostfixValues_cleanup)))

static inline void __sn__FloatingPostfixValues_cleanup_elem(void *p) { __sn__FloatingPostfixValues_cleanup((__sn__FloatingPostfixValues *)p); }
static inline void __sn__FloatingPostfixValues_copy_into(const void *src, void *dst) { *(__sn__FloatingPostfixValues *)dst = __sn__FloatingPostfixValues_copy((const __sn__FloatingPostfixValues *)src); }

/* Ref/pointer operations */
static inline __sn__FloatingPostfixValues *__sn__FloatingPostfixValues_alloc(void) {
    return calloc(1, sizeof(__sn__FloatingPostfixValues));
}

static inline void __sn__FloatingPostfixValues_release(__sn__FloatingPostfixValues **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_FloatingPostfixValues __attribute__((cleanup(__sn__FloatingPostfixValues_release)))

static inline void __sn__FloatingPostfixValues_release_elem(void *p) { __sn__FloatingPostfixValues_release((__sn__FloatingPostfixValues **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__FloatingPostfixValues_to_string(const __sn__FloatingPostfixValues *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "FloatingPostfixValues { ");
    off += snprintf(buf + off, sizeof(buf) - off, "single: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__single);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "precise: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__precise);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



bool __sn__FloatingPostfixValues_mutateSelf(__sn__FloatingPostfixValues *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


bool __sn__FloatingPostfixValues_mutateSelf(__sn__FloatingPostfixValues *__sn__self) {

    double __sn__before_increment = __sn__self->__sn__precise++;

    double __sn__before_decrement = __sn__self->__sn__precise--;

    return (((__sn__before_increment == 6.0) && (__sn__before_decrement == 7.0)) && (__sn__self->__sn__precise == 6.0));}

int main() {
    float __sn__single = 1.5;
    float __sn__single_before_increment = __sn__single++;
    float __sn__single_before_decrement = __sn__single--;
    printf("%s\n", ((((__sn__single_before_increment == 1.5) && (__sn__single_before_decrement == 2.5)) && (__sn__single == 1.5))) ? "true" : "false");
    
    double __sn__precise = 3.25;
    double __sn__precise_before_increment = __sn__precise++;
    double __sn__precise_before_decrement = __sn__precise--;
    printf("%s\n", ((((__sn__precise_before_increment == 3.25) && (__sn__precise_before_decrement == 4.25)) && (__sn__precise == 3.25))) ? "true" : "false");
    
    __sn__FloatingPostfixValues __sn__fields = (__sn__FloatingPostfixValues){ .__sn__single = 2.0, .__sn__precise = 6.0 };
    float __sn__field_single_before = __sn__fields.__sn__single++;
    double __sn__field_precise_before = __sn__fields.__sn__precise--;
    printf("%s\n", (((((__sn__field_single_before == 2.0) && (__sn__fields.__sn__single == 3.0)) && (__sn__field_precise_before == 6.0)) && (__sn__fields.__sn__precise == 5.0))) ? "true" : "false");
    
    __sn__fields.__sn__precise++;
    
    printf("%s\n", (__sn__FloatingPostfixValues_mutateSelf(&__sn__fields)) ? "true" : "false");
    
    sn_auto_arr SnArray * __sn__singles = ({
            SnArray *__al__ = sn_array_new(sizeof(float), 1);
            __al__->elem_tag = SN_TAG_DOUBLE;
    
    
            sn_array_push(__al__, &(float){ 4.0 });
            __al__;
        });
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__singles);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            float __sn__value__source = ((float *)__arr_0__->data)[__idx_0__];
            float __sn__value = __sn__value__source;
            {
                float __sn__before = __sn__value++;
                printf("%s\n", ((((__sn__before == 4.0) && (__sn__value == 5.0)) && ((((float *)__sn__singles->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__singles->len : __ai__; })]) == 4.0))) ? "true" : "false");
                
            }
        }
    }
    sn_auto_arr SnArray * __sn__doubles = ({
            SnArray *__al__ = sn_array_new(sizeof(double), 1);
            __al__->elem_tag = SN_TAG_DOUBLE;
    
    
            sn_array_push(__al__, &(double){ 9.0 });
            __al__;
        });
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__doubles);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            double __sn__value__source = ((double *)__arr_0__->data)[__idx_0__];
            double __sn__value = __sn__value__source;
            {
                double __sn__before = __sn__value--;
                printf("%s\n", ((((__sn__before == 9.0) && (__sn__value == 8.0)) && ((((double *)__sn__doubles->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__doubles->len : __ai__; })]) == 9.0))) ? "true" : "false");
                
            }
        }
    }
    float __sn____sn_place = 10.0;
    float __sn__place_before = __sn____sn_place++;
    double __sn____sn_previous = 12.0;
    double __sn__previous_before = __sn____sn_previous--;
    float __sn____sn_next = 14.0;
    float __sn__next_before = __sn____sn_next++;
    printf("%s\n", (((((((__sn__place_before == 10.0) && (__sn____sn_place == 11.0)) && (__sn__previous_before == 12.0)) && (__sn____sn_previous == 11.0)) && (__sn__next_before == 14.0)) && (__sn____sn_next == 15.0))) ? "true" : "false");
    
    fflush(stdout);
    return 0;
}
