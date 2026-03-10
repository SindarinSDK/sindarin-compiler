#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__x = 99LL;
    long long __sn__result = ({
            long long __match_result__;
            long long __match_subject__ = __sn__x;
            if (__match_subject__ == 1LL) {
                __match_result__ = 10LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    return __sn__result;
}
