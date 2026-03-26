#include <stdio.h>
#include <stdlib.h>

/* Simulates a heap-allocated native as-ref struct (like TcpStream). */

__sn__Resource *resource_create(long long id) {
    __sn__Resource *r = calloc(1, sizeof(__sn__Resource));
    r->__sn___handle = 100 + id;
    return r;
}

long long resource_read_silent(__sn__Resource *self) {
    /* Access struct field — triggers ASAN if struct was freed */
    return self->__sn___handle;
}

void resource_dispose(__sn__Resource *self) {
    (void)self;
}
