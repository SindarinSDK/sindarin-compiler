#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: MetaInner (as val) */
typedef struct {
    long long __sn__value;
} __sn__MetaInner;
/* Value operations */
static inline __sn__MetaInner __sn__MetaInner_copy(const __sn__MetaInner *src) {
    __sn__MetaInner dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__MetaInner_cleanup(__sn__MetaInner *p) {

}

#define sn_auto_MetaInner __attribute__((cleanup(__sn__MetaInner_cleanup)))

static inline void __sn__MetaInner_cleanup_elem(void *p) { __sn__MetaInner_cleanup((__sn__MetaInner *)p); }
static inline void __sn__MetaInner_copy_into(const void *src, void *dst) { *(__sn__MetaInner *)dst = __sn__MetaInner_copy((const __sn__MetaInner *)src); }

/* Ref/pointer operations */
static inline __sn__MetaInner *__sn__MetaInner_alloc(void) {
    return calloc(1, sizeof(__sn__MetaInner));
}

static inline void __sn__MetaInner_release(__sn__MetaInner **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_MetaInner __attribute__((cleanup(__sn__MetaInner_release)))

static inline void __sn__MetaInner_release_elem(void *p) { __sn__MetaInner_release((__sn__MetaInner **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__MetaInner_to_string(const __sn__MetaInner *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "MetaInner { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: MetaOuter (as val) */
typedef struct {
    char * __sn__label;
    SnArray * __sn__items;
    __sn__MetaInner __sn__inner;
} __sn__MetaOuter;
/* Value operations */
static inline __sn__MetaOuter __sn__MetaOuter_copy(const __sn__MetaOuter *src) {
    __sn__MetaOuter dst;
    dst.__sn__label = src->__sn__label ? strdup(src->__sn__label) : NULL;
    dst.__sn__items = sn_array_copy(src->__sn__items);
    dst.__sn__inner = __sn__MetaInner_copy(&src->__sn__inner);
    return dst;
}

static inline void __sn__MetaOuter_cleanup(__sn__MetaOuter *p) {
    free(p->__sn__label);
    sn_cleanup_array(&p->__sn__items);

}

#define sn_auto_MetaOuter __attribute__((cleanup(__sn__MetaOuter_cleanup)))

static inline void __sn__MetaOuter_cleanup_elem(void *p) { __sn__MetaOuter_cleanup((__sn__MetaOuter *)p); }
static inline void __sn__MetaOuter_copy_into(const void *src, void *dst) { *(__sn__MetaOuter *)dst = __sn__MetaOuter_copy((const __sn__MetaOuter *)src); }

/* Ref/pointer operations */
static inline __sn__MetaOuter *__sn__MetaOuter_alloc(void) {
    return calloc(1, sizeof(__sn__MetaOuter));
}

static inline void __sn__MetaOuter_release(__sn__MetaOuter **p) {
    if (*p) {
        free((*p)->__sn__label);
        sn_cleanup_array(&(*p)->__sn__items);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_MetaOuter __attribute__((cleanup(__sn__MetaOuter_release)))

static inline void __sn__MetaOuter_release_elem(void *p) { __sn__MetaOuter_release((__sn__MetaOuter **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__MetaOuter_to_string(const __sn__MetaOuter *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "MetaOuter { ");
    off += snprintf(buf + off, sizeof(buf) - off, "label: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__label ? p->__sn__label : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "items: ");
    { char *__fs__ = sn_array_to_string(p->__sn__items); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "inner: ");
    { char *__fs__ = __sn__MetaInner_to_string(&p->__sn__inner); off += snprintf(buf + off, sizeof(buf) - off, "%s", __fs__); free(__fs__); }
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



__sn__TypeInfo __sn__inspect(__sn__MetaOuter *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


__sn__TypeInfo __sn__inspect(__sn__MetaOuter *__sn__outer) {

    return ({static __sn__FieldInfo __typeof_fields__[] = { { .__sn__name = (char *)"label", .__sn__typeName = (char *)"str", .__sn__typeId = 1112265104 }, { .__sn__name = (char *)"items", .__sn__typeName = (char *)"array", .__sn__typeId = 173583654 }, { .__sn__name = (char *)"inner", .__sn__typeName = (char *)"MetaInner", .__sn__typeId = 2125471480 } }; sn_typeinfo_create("MetaOuter", 64740043, __typeof_fields__, 3);});}



int main() {
    return 0LL;    fflush(stdout);
}
