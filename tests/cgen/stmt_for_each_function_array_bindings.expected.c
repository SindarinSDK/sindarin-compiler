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
    sn_auto_arr SnArray * __sn__callbacks = ({
            SnArray *__al__ = sn_array_new(sizeof(void *), 0);
    
            __al__->elem_release = sn_release_closure_elem;
    
            __al__->elem_copy = sn_copy_closure;
            __al__;
        });
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__callbacks);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            void * __sn__callback__source = ((void * *)__arr_0__->data)[__idx_0__];
            sn_auto_fn void * __sn__callback = sn_closure_retain(__sn__callback__source);
            {
                ({
                    void *__old_cl__ = __sn__callback;
                    __sn__callback = sn_closure_retain(__sn__callback);
                    sn_closure_release(&__old_cl__);
                    __sn__callback;
                });
                
            }
        }
    }
    fflush(stdout);
    return 0;
}
