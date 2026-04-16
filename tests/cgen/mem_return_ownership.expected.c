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
            sn_auto_str char *__is_p0__ = sn_strdup("Hello ");
            sn_auto_str char *__is_p1__ = sn_strdup(__sn__name);
            sn_str_concat_multi(2, __is_p0__, __is_p1__);
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
