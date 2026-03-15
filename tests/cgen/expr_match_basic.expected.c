#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;

int main() {
    long long __sn__x = 2LL;
    long long __sn__result = ({
            long long __match_result__;
            long long __match_subject__ = __sn__x;
            if (__match_subject__ == 1LL) {
                __match_result__ = 10LL;
            } else if (__match_subject__ == 2LL) {
                __match_result__ = 20LL;
            } else if (__match_subject__ == 3LL) {
                __match_result__ = 30LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    sn_assert((__sn__result == 20LL), "expected match to return 20");
    
    return 0LL;    fflush(stdout);
}
