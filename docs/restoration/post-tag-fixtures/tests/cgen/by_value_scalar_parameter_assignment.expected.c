#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: ScalarAssignments (as val) */
typedef struct {
    long long __sn__marker;
} __sn__ScalarAssignments;
/* Value operations */
static inline __sn__ScalarAssignments __sn__ScalarAssignments_copy(const __sn__ScalarAssignments *src) {
    __sn__ScalarAssignments dst;
    dst.__sn__marker = src->__sn__marker;
    return dst;
}

static inline void __sn__ScalarAssignments_cleanup(__sn__ScalarAssignments *p) {

}

#define sn_auto_ScalarAssignments __attribute__((cleanup(__sn__ScalarAssignments_cleanup)))

static inline void __sn__ScalarAssignments_cleanup_elem(void *p) { __sn__ScalarAssignments_cleanup((__sn__ScalarAssignments *)p); }
static inline void __sn__ScalarAssignments_copy_into(const void *src, void *dst) { *(__sn__ScalarAssignments *)dst = __sn__ScalarAssignments_copy((const __sn__ScalarAssignments *)src); }

/* Ref/pointer operations */
static inline __sn__ScalarAssignments *__sn__ScalarAssignments_alloc(void) {
    return calloc(1, sizeof(__sn__ScalarAssignments));
}

