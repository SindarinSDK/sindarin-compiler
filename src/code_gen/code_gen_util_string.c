#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *arena_vsprintf(Arena *arena, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (size < 0)
    {
        exit(1);
    }
    char *buf = arena_alloc(arena, size + 1);
    if (buf == NULL)
    {
        exit(1);
    }
    vsnprintf(buf, size + 1, fmt, args);
    return buf;
}

char *arena_sprintf(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *result = arena_vsprintf(arena, fmt, args);
    va_end(args);
    return result;
}

char *escape_char_literal(Arena *arena, char ch)
{
    DEBUG_VERBOSE("Entering escape_char_literal");
    char buf[16];
    if (ch == '\'') {
        sprintf(buf, "'\\''");
    } else if (ch == '\\') {
        sprintf(buf, "'\\\\'");
    } else if (ch == '\n') {
        sprintf(buf, "'\\n'");
    } else if (ch == '\t') {
        sprintf(buf, "'\\t'");
    } else if (ch == '\r') {
        sprintf(buf, "'\\r'");
    } else if (ch == '\0') {
        sprintf(buf, "'\\0'");
    } else if ((unsigned char)ch >= 0x80 || ch < 0) {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else if (ch < ' ' || ch > '~') {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else {
        sprintf(buf, "'%c'", ch);
    }
    return arena_strdup(arena, buf);
}

char *escape_c_string(Arena *arena, const char *str)
{
    DEBUG_VERBOSE("Entering escape_c_string");
    if (str == NULL)
    {
        return arena_strdup(arena, "NULL");
    }
    size_t len = strlen(str);
    char *buf = arena_alloc(arena, len * 2 + 3);
    if (buf == NULL)
    {
        exit(1);
    }
    char *p = buf;
    *p++ = '"';
    for (const char *s = str; *s; s++)
    {
        switch (*s)
        {
        case '"':
            *p++ = '\\';
            *p++ = '"';
            break;
        case '\\':
            *p++ = '\\';
            *p++ = '\\';
            break;
        case '\n':
            *p++ = '\\';
            *p++ = 'n';
            break;
        case '\t':
            *p++ = '\\';
            *p++ = 't';
            break;
        case '\r':
            *p++ = '\\';
            *p++ = 'r';
            break;
        default:
            *p++ = *s;
            break;
        }
    }
    *p++ = '"';
    *p = '\0';
    return buf;
}
