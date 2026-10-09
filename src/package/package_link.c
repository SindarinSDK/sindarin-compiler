#include "package_link.h"
#include <stdlib.h>
#include <string.h>
char *package_link_token(const char *value)
{
    if (!value || (!strchr(value, '/') && !strchr(value, '\\') && value[0] != '-')) return NULL;
    size_t length = strlen(value);
    char *token = malloc(length * 4 + 3);
    if (!token) return NULL;
    char *out = token;
#ifdef _WIN32
    *out++ = '"';
    for (const char *p = value; *p; p++) {
        if (*p == '"') *out++ = '"';
        *out++ = *p;
    }
    *out++ = '"';
#else
    *out++ = '\'';
    for (const char *p = value; *p; p++) {
        if (*p == '\'') { memcpy(out, "'\\''", 4); out += 4; }
        else *out++ = *p;
    }
    *out++ = '\'';
#endif
    *out = 0;
    return token;
}
