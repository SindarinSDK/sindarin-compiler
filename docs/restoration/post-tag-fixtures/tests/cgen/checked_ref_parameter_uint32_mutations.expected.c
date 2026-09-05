#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Box (as val) */
typedef struct {
    uint32_t __sn__value;
} __sn__Box;
/* Value operations */
static inline __sn__Box __sn__Box_copy(const __sn__Box *src) {
    __sn__Box dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__Box_cleanup(__sn__Box *p) {

}

#define sn_auto_Box __attribute__((cleanup(__sn__Box_cleanup)))

static inline void __sn__Box_cleanup_elem(void *p) { __sn__Box_cleanup((__sn__Box *)p); }
static inline void __sn__Box_copy_into(const void *src, void *dst) { *(__sn__Box *)dst = __sn__Box_copy((const __sn__Box *)src); }

/* Ref/pointer operations */
static inline __sn__Box *__sn__Box_alloc(void) {
    return calloc(1, sizeof(__sn__Box));
}

static inline void __sn__Box_release(__sn__Box **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Box __attribute__((cleanup(__sn__Box_release)))

static inline void __sn__Box_release_elem(void *p) { __sn__Box_release((__sn__Box **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Box_to_string(const __sn__Box *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Box { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Wrapper (as val) */
typedef struct {
    __sn__Box __sn__box;
} __sn__Wrapper;
/* Value operations */
static inline __sn__Wrapper __sn__Wrapper_copy(const __sn__Wrapper *src) {
    __sn__Wrapper dst;
    dst.__sn__box = __sn__Box_copy(&src->__sn__box);
    return dst;
}

static inline void __sn__Wrapper_cleanup(__sn__Wrapper *p) {

}

#define sn_auto_Wrapper __attribute__((cleanup(__sn__Wrapper_cleanup)))

static inline void __sn__Wrapper_cleanup_elem(void *p) { __sn__Wrapper_cleanup((__sn__Wrapper *)p); }
static inline void __sn__Wrapper_copy_into(const void *src, void *dst) { *(__sn__Wrapper *)dst = __sn__Wrapper_copy((const __sn__Wrapper *)src); }

/* Ref/pointer operations */
static inline __sn__Wrapper *__sn__Wrapper_alloc(void) {
    return calloc(1, sizeof(__sn__Wrapper));
}

static inline void __sn__Wrapper_release(__sn__Wrapper **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Wrapper __attribute__((cleanup(__sn__Wrapper_release)))

static inline void __sn__Wrapper_release_elem(void *p) { __sn__Wrapper_release((__sn__Wrapper **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Wrapper_to_string(const __sn__Wrapper *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Wrapper { ");
    off += snprintf(buf + off, sizeof(buf) - off, "box: ");
    { char *__fs__ = __sn__Box_to_string(&p->__sn__box); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: RefOps (as val) */
typedef struct {
} __sn__RefOps;
/* Value operations */
static inline __sn__RefOps __sn__RefOps_copy(const __sn__RefOps *src) {
    __sn__RefOps dst;
    return dst;
}

static inline void __sn__RefOps_cleanup(__sn__RefOps *p) {

}

#define sn_auto_RefOps __attribute__((cleanup(__sn__RefOps_cleanup)))

static inline void __sn__RefOps_cleanup_elem(void *p) { __sn__RefOps_cleanup((__sn__RefOps *)p); }
static inline void __sn__RefOps_copy_into(const void *src, void *dst) { *(__sn__RefOps *)dst = __sn__RefOps_copy((const __sn__RefOps *)src); }

/* Ref/pointer operations */
static inline __sn__RefOps *__sn__RefOps_alloc(void) {
    return calloc(1, sizeof(__sn__RefOps));
}

static inline void __sn__RefOps_release(__sn__RefOps **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_RefOps __attribute__((cleanup(__sn__RefOps_release)))

static inline void __sn__RefOps_release_elem(void *p) { __sn__RefOps_release((__sn__RefOps **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__RefOps_to_string(const __sn__RefOps *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "RefOps { ");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



uint32_t __sn__oldThenIncrement(uint32_t *);
uint32_t __sn__freeOps(uint32_t *);
uint32_t __sn__forwardStatic(uint32_t *);
uint32_t __sn__forwardInstance(uint32_t *);
uint32_t __sn__maxBoundary(uint32_t *);
uint32_t __sn__minBoundary(uint32_t *);
uint32_t __sn__assignAndRead(uint32_t *);
uint32_t __sn__read(uint32_t *);
void __sn__freeBump(uint32_t *);
uint32_t __sn__RefOps_staticOps(uint32_t *);
void __sn__RefOps_staticBump(uint32_t *);
uint32_t __sn__RefOps_instancePostfix(__sn__RefOps *, uint32_t *);
void __sn__RefOps_instanceBump(__sn__RefOps *, uint32_t *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


uint32_t __sn__oldThenIncrement(uint32_t *__sn__value) {

    return ({
         uint32_t *__sn_place__ = &((*__sn__value));
         uint32_t __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_add_uint32(__sn_previous__, 1);
         __sn_previous__;
     });}


uint32_t __sn__freeOps(uint32_t *__sn__value) {

    uint32_t __sn__add = ({
        uint32_t __sn_rhs__ = __sn__oldThenIncrement(&(*__sn__value));
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__sub = ({
        uint32_t __sn_rhs__ = 1LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__mul = ({
        uint32_t __sn_rhs__ = 2LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__div = ({
        uint32_t __sn_rhs__ = 2LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__rem = ({
        uint32_t __sn_rhs__ = 3LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__old_inc = ({
        uint32_t *__sn_place__ = &((*__sn__value));
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint32(__sn_previous__, 1);
        __sn_previous__;
    });

    uint32_t __sn__old_dec = ({
        uint32_t *__sn_place__ = &((*__sn__value));
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint32(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}


uint32_t __sn__forwardStatic(uint32_t *__sn__value) {

    __sn__RefOps_staticBump(&(*__sn__value));
    

    return (*__sn__value);}


uint32_t __sn__forwardInstance(uint32_t *__sn__value) {

    __sn__RefOps __sn__ops = (__sn__RefOps){  };

    __sn__RefOps_instanceBump(&__sn__ops, &(*__sn__value));
    

    return (*__sn__value);}


uint32_t __sn__maxBoundary(uint32_t *__sn__value) {

    return ({
         uint32_t __sn_rhs__ = 1LL;
         uint32_t *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}


uint32_t __sn__minBoundary(uint32_t *__sn__value) {

    return ({
         uint32_t __sn_rhs__ = 1LL;
         uint32_t *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_sub_uint32(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}


uint32_t __sn__assignAndRead(uint32_t *__sn__value) {

    (*__sn__value = 4000000000LL);
    

    return (*__sn__value);}


uint32_t __sn__read(uint32_t *__sn__value) {

    return (*__sn__value);}


void __sn__freeBump(uint32_t *__sn__value) {

    ({
        uint32_t __sn_rhs__ = 1LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}




uint32_t __sn__RefOps_staticOps(uint32_t *__sn__value) {

    uint32_t __sn__add = ({
        uint32_t __sn_rhs__ = 3LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__sub = ({
        uint32_t __sn_rhs__ = 1LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__mul = ({
        uint32_t __sn_rhs__ = 2LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__div = ({
        uint32_t __sn_rhs__ = 2LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__rem = ({
        uint32_t __sn_rhs__ = 3LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    uint32_t __sn__old_inc = ({
        uint32_t *__sn_place__ = &((*__sn__value));
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_uint32(__sn_previous__, 1);
        __sn_previous__;
    });

    uint32_t __sn__old_dec = ({
        uint32_t *__sn_place__ = &((*__sn__value));
        uint32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_uint32(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(sn_add_uint32(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}

void __sn__RefOps_staticBump(uint32_t *__sn__value) {

    ({
        uint32_t __sn_rhs__ = 1LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}

uint32_t __sn__RefOps_instancePostfix(__sn__RefOps *__sn__self, uint32_t *__sn__value) {

    return ({
         uint32_t *__sn_place__ = &((*__sn__value));
         uint32_t __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_sub_uint32(__sn_previous__, 1);
         __sn_previous__;
     });}

void __sn__RefOps_instanceBump(__sn__RefOps *__sn__self, uint32_t *__sn__value) {

    ({
        uint32_t __sn_rhs__ = 1LL;
        uint32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_uint32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}

int main() {
    uint32_t __sn__free_value = 2LL;
    uint32_t __sn__static_value = 10LL;
    uint32_t __sn__postfix_value = 5LL;
    uint32_t __sn__forwarded_static = 1LL;
    uint32_t __sn__forwarded_instance = 1LL;
    uint32_t __sn__maximum = 4294967294LL;
    uint32_t __sn__minimum = 1LL;
    uint32_t __sn__assigned = 42LL;
    __sn__Box __sn__direct = (__sn__Box){ .__sn__value = 1LL };
    __sn__Wrapper __sn__nested = (__sn__Wrapper){ .__sn__box = (__sn__Box){ .__sn__value = 10LL } };
    __sn__RefOps __sn__ops = (__sn__RefOps){  };
    __sn__freeBump(&__sn__direct.__sn__value);
    
    __sn__freeBump(&__sn__nested.__sn__box.__sn__value);
    
    __sn__RefOps_staticBump(&__sn__direct.__sn__value);
    
    __sn__RefOps_staticBump(&__sn__nested.__sn__box.__sn__value);
    
    __sn__RefOps_instanceBump(&__sn__ops, &__sn__direct.__sn__value);
    
    __sn__RefOps_instanceBump(&__sn__ops, &__sn__nested.__sn__box.__sn__value);
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__freeOps(&__sn__free_value)));
            sn_auto_str char *__is_p1__ = sn_strdup(" ");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__free_value));
            sn_auto_str char *__is_p3__ = sn_strdup(" ");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__RefOps_staticOps(&__sn__static_value)));
            sn_auto_str char *__is_p5__ = sn_strdup(" ");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__static_value));
            sn_auto_str char *__is_p7__ = sn_strdup(" ");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__RefOps_instancePostfix(&__sn__ops, &__sn__postfix_value)));
            sn_auto_str char *__is_p9__ = sn_strdup(" ");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__postfix_value));
            sn_auto_str char *__is_p11__ = sn_strdup(" ");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__forwardStatic(&__sn__forwarded_static)));
            sn_auto_str char *__is_p13__ = sn_strdup(" ");
            sn_auto_str char *__is_p14__ = sn_str_fmt("%lld", (long long)(__sn__forwarded_static));
            sn_auto_str char *__is_p15__ = sn_strdup(" ");
            sn_auto_str char *__is_p16__ = sn_str_fmt("%lld", (long long)(__sn__forwardInstance(&__sn__forwarded_instance)));
            sn_auto_str char *__is_p17__ = sn_strdup(" ");
            sn_auto_str char *__is_p18__ = sn_str_fmt("%lld", (long long)(__sn__forwarded_instance));
            sn_auto_str char *__is_p19__ = sn_strdup(" ");
            sn_auto_str char *__is_p20__ = sn_str_fmt("%lld", (long long)(__sn__maxBoundary(&__sn__maximum)));
            sn_auto_str char *__is_p21__ = sn_strdup(" ");
            sn_auto_str char *__is_p22__ = sn_str_fmt("%lld", (long long)(__sn__minBoundary(&__sn__minimum)));
            sn_auto_str char *__is_p23__ = sn_strdup(" ");
            sn_auto_str char *__is_p24__ = sn_str_fmt("%lld", (long long)(__sn__assignAndRead(&__sn__assigned)));
            sn_auto_str char *__is_p25__ = sn_strdup(" ");
            sn_auto_str char *__is_p26__ = sn_str_fmt("%lld", (long long)(__sn__assigned));
            sn_auto_str char *__is_p27__ = sn_strdup(" ");
            sn_auto_str char *__is_p28__ = sn_str_fmt("%lld", (long long)(__sn__read(&__sn__direct.__sn__value)));
            sn_auto_str char *__is_p29__ = sn_strdup(" ");
            sn_auto_str char *__is_p30__ = sn_str_fmt("%lld", (long long)(__sn__direct.__sn__value));
            sn_auto_str char *__is_p31__ = sn_strdup(" ");
            sn_auto_str char *__is_p32__ = sn_str_fmt("%lld", (long long)(__sn__nested.__sn__box.__sn__value));
            sn_str_concat_multi(33, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__, __is_p17__, __is_p18__, __is_p19__, __is_p20__, __is_p21__, __is_p22__, __is_p23__, __is_p24__, __is_p25__, __is_p26__, __is_p27__, __is_p28__, __is_p29__, __is_p30__, __is_p31__, __is_p32__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
