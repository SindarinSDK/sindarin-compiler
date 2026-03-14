#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__make_greeting(char *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


char * __sn__make_greeting(char * __sn__name) {

    sn_auto_str char * __sn__result = ({
            char __is_buf__[1024];
            int __is_off__ = 0;
            __is_buf__[0] = '\0';
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "%s", "Hello ");
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "%s", __sn__name);
            strdup(__is_buf__);
        });

    {
        char * __ret__ = __sn__result;
        __sn__result = NULL;
        return __ret__;
    }}

int main() {
    sn_auto_str char * __sn__g = __sn__make_greeting("world");
    sn_assert((sn_str_length(__sn__g) == 11LL), "greeting should be 11 chars");
    fflush(stdout);
    return 0;
}
