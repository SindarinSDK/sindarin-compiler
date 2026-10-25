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

bool package_link_named(const char *value)
{
    return value && value[0] != '-' && !strchr(value, '/') && !strchr(value, '\\');
}

bool package_link_defer_named(const char *const *values, int count)
{
    for (int i = 0; values && i < count; i++) {
        size_t length = strlen(values[i]);
        if ((length > 2 && !strcmp(values[i] + length - 2, ".a")) ||
            (length > 4 && !strcmp(values[i] + length - 4, ".lib"))) return true;
    }
    return false;
}
