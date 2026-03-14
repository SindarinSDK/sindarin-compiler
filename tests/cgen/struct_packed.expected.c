#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: PackedData (as val) */
#pragma pack(push, 1)
typedef struct {
    unsigned char __sn__flags;
    long long __sn__value;
} __sn__PackedData;
#pragma pack(pop)
/* Value operations */
static inline __sn__PackedData __sn__PackedData_copy(const __sn__PackedData *src) {
    __sn__PackedData dst;
    dst.__sn__flags = src->__sn__flags;
    dst.__sn__value = src->__sn__value;
    return dst;
}

static inline void __sn__PackedData_cleanup(__sn__PackedData *p) {

}

#define sn_auto_PackedData __attribute__((cleanup(__sn__PackedData_cleanup)))

static inline void __sn__PackedData_cleanup_elem(void *p) { __sn__PackedData_cleanup((__sn__PackedData *)p); }
static inline void __sn__PackedData_copy_into(const void *src, void *dst) { *(__sn__PackedData *)dst = __sn__PackedData_copy((const __sn__PackedData *)src); }

/* Ref/pointer operations */
static inline __sn__PackedData *__sn__PackedData_alloc(void) {
    return calloc(1, sizeof(__sn__PackedData));
}

static inline void __sn__PackedData_release(__sn__PackedData **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_PackedData __attribute__((cleanup(__sn__PackedData_release)))

static inline void __sn__PackedData_release_elem(void *p) { __sn__PackedData_release((__sn__PackedData **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__PackedData_to_string(const __sn__PackedData *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "PackedData { ");
    off += snprintf(buf + off, sizeof(buf) - off, "flags: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%u", (unsigned)p->__sn__flags);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


int main() {
    return 0LL;    fflush(stdout);
}
