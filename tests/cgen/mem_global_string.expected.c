#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__greeting = NULL;

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;

int main() {
    __sn__greeting = strdup("hello");
    sn_assert((sn_str_length(__sn__greeting) == 5LL), "greeting should be 5 chars");
    
    free(__sn__greeting);
    fflush(stdout);
    return 0;
}
