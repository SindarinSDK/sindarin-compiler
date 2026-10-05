#include <stdarg.h>

bool record_mutate(__sn__VariadicRecord *left, __sn__VariadicRecord *right, ...) {
    va_list args;
    va_start(args, right);
    long long increment = va_arg(args, long long);
    char *text = va_arg(args, char *);
    va_end(args);
    bool same = left == right;
    left->__sn__number += increment;
    right->__sn__number += increment;
    free(left->__sn__text);
    left->__sn__text = strdup(text);
    return same;
}
__sn__VariadicRecord record_duplicate(__sn__VariadicRecord value, ...) {
    va_list args;
    va_start(args, value);
    long long increment = va_arg(args, long long);
    char *text = va_arg(args, char *);
    va_end(args);
    __sn__VariadicRecord result = {0};
    result.__sn__number = value.__sn__number + increment;
    result.__sn__text = strdup(text);
    /* The as-val parameter owns its string, independently of the caller. */
    free(value.__sn__text);
    return result;
}
