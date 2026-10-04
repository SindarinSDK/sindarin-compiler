SnArray *absent(void) { return NULL; }
SnArray *empty(void) {
    SnArray *value = sn_array_new(sizeof(unsigned char), 4);
    value->elem_tag = SN_TAG_BYTE;
    return value;
}
