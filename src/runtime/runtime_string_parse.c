// src/runtime/runtime_string_parse.c
// String Parsing Functions

/* ============================================================================
 * String Parsing Functions
 * ============================================================================ */

long long rt_str_to_int(const char *str) {
    if (str == NULL || *str == '\0') return 0;

    long long result = 0;
    int negative = 0;
    const char *p = str;

    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    /* Handle sign */
    if (*p == '-') {
        negative = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    /* Parse digits */
    while (*p >= '0' && *p <= '9') {
        result = result * 10 + (*p - '0');
        p++;
    }

    return negative ? -result : result;
}

long long rt_str_to_long(const char *str) {
    /* Same implementation as toInt for now */
    return rt_str_to_int(str);
}

double rt_str_to_double(const char *str) {
    if (str == NULL || *str == '\0') return 0.0;

    double result = 0.0;
    double fraction = 0.0;
    double fraction_div = 1.0;
    int negative = 0;
    int in_fraction = 0;
    int exp_negative = 0;
    int exponent = 0;
    const char *p = str;

    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    /* Handle sign */
    if (*p == '-') {
        negative = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    /* Parse integer and fraction parts */
    while ((*p >= '0' && *p <= '9') || *p == '.') {
        if (*p == '.') {
            in_fraction = 1;
            p++;
            continue;
        }
        if (in_fraction) {
            fraction = fraction * 10 + (*p - '0');
            fraction_div *= 10;
        } else {
            result = result * 10 + (*p - '0');
        }
        p++;
    }

    /* Handle exponent */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-') {
            exp_negative = 1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            exponent = exponent * 10 + (*p - '0');
            p++;
        }
    }

    result = result + fraction / fraction_div;
    if (negative) result = -result;

    /* Apply exponent */
    if (exponent > 0) {
        double multiplier = 1.0;
        for (int i = 0; i < exponent; i++) {
            multiplier *= 10.0;
        }
        if (exp_negative) {
            result /= multiplier;
        } else {
            result *= multiplier;
        }
    }

    return result;
}
