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
    int __rc__;
} __Closure__;

int main() {
    sn_auto_str char * __sn__source = strdup("one");
    sn_auto_arr SnArray * __sn__names = __sn___split(&__sn__source, ",");
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__names);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            char * __sn__name__source = ((char * *)__arr_0__->data)[__idx_0__];
            sn_auto_str char * __sn__name = __sn__name__source ? strdup(__sn__name__source) : NULL;
            {
                ({
                    char *__sn_tmp__ = strdup("changed");
                    free(__sn__name);
                    __sn__name = __sn_tmp__;
                    __sn__name;
                });
                
            }
        }
    }
    fflush(stdout);
    return 0;
}
