#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__factorial(long long, long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


long long __sn__factorial(long long __sn__n, long long __sn__acc) {

    if ((__sn__n <= 1LL)) {
        return __sn__acc;}

    return __sn__factorial(sn_sub_long(__sn__n, 1LL), sn_mul_long(__sn__acc, __sn__n));}

int main() {
    sn_assert((__sn__factorial(5LL, 1LL) == 120LL), "expected 5! to be 120");
    sn_assert((__sn__factorial(1LL, 1LL) == 1LL), "expected 1! to be 1");
    sn_assert((__sn__factorial(0LL, 1LL) == 1LL), "expected 0! to be 1");
    return 0;
}
