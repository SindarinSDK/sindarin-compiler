#include <stdlib.h>
static long long native_handle_live;
__sn__Token *__sn__Token_create(long long id) {
    __sn__Token *value = __sn__Token__new();
    value->__sn__id = id;
    native_handle_live++;
    return value;
}
void __sn__Token_dispose(__sn__Token *value) {
    (void)value;
    native_handle_live--;
}
__sn__Token *__sn__Token_borrow(__sn__Token *value) { return value; }
__sn__Token *__sn__Token_pick(__sn__Token *value, __sn__Token *other) {
    (void)value;
    return other;
}
long long __sn__Token_refs(__sn__Token *value) { return value ? value->__rc__ : 0; }
long long __sn__Token_live(void) { return native_handle_live; }
