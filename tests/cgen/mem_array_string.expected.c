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
    sn_auto_arr SnArray * __sn__names = ({
            SnArray *__al__ = sn_array_new(sizeof(char *), 2);
            __al__->elem_tag = SN_TAG_STRING;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_str;
    
            __al__->elem_copy = sn_copy_str;
    
            sn_array_push(__al__, &(char *){ strdup("Alice") });
    
            sn_array_push(__al__, &(char *){ strdup("Bob") });
            __al__;
        });
    sn_assert((sn_array_length(__sn__names) == 2LL), "should have 2 names");
    
    fflush(stdout);
    return 0;
}
