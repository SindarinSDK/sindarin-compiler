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
    sn_auto_arr SnArray * __sn__arr = ({
            SnArray *__sa__ = sn_array_new(sizeof(long long), 10LL);
            __sa__->elem_tag = SN_TAG_INT;
    
            for (long long __si__ = 0; __si__ < (long long)(10LL); __si__++) {
                sn_array_push(__sa__, &(long long){ 0 });
            }
            __sa__;
        });
    return 0LL;    fflush(stdout);
}
