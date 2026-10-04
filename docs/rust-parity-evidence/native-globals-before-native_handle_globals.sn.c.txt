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
long long array_append(SnArray *values, long long id) {
    __sn__Token *value = token_create(id);
    sn_array_push(values, &value);
    return values->len;
}
bool array_same(SnArray *left, SnArray *right) { return left == right; }
SnArray *array_copy(SnArray *values) { return sn_array_copy(values); }
SnArray *array_nil(void) { return NULL; }
long long token_after(__sn__Token *value, long long ignored) { (void)ignored; return value->__sn__id; }
long long array_count(SnArray *values) { return values ? values->len : 0; }
SnArray *array_owned(long long id) {
    SnArray *values = sn_array_new(sizeof(__sn__Token *), 1);
    values->elem_release = __sn__Token_release_elem;
    values->elem_copy = __sn__Token_retain_into;
    values->elem_tag = SN_TAG_STRUCT;
    __sn__Token *value = token_create(id);
    sn_array_push(values, &value);
    return values;
}

long long array_metadata(SnArray *values) {
    return (values->elem_size == sizeof(long long) ? 1 : 0)
         + (!values->elem_copy ? 2 : 0)
         + (!values->elem_release ? 4 : 0)
         + (values->elem_tag == SN_TAG_DEFAULT ? 8 : 0);
}

static long long g_copy_count;
static void tracked_copy(const void *src, void *dst) {
    g_copy_count++;
    __sn__Token_retain_into(src, dst);
}
void enable_copy_counter(SnArray *values) { values->elem_copy = tracked_copy; }
long long copy_count(void) { return g_copy_count; }