static inline void __sn__ScalarAssignments_release(__sn__ScalarAssignments **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_ScalarAssignments __attribute__((cleanup(__sn__ScalarAssignments_release)))

static inline void __sn__ScalarAssignments_release_elem(void *p) { __sn__ScalarAssignments_release((__sn__ScalarAssignments **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__ScalarAssignments_to_string(const __sn__ScalarAssignments *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "ScalarAssignments { ");
    off += snprintf(buf + off, sizeof(buf) - off, "marker: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__marker);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__observeInt(long long *, long long);
bool __sn__assignBool(bool, bool);
long long __sn__assignInt(long long, long long *, long long);
long long __sn__assignLong(long long, long long);
long long __sn__helperNames(long long, long long, long long);
long long __sn__statementOrder(long long, long long);
int32_t __sn__ScalarAssignments_assignInt32(int32_t, int32_t);
unsigned char __sn__ScalarAssignments_assignByte(unsigned char, unsigned char);
uint32_t __sn__ScalarAssignments_assignUint32(uint32_t, uint32_t);
uint64_t __sn__ScalarAssignments_assignUint(__sn__ScalarAssignments *, uint64_t, uint64_t);
float __sn__ScalarAssignments_assignFloat(__sn__ScalarAssignments *, float, float);
double __sn__ScalarAssignments_assignDouble(__sn__ScalarAssignments *, double, double);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__observeInt(long long *__sn__calls, long long __sn__value) {

    (*__sn__calls = sn_add_long((*__sn__calls), 1LL));
    

    return __sn__value;}


bool __sn__assignBool(bool __sn__value, bool __sn__untouched) {

    (__sn__value = (!__sn__value));
    

    return (__sn__value && __sn__untouched);}


long long __sn__assignInt(long long __sn__value, long long *__sn__calls, long long __sn__untouched) {

    (__sn__value = __sn__observeInt(&(*__sn__calls), sn_add_long(__sn__value, 2LL)));
    

    return sn_add_long(__sn__value, __sn__untouched);}


long long __sn__assignLong(long long __sn__value, long long __sn__untouched) {

    long long __sn__assigned = (__sn__value = sn_add_long(__sn__value, 3LL));

    return sn_add_long(sn_add_long(__sn__assigned, __sn__value), __sn__untouched);}


long long __sn__helperNames(long long __sn____sn_rhs, long long __sn____sn_place, long long __sn____sn_next) {

    long long __sn__assigned = (__sn____sn_rhs = sn_add_long(__sn____sn_rhs, __sn____sn_place));

    if (true) {
        long long __sn____sn_next = 4LL;
        (__sn____sn_next = 5LL);
        
    }

    return sn_add_long(sn_add_long(__sn__assigned, __sn____sn_rhs), __sn____sn_next);}


long long __sn__statementOrder(long long __sn__value, long long __sn__delta) {

    if (true) {
        long long __sn__readBefore = sn_add_long(__sn__value, __sn__delta);
        (__sn__value = sn_add_long(__sn__value, 1LL));
        
        long long __sn__value = __sn__readBefore;
        (__sn__value = sn_add_long(__sn__value, 10LL));
        
    }

    return __sn__value;}


int32_t __sn__ScalarAssignments_assignInt32(int32_t __sn__value, int32_t __sn__untouched) {

    (__sn__value = sn_add_int32(__sn__value, 4LL));
    

    return sn_add_int32(__sn__value, __sn__untouched);}

unsigned char __sn__ScalarAssignments_assignByte(unsigned char __sn__value, unsigned char __sn__untouched) {

    unsigned char __sn__assigned = (__sn__value = sn_add_byte(__sn__value, (unsigned char)5));

    return sn_add_byte(sn_add_byte(__sn__assigned, __sn__value), __sn__untouched);}

uint32_t __sn__ScalarAssignments_assignUint32(uint32_t __sn__value, uint32_t __sn__untouched) {

    return (__sn__value = sn_add_uint32(__sn__value, __sn__untouched));}

uint64_t __sn__ScalarAssignments_assignUint(__sn__ScalarAssignments *__sn__self, uint64_t __sn__value, uint64_t __sn__untouched) {

    (__sn__value = sn_add_uint(__sn__value, 7LL));
    

    return sn_add_uint(sn_add_uint(__sn__value, __sn__untouched), __sn__int_toUint(__sn__self->__sn__marker));}

float __sn__ScalarAssignments_assignFloat(__sn__ScalarAssignments *__sn__self, float __sn__value, float __sn__untouched) {

    float __sn__assigned = (__sn__value = sn_add_float(__sn__value, 1.5f));

    return sn_add_float(sn_add_float(__sn__assigned, __sn__value), __sn__untouched);}

double __sn__ScalarAssignments_assignDouble(__sn__ScalarAssignments *__sn__self, double __sn__value, double __sn__untouched) {

    return (__sn__value = sn_add_double(__sn__value, __sn__untouched));}

int main() {
    bool __sn__boolCaller = false;
    long long __sn__intCaller = 10LL;
    long long __sn__longCaller = 20LL;
    int32_t __sn__int32Caller = 30LL;
    unsigned char __sn__byteCaller = (unsigned char)40;
    uint32_t __sn__uint32Caller = 50LL;
    uint64_t __sn__uintCaller = 60LL;
    float __sn__floatCaller = 2.0f;
    double __sn__doubleCaller = 3.0;
    long long __sn__calls = 0LL;
    __sn__ScalarAssignments __sn__ops = (__sn__ScalarAssignments){ .__sn__marker = 1LL };
    bool __sn__boolResult = __sn__assignBool(__sn__boolCaller, true);
    long long __sn__intResult = __sn__assignInt(__sn__intCaller, &__sn__calls, 1LL);
    long long __sn__longResult = __sn__assignLong(__sn__longCaller, 1LL);
    int32_t __sn__int32Result = __sn__ScalarAssignments_assignInt32(__sn__int32Caller, 1LL);
    unsigned char __sn__byteResult = __sn__ScalarAssignments_assignByte(__sn__byteCaller, (unsigned char)1);
    uint32_t __sn__uint32Result = __sn__ScalarAssignments_assignUint32(__sn__uint32Caller, 2LL);
    uint64_t __sn__uintResult = __sn__ScalarAssignments_assignUint(&__sn__ops, __sn__uintCaller, 2LL);
    float __sn__floatResult = __sn__ScalarAssignments_assignFloat(&__sn__ops, __sn__floatCaller, 0.5f);
    double __sn__doubleResult = __sn__ScalarAssignments_assignDouble(&__sn__ops, __sn__doubleCaller, 0.25);
    long long __sn__helperResult = __sn__helperNames(1LL, 2LL, 3LL);
    long long __sn__orderCaller = 4LL;
    long long __sn__orderResult = __sn__statementOrder(__sn__orderCaller, 2LL);
    printf("%s\n", ((__sn__boolResult && (!__sn__boolCaller))) ? "true" : "false");
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__intResult));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__intCaller));
            sn_auto_str char *__is_p3__ = sn_strdup("/");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__calls));
            sn_str_concat_multi(5, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__longResult));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__longCaller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__int32Result));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__int32Caller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%u", (unsigned)(__sn__byteResult));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%u", (unsigned)(__sn__byteCaller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__uint32Result));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__uint32Caller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__uintResult));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__uintCaller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    printf("%s\n", (((__sn__floatResult == 7.5f) && (__sn__floatCaller == 2.0f))) ? "true" : "false");
    
    printf("%s\n", (((__sn__doubleResult == 3.25) && (__sn__doubleCaller == 3.0))) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__helperResult));
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__orderResult));
            sn_auto_str char *__is_p1__ = sn_strdup("/");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__orderCaller));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
