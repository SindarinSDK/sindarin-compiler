char *owned_bytes(char *label) {
    static const unsigned char result_bytes[] = { 'A', 0xff, 'B', 0 };
    if (!label || strcmp(label, "request") != 0) return strdup("bad request");
    char *result = malloc(sizeof(result_bytes));
    memcpy(result, result_bytes, sizeof(result_bytes));
    return result;
}

SnArray *owned_array(void) {
    static const unsigned char values[] = { 0, 127, 255 };
    SnArray *result = sn_array_new(sizeof(unsigned char), 3);
    result->elem_tag = SN_TAG_BYTE;
    for (size_t i = 0; i < sizeof(values); i++) sn_array_push(result, &values[i]);
    return result;
}

void mutate_values(long long *number, double *fraction, char *letter, bool *flag) {
    *number = 9;
    *fraction = 4.0;
    *letter = (char)0xff;
    *flag = true;
}

unsigned char char_bits(char value) { return (unsigned char)value; }
