#define DEFINE_MUTATOR(name, type, first, added) \
    void name(SnArray *values) { \
        ((type *)values->data)[0] = (first); \
        type next = (added); \
        sn_array_push(values, &next); \
    }

#define DEFINE_RESULT(name, type, tag, first, second) \
    SnArray *name(void) { \
        SnArray *values = sn_array_new(sizeof(type), 2); \
        values->elem_tag = (tag); \
        type a = (first); \
        type b = (second); \
        sn_array_push(values, &a); \
        sn_array_push(values, &b); \
        return values; \
    }

DEFINE_MUTATOR(mutate_int, long long, 11, 12)
DEFINE_MUTATOR(mutate_long, long long, 21, 22)
DEFINE_MUTATOR(mutate_int32, int32_t, 31, 32)
DEFINE_MUTATOR(mutate_uint, uint64_t, 41, 42)
DEFINE_MUTATOR(mutate_uint32, uint32_t, 51, 52)
DEFINE_MUTATOR(mutate_byte, unsigned char, 71, 72)
DEFINE_MUTATOR(mutate_bool, bool, true, true)
DEFINE_MUTATOR(mutate_char, char, 'x', 'z')
DEFINE_MUTATOR(mutate_float, float, 6.5f, 7.5f)
DEFINE_MUTATOR(mutate_double, double, 8.5, 9.5)
DEFINE_MUTATOR(mutate_int_as_val, long long, 61, 62)

DEFINE_RESULT(make_int, long long, SN_TAG_INT, 101, 102)
DEFINE_RESULT(make_long, long long, SN_TAG_INT, 201, 202)
DEFINE_RESULT(make_int32, int32_t, SN_TAG_DEFAULT, 301, 302)
DEFINE_RESULT(make_uint, uint64_t, SN_TAG_DEFAULT, 401, 402)
DEFINE_RESULT(make_uint32, uint32_t, SN_TAG_DEFAULT, 501, 502)
DEFINE_RESULT(make_bool, bool, SN_TAG_BOOL, true, false)
DEFINE_RESULT(make_char, char, SN_TAG_CHAR, 'q', 'r')
DEFINE_RESULT(make_float, float, SN_TAG_DOUBLE, 11.5f, 12.5f)
DEFINE_RESULT(make_double, double, SN_TAG_DOUBLE, 21.5, 22.5)

long long mutate_ordered(SnArray *values, long long marker) {
    ((long long *)values->data)[0] += marker;
    return ((long long *)values->data)[0];
}

long long observe_empty(SnArray *values) { return values->len; }

SnArray *make_empty_int(void) {
    SnArray *values = sn_array_new(sizeof(long long), 0);
    values->elem_tag = SN_TAG_INT;
    return values;
}
