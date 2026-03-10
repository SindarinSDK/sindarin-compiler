#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
char * __sn__greet(char *);

char * __sn__greet(char * __sn__name) {
    char * __sn__msg = ({
            char __is_buf__[1024];
            int __is_off__ = 0;
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "Hello ");
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "%s", __sn__name);
            strdup(__is_buf__);
        });
    return __sn__msg;
}

int main() {
    char * __sn__g = __sn__greet("world");
    sn_assert((sn_str_length(__sn__g) == 11LL), "expected greeting length to be 11");
    return 0;
}
