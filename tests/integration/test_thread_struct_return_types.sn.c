/* Native function for Section D: native struct return via thread wrapper */

__sn__Metrics raw_metrics(long long n) {
    __sn__Metrics m;
    m.__sn__count = n;
    m.__sn__total = (double)n * 1.5;
    return m;
}
