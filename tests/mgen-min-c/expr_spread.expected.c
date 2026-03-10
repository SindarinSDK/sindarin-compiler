#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    sn_auto_arr SnArray * __sn__a = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 3);
            sn_array_push(__al__, &(long long){ 1LL });
            sn_array_push(__al__, &(long long){ 2LL });
            sn_array_push(__al__, &(long long){ 3LL });
            __al__;
        });
    sn_auto_arr SnArray * __sn__b = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 3);
            /* TODO: spread into minimal array */
            sn_array_push(__al__, &(long long){ 4LL });
            sn_array_push(__al__, &(long long){ 5LL });
            __al__;
        });
    return 0LL;
}
