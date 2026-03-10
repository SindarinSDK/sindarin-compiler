#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    sn_auto_arr SnArray * __sn__arr = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 5);
            sn_array_push(__al__, &(long long){ 1LL });
            sn_array_push(__al__, &(long long){ 2LL });
            sn_array_push(__al__, &(long long){ 3LL });
            sn_array_push(__al__, &(long long){ 4LL });
            sn_array_push(__al__, &(long long){ 5LL });
            __al__;
        });
    sn_auto_arr SnArray * __sn__s = /* TODO: array slice for minimal runtime */NULL;
    return 0LL;
}
