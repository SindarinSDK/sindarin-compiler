#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Point (as val) */
typedef struct {
    long long __sn__x;
} __sn__Point;
/* Value operations */
static inline __sn__Point __sn__Point_copy(const __sn__Point *src) {
    __sn__Point dst;
    dst.__sn__x = src->__sn__x;
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
    off += snprintf(buf + off, sizeof(buf) - off, "x: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__x);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



char * __sn__observe_text(long long *);
SnArray * __sn__observe_values(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char * __sn__observe_text(long long *__sn__counter) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__counter));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return strdup("called");}


SnArray * __sn__observe_values(long long *__sn__counter) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__counter));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return ({
             SnArray *__al__ = sn_array_new(sizeof(long long), 2);
             __al__->elem_tag = SN_TAG_INT;
     
     
             sn_array_push(__al__, &(long long){ 1LL });
     
             sn_array_push(__al__, &(long long){ 2LL });
             __al__;
         });}


int main() {
    long long __sn__type_string = sizeof(char *);
    long long __sn__type_array = sizeof(SnArray *);
    long long __sn__type_nested_array = sizeof(SnArray *);
    long long __sn__type_struct_array = sizeof(SnArray *);
    sn_auto_str char * __sn__text = strdup("managed");
    sn_auto_arr SnArray * __sn__values = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 2);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 1LL });
    
            sn_array_push(__al__, &(long long){ 2LL });
            __al__;
        });
    sn_auto_arr SnArray * __sn__rows = ({
            SnArray *__al__ = sn_array_new(sizeof(SnArray *), 1);
            __al__->elem_tag = SN_TAG_ARRAY;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_array;
    
            __al__->elem_copy = sn_copy_array;
    
            sn_array_push(__al__, &(SnArray *){ ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 2);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 1LL });
    
            sn_array_push(__al__, &(long long){ 2LL });
            __al__;
        }) });
            __al__;
        });
    sn_auto_arr SnArray * __sn__points = ({
            SnArray *__al__ = sn_array_new(sizeof(__sn__Point), 1);
            __al__->elem_tag = SN_TAG_STRUCT;
    
    
            sn_array_push(__al__, &((__sn__Point){ .__sn__x = 1LL }));
            __al__;
        });
    long long __sn__counter = 0LL;
    long long __sn__expression_sizes = sn_add_long(sn_add_long(sn_add_long(sn_add_long(sn_add_long(sizeof(char *), sizeof(SnArray *)), sizeof(SnArray *)), sizeof(SnArray *)), sizeof(char *)), sizeof(SnArray *));
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((((((__sn__type_string == 8LL) && (__sn__type_array == 8LL)) && (__sn__type_nested_array == 8LL)) && (__sn__type_struct_array == 8LL))) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup(" ");
            sn_auto_str char *__is_p2__ = sn_strdup(((__sn__expression_sizes == 48LL)) ? "true" : "false");
            sn_auto_str char *__is_p3__ = sn_strdup(" ");
            sn_auto_str char *__is_p4__ = sn_strdup(((__sn__counter == 0LL)) ? "true" : "false");
            sn_str_concat_multi(5, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
