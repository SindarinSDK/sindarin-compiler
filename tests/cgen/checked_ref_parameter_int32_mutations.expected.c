#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Box (as val) */
typedef struct {
    int32_t __sn__value;
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



int32_t __sn__oldThenIncrement(int32_t *);
int32_t __sn__freeOps(int32_t *);
int32_t __sn__forwardStatic(int32_t *);
int32_t __sn__forwardInstance(int32_t *);
int32_t __sn__maxBoundary(int32_t *);
int32_t __sn__minBoundary(int32_t *);
int32_t __sn__read(int32_t *);
void __sn__freeBump(int32_t *);
int32_t __sn__RefOps_staticOps(int32_t *);
void __sn__RefOps_staticBump(int32_t *);
int32_t __sn__RefOps_instancePostfix(__sn__RefOps *, int32_t *);
void __sn__RefOps_instanceBump(__sn__RefOps *, int32_t *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


int32_t __sn__oldThenIncrement(int32_t *__sn__value) {

    return ({
         int32_t *__sn_place__ = &((*__sn__value));
         int32_t __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_add_int32(__sn_previous__, 1);
         __sn_previous__;
     });}


int32_t __sn__freeOps(int32_t *__sn__value) {

    int32_t __sn__add = ({
        int32_t __sn_rhs__ = __sn__oldThenIncrement(&(*__sn__value));
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__sub = ({
        int32_t __sn_rhs__ = 1LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__mul = ({
        int32_t __sn_rhs__ = 2LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__div = ({
        int32_t __sn_rhs__ = 2LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__rem = ({
        int32_t __sn_rhs__ = 3LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__old_inc = ({
        int32_t *__sn_place__ = &((*__sn__value));
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_int32(__sn_previous__, 1);
        __sn_previous__;
    });

    int32_t __sn__old_dec = ({
        int32_t *__sn_place__ = &((*__sn__value));
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_int32(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}


int32_t __sn__forwardStatic(int32_t *__sn__value) {

    __sn__RefOps_staticBump(&(*__sn__value));
    

    return (*__sn__value);}


int32_t __sn__forwardInstance(int32_t *__sn__value) {

    __sn__RefOps __sn__ops = (__sn__RefOps){  };

    __sn__RefOps_instanceBump(&__sn__ops, &(*__sn__value));
    

    return (*__sn__value);}


int32_t __sn__maxBoundary(int32_t *__sn__value) {

    return ({
         int32_t __sn_rhs__ = 1LL;
         int32_t *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}


int32_t __sn__minBoundary(int32_t *__sn__value) {

    return ({
         int32_t __sn_rhs__ = 1LL;
         int32_t *__sn_place__ = &((*__sn__value));
         *__sn_place__ = sn_sub_int32(*__sn_place__, __sn_rhs__);
         *__sn_place__;
     });}


int32_t __sn__read(int32_t *__sn__value) {

    return (*__sn__value);}


void __sn__freeBump(int32_t *__sn__value) {

    ({
        int32_t __sn_rhs__ = 1LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}




int32_t __sn__RefOps_staticOps(int32_t *__sn__value) {

    int32_t __sn__add = ({
        int32_t __sn_rhs__ = 3LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__sub = ({
        int32_t __sn_rhs__ = 1LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_sub_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__mul = ({
        int32_t __sn_rhs__ = 2LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mul_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__div = ({
        int32_t __sn_rhs__ = 2LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_div_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__rem = ({
        int32_t __sn_rhs__ = 3LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_mod_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });

    int32_t __sn__old_inc = ({
        int32_t *__sn_place__ = &((*__sn__value));
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_int32(__sn_previous__, 1);
        __sn_previous__;
    });

    int32_t __sn__old_dec = ({
        int32_t *__sn_place__ = &((*__sn__value));
        int32_t __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_sub_int32(__sn_previous__, 1);
        __sn_previous__;
    });

    return sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(sn_add_int32(__sn__add, __sn__sub), __sn__mul), __sn__div), __sn__rem), __sn__old_inc), __sn__old_dec), (*__sn__value));}

void __sn__RefOps_staticBump(int32_t *__sn__value) {

    ({
        int32_t __sn_rhs__ = 1LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}

int32_t __sn__RefOps_instancePostfix(__sn__RefOps *__sn__self, int32_t *__sn__value) {

    return ({
         int32_t *__sn_place__ = &((*__sn__value));
         int32_t __sn_previous__ = *__sn_place__;
         *__sn_place__ = sn_sub_int32(__sn_previous__, 1);
         __sn_previous__;
     });}

void __sn__RefOps_instanceBump(__sn__RefOps *__sn__self, int32_t *__sn__value) {

    ({
        int32_t __sn_rhs__ = 1LL;
        int32_t *__sn_place__ = &((*__sn__value));
        *__sn_place__ = sn_add_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
}

int main() {
    int32_t __sn__free_value = 2LL;
    int32_t __sn__static_value = 10LL;
    int32_t __sn__postfix_value = 5LL;
    int32_t __sn__forwarded_static = 1LL;
    int32_t __sn__forwarded_instance = 1LL;
    int32_t __sn__maximum = 2147483646LL;
    int32_t __sn__minimum = (-2147483647LL);
    __sn__Box __sn__direct = (__sn__Box){ .__sn__value = 1LL };
    __sn__Wrapper __sn__nested = (__sn__Wrapper){ .__sn__box = (__sn__Box){ .__sn__value = 10LL } };
    __sn__RefOps __sn__ops = (__sn__RefOps){  };
    printf("%lld\n", (long long)(__sn__freeOps(&__sn__free_value)));
    
    printf("%lld\n", (long long)(__sn__free_value));
    
    printf("%lld\n", (long long)(__sn__RefOps_staticOps(&__sn__static_value)));
    
    printf("%lld\n", (long long)(__sn__static_value));
    
    printf("%lld\n", (long long)(__sn__RefOps_instancePostfix(&__sn__ops, &__sn__postfix_value)));
    
    printf("%lld\n", (long long)(__sn__postfix_value));
    
    printf("%lld\n", (long long)(__sn__forwardStatic(&__sn__forwarded_static)));
    
    printf("%lld\n", (long long)(__sn__forwarded_static));
    
    printf("%lld\n", (long long)(__sn__forwardInstance(&__sn__forwarded_instance)));
    
    printf("%lld\n", (long long)(__sn__forwarded_instance));
    
    printf("%lld\n", (long long)(__sn__maxBoundary(&__sn__maximum)));
    
    printf("%lld\n", (long long)(__sn__minBoundary(&__sn__minimum)));
    
    __sn__freeBump(&__sn__direct.__sn__value);
    
    __sn__freeBump(&__sn__nested.__sn__box.__sn__value);
    
    __sn__RefOps_staticBump(&__sn__direct.__sn__value);
    
    __sn__RefOps_staticBump(&__sn__nested.__sn__box.__sn__value);
    
    __sn__RefOps_instanceBump(&__sn__ops, &__sn__direct.__sn__value);
    
    __sn__RefOps_instanceBump(&__sn__ops, &__sn__nested.__sn__box.__sn__value);
    
    printf("%lld\n", (long long)(__sn__read(&__sn__direct.__sn__value)));
    
    printf("%lld\n", (long long)(__sn__direct.__sn__value));
    
    printf("%lld\n", (long long)(__sn__nested.__sn__box.__sn__value));
    
    fflush(stdout);
    return 0;
}
