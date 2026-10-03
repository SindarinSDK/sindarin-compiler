#include <stdint.h>
#include <stdlib.h>

/* unsigned char may read the object representation of every scalar type. */
char *byte_encoding_int_prefix(long long value, long long count) {
    const unsigned char *bytes = (const unsigned char *)&value;
    const char *digits = "0123456789abcdef";
    char *result = malloc((size_t)count * 2 + 1);
    for (long long i = 0; i < count; i++) {
        result[i * 2] = digits[bytes[i] >> 4];
        result[i * 2 + 1] = digits[bytes[i] & 15];
    }
    result[count * 2] = 0;
    return result;
}
