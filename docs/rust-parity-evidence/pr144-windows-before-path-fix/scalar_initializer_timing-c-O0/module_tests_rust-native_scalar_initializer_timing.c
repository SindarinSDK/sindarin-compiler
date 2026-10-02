#include "sn_types.h"

long long __sn__helperSeed = 0;



long long __sn__initialValue() {

    sn_println("initializer");
    

    return 40LL;}


long long __sn__helper(long long __sn__value) {

    return sn_add_long(__sn__value, __sn__helperSeed);}

long long throughNative(long long __sn__value) {

    return __sn__helper(__sn__value);}

