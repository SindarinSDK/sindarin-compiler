/* Native function for error test: direct thread spawn of native fn */

__sn__NativeResult compute_native(long long n) {
    __sn__NativeResult m;
    m.__sn__count = n;
    m.__sn__total = (double)n * 1.5;
    return m;
}
