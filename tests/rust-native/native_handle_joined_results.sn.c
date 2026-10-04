static long long g_token_alive;
__sn__Token *token_create(long long id) {
    __sn__Token *value = __sn__Token__new();
    value->__sn__id = id;
    g_token_alive++;
    return value;
}
void token_dispose(__sn__Token *value) { (void)value; g_token_alive--; }
long long token_refs(__sn__Token *value) { return value ? value->__rc__ : 0; }
long long token_alive(void) { return g_token_alive; }
