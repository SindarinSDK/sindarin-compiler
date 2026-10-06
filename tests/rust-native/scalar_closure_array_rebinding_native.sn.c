static long long alive;
__sn__Token *token_create(long long id) {
    __sn__Token *value = __sn__Token__new();
    value->__sn__id = id;
    alive++;
    return value;
}
void token_dispose(__sn__Token *value) { (void)value; alive--; }
long long token_refs(__sn__Token *value) { return value ? value->__rc__ : 0; }
long long token_alive(void) { return alive; }
bool array_same(SnArray *left, SnArray *right) { return left == right; }
