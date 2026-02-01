// src/runtime/runtime_string_format.c
// Format Functions (rt_format_*)

/* ============================================================================
 * Format Functions (rt_format_*)
 *
 * These functions support format specifiers in string interpolation.
 * Format syntax: [0][width][type] where type varies by function.
 * ============================================================================ */

char *rt_format_long(RtArena *arena, long long val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%lld", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][type] where type is d, x, X, o, b */
    int width = 0;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Determine type */
    char type = *p ? *p : 'd';

    switch (type) {
        case 'd':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dlld", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dlld", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%lld");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'x':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllx", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllx", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llx");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'X':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllX", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllX", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llX");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'o':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllo", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllo", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llo");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'b': {
            /* Binary format - custom implementation */
            char binbuf[65];
            int len = 0;
            unsigned long long uval = (unsigned long long)val;
            if (uval == 0) {
                binbuf[len++] = '0';
            } else {
                while (uval > 0) {
                    binbuf[len++] = (uval & 1) ? '1' : '0';
                    uval >>= 1;
                }
            }
            /* Reverse the string */
            for (int i = 0; i < len / 2; i++) {
                char tmp = binbuf[i];
                binbuf[i] = binbuf[len - 1 - i];
                binbuf[len - 1 - i] = tmp;
            }
            /* Pad if needed */
            if (width > len) {
                int pad = width - len;
                memmove(binbuf + pad, binbuf, len);
                for (int i = 0; i < pad; i++) {
                    binbuf[i] = zero_pad ? '0' : ' ';
                }
                len = width;
            }
            binbuf[len] = '\0';
            return rt_arena_strdup(arena, binbuf);
        }
        default:
            snprintf(buf, sizeof(buf), "%lld", val);
            break;
    }

    return rt_arena_strdup(arena, buf);
}

char *rt_format_double(RtArena *arena, double val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%g", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][.precision][type] where type is f, e, E, g, G, % */
    int width = 0;
    int precision = -1;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse precision */
    if (*p == '.') {
        p++;
        precision = 0;
        while (*p >= '0' && *p <= '9') {
            precision = precision * 10 + (*p - '0');
            p++;
        }
    }

    /* Determine type */
    char type = *p ? *p : 'f';

    /* Handle percentage format */
    if (type == '%') {
        val *= 100.0;
        if (precision >= 0) {
            snprintf(format_str, sizeof(format_str), "%%.%df%%%%", precision);
        } else {
            snprintf(format_str, sizeof(format_str), "%%f%%%%");
        }
        snprintf(buf, sizeof(buf), format_str, val);
        return rt_arena_strdup(arena, buf);
    }

    /* Build format string dynamically */
    int pos = 0;
    format_str[pos++] = '%';
    if (zero_pad) format_str[pos++] = '0';
    if (width > 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, "%d", width);
    }
    if (precision >= 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, ".%d", precision);
    }

    switch (type) {
        case 'f':
            format_str[pos++] = 'f';
            break;
        case 'e':
            format_str[pos++] = 'e';
            break;
        case 'E':
            format_str[pos++] = 'E';
            break;
        case 'g':
            format_str[pos++] = 'g';
            break;
        case 'G':
            format_str[pos++] = 'G';
            break;
        default:
            format_str[pos++] = 'f';
            break;
    }
    format_str[pos] = '\0';

    snprintf(buf, sizeof(buf), format_str, val);
    return rt_arena_strdup(arena, buf);
}

char *rt_format_string(RtArena *arena, const char *val, const char *fmt)
{
    if (val == NULL) {
        val = "nil";
    }

    if (fmt == NULL || fmt[0] == '\0') {
        return rt_arena_strdup(arena, val);
    }

    /* Parse format specifier: [width][.maxlen]s */
    int width = 0;
    int maxlen = -1;
    int left_align = 0;
    const char *p = fmt;

    /* Check for left alignment */
    if (*p == '-') {
        left_align = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse max length */
    if (*p == '.') {
        p++;
        maxlen = 0;
        while (*p >= '0' && *p <= '9') {
            maxlen = maxlen * 10 + (*p - '0');
            p++;
        }
    }

    int len = strlen(val);

    /* Apply max length truncation */
    if (maxlen >= 0 && len > maxlen) {
        len = maxlen;
    }

    /* Calculate output size */
    int out_len = len;
    if (width > len) {
        out_len = width;
    }

    char *result = rt_arena_alloc(arena, out_len + 1);
    if (result == NULL) return NULL;

    if (width > len) {
        if (left_align) {
            memcpy(result, val, len);
            memset(result + len, ' ', width - len);
        } else {
            memset(result, ' ', width - len);
            memcpy(result + (width - len), val, len);
        }
        result[width] = '\0';
    } else {
        memcpy(result, val, len);
        result[len] = '\0';
    }

    return result;
}
