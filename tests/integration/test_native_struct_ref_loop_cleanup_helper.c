#include <stdlib.h>
#include <stdio.h>

__sn__Handle *handle_create(long long id) {
    __sn__Handle *h = calloc(1, sizeof(__sn__Handle));
    if (!h) { fprintf(stderr, "Handle: alloc failed\n"); exit(1); }
    h->__sn___id = id;
    h->__rc__ = 1;
    return h;
}
