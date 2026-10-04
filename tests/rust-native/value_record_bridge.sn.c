__sn__Record make_record(void) {
    return (__sn__Record) {
        .__sn__leaf = { .__sn__mark = (char)0xff, .__sn__small = -123,
                       .__sn__flag = true, .__sn__fraction = 1.5f },
        .__sn__count = 7000000000LL, .__sn__wide = -8000000000LL,
        .__sn__natural = 9000000000ULL, .__sn__unsigned_small = 4000000000U,
        .__sn__octet = 254, .__sn__fraction = 2.5
    };
}

__sn__Record update_record(__sn__Record value) {
    value.__sn__count += 10;
    value.__sn__leaf.__sn__mark = (char)0x80;
    value.__sn__leaf.__sn__flag = false;
    return value;
}

__sn__Leaf leaf_identity(__sn__Leaf value) { return value; }
__sn__Leaf make_leaf(unsigned char value) {
    return (__sn__Leaf) { .__sn__mark = (char)value, .__sn__small = 37,
                         .__sn__flag = true, .__sn__fraction = 1.5f };
}

char *record_label(__sn__Record value, const char *label, char *left, char *right) {
    *left = (char)0xfe;
    *right = (char)((unsigned char)*left + 1);
    return sn_strdup(value.__sn__count == 7000000010LL ? label : "bad record");
}

SnArray *record_bytes(__sn__Record value) {
    SnArray *result = sn_array_new(sizeof(unsigned char), 2);
    result->elem_tag = SN_TAG_BYTE;
    unsigned char first = (unsigned char)value.__sn__leaf.__sn__mark;
    unsigned char second = value.__sn__octet;
    sn_array_push(result, &first);
    sn_array_push(result, &second);
    return result;
}

unsigned char char_bits(char value) { return (unsigned char)value; }
