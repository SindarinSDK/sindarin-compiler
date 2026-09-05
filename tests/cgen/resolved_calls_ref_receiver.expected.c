#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Item (as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn__value;
} __sn__Item;



static inline __sn__Item *__sn__Item__new(void) {
    __sn__Item *p = calloc(1, sizeof(__sn__Item));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Item *__sn__Item_retain(__sn__Item *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Item_release(__sn__Item **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Item *__sn__Item_copy(const __sn__Item *src) {
    __sn__Item *dst = calloc(1, sizeof(__sn__Item));
    dst->__rc__ = 1;
    dst->__sn__value = src->__sn__value;
    return dst;
}

#define sn_auto_Item __attribute__((cleanup(__sn__Item_release)))
#define sn_auto_ref_Item __attribute__((cleanup(__sn__Item_release)))

static inline void __sn__Item_release_elem(void *p) { __sn__Item_release((__sn__Item **)p); }
static inline void __sn__Item_retain_into(const void *src, void *dst) { *(__sn__Item **)dst = __sn__Item_retain(*(__sn__Item *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Item_to_string(const __sn__Item *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Item { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}

__sn__Item * __sn__makeItem(long long *, long long);
bool __sn__Item_op_lt(__sn__Item *, __sn__Item *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


__sn__Item * __sn__makeItem(long long *__sn__calls, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return ({
         __sn__Item *__tmp__ = __sn__Item__new();
         __tmp__->__sn__value = __sn__value;
         __tmp__;
     });}


bool __sn__Item_op_lt(__sn__Item *__sn__self, __sn__Item * __sn__other) {

    return sn_lt_long(__sn__self->__sn__value, __sn__other->__sn__value);}

int main() {
    long long __sn__calls = 0LL;
    sn_auto_Item __sn__Item * __sn__low = ({
        __sn__Item *__tmp__ = __sn__Item__new();
        __tmp__->__sn__value = 1LL;
        __tmp__;
    });
    sn_auto_Item __sn__Item * __sn__high = ({
        __sn__Item *__tmp__ = __sn__Item__new();
        __tmp__->__sn__value = 2LL;
        __tmp__;
    });
    printf("%s\n", (__sn__Item_op_lt(__sn__low, __sn__high)) ? "true" : "false");
    
    printf("%s\n", (({ __sn__Item * __sn_resolved_source_arg__ = __sn__low; __sn__Item * __sn_resolved_source_receiver__ = __sn__high; __sn__Item_op_lt(__sn_resolved_source_receiver__, __sn_resolved_source_arg__); })) ? "true" : "false");
    
    printf("%s\n", ((!({ __sn__Item * __sn_resolved_source_arg__ = __sn__low; __sn__Item * __sn_resolved_source_receiver__ = __sn__high; __sn__Item_op_lt(__sn_resolved_source_receiver__, __sn_resolved_source_arg__); }))) ? "true" : "false");
    
    printf("%s\n", (({ __sn__Item * __sn_resolved_source_arg__ = __sn__low; sn_auto_Item __sn__Item * __sn____chain_tmp_0 = __sn__makeItem(&__sn__calls, 2LL);
    __sn__Item * __sn_resolved_source_receiver__ = __sn____chain_tmp_0; __sn__Item_op_lt(__sn_resolved_source_receiver__, __sn_resolved_source_arg__); })) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__calls));
    
    fflush(stdout);
    return 0;
}
