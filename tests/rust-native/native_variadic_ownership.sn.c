#include <stdarg.h>

static char *fixed_text;
static char *tail_text;
void remember(char *label, ...) {
    va_list args;
    va_start(args, label);
    fixed_text = label;
    tail_text = va_arg(args, char *);
    va_end(args);
}
bool remembered(int32_t tag, ...) {
    return tag == 1 && strcmp(fixed_text, "fixed") == 0 && strcmp(tail_text, "tail") == 0;
}
char *owned_text(int32_t tag, ...) {
    va_list args;
    va_start(args, tag);
    char *value = va_arg(args, char *);
    char *result = strdup(tag == 2 ? value : "bad tag");
    va_end(args);
    return result;
}
SnArray *owned_bytes(int32_t tag, ...) {
    va_list args;
    va_start(args, tag);
    unsigned char tail = (unsigned char)va_arg(args, int);
    va_end(args);
    unsigned char values[] = { tag == 3 ? 0 : 1, 127, tail };
    SnArray *result = sn_array_new(sizeof(unsigned char), 3);
    result->elem_tag = SN_TAG_BYTE;
    for (size_t i = 0; i < 3; i++) sn_array_push(result, &values[i]);
    return result;
}
bool mutate_alias(long long *left, long long *right, char *first, char *second, ...) {
    va_list args;
    va_start(args, second);
    long long increment = va_arg(args, long long);
    int bits = va_arg(args, int);
    va_end(args);
    bool same = left == right && first == second;
    *left += increment;
    *right += increment;
    *first = 'B';
    *second = (char)bits;
    return same;
}
unsigned char char_bits(char value) { return (unsigned char)value; }
