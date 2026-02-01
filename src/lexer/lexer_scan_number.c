// src/lexer/lexer_scan_number.c
// Number Literal Scanning

Token lexer_scan_number(Lexer *lexer)
{
    /* Check for hex (0x/0X), binary (0b/0B), or octal (0o/0O) prefix.
     * At this point, the caller has already consumed the first digit into lexer->start[0],
     * and lexer->current points to the next character (lexer_peek). */
    if (lexer->start[0] == '0')
    {
        char prefix = lexer_peek(lexer);

        /* Hex: 0x or 0X */
        if (prefix == 'x' || prefix == 'X')
        {
            lexer_advance(lexer); /* consume 'x'/'X' */
            if (!isxdigit(lexer_peek(lexer)))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Expected hex digit after '0%c'", prefix);
                return lexer_error_token(lexer, error_buffer);
            }
            while (isxdigit(lexer_peek(lexer)))
            {
                lexer_advance(lexer);
            }
            /* Extract hex digits (skip "0x" prefix) */
            char buffer[256];
            const char *digits_start = lexer->start + 2;
            int digits_len = (int)(lexer->current - digits_start);
            /* Check for long suffix */
            if (lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L')
            {
                lexer_advance(lexer);
                if (digits_len >= (int)sizeof(buffer))
                {
                    snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                    return lexer_error_token(lexer, error_buffer);
                }
                strncpy(buffer, digits_start, digits_len);
                buffer[digits_len] = '\0';
                Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
                int64_t value = strtoll(buffer, NULL, 16);
                token_set_int_literal(&token, value);
                return token;
            }
            if (digits_len >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, digits_start, digits_len);
            buffer[digits_len] = '\0';
            Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
            int64_t value = strtoll(buffer, NULL, 16);
            token_set_int_literal(&token, value);
            return token;
        }

        /* Binary: 0b or 0B (disambiguate from byte suffix: only if next char is 0 or 1) */
        if ((prefix == 'b' || prefix == 'B') &&
            (lexer_peek_next(lexer) == '0' || lexer_peek_next(lexer) == '1'))
        {
            lexer_advance(lexer); /* consume 'b'/'B' */
            while (lexer_peek(lexer) == '0' || lexer_peek(lexer) == '1')
            {
                lexer_advance(lexer);
            }
            /* Extract binary digits (skip "0b" prefix) */
            char buffer[256];
            const char *digits_start = lexer->start + 2;
            int digits_len = (int)(lexer->current - digits_start);
            /* Check for long suffix */
            if (lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L')
            {
                lexer_advance(lexer);
                if (digits_len >= (int)sizeof(buffer))
                {
                    snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                    return lexer_error_token(lexer, error_buffer);
                }
                strncpy(buffer, digits_start, digits_len);
                buffer[digits_len] = '\0';
                Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
                int64_t value = strtoll(buffer, NULL, 2);
                token_set_int_literal(&token, value);
                return token;
            }
            if (digits_len >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, digits_start, digits_len);
            buffer[digits_len] = '\0';
            Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
            int64_t value = strtoll(buffer, NULL, 2);
            token_set_int_literal(&token, value);
            return token;
        }

        /* Octal: 0o or 0O */
        if (prefix == 'o' || prefix == 'O')
        {
            lexer_advance(lexer); /* consume 'o'/'O' */
            if (!(lexer_peek(lexer) >= '0' && lexer_peek(lexer) <= '7'))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Expected octal digit after '0%c'", prefix);
                return lexer_error_token(lexer, error_buffer);
            }
            while (lexer_peek(lexer) >= '0' && lexer_peek(lexer) <= '7')
            {
                lexer_advance(lexer);
            }
            /* Extract octal digits (skip "0o" prefix) */
            char buffer[256];
            const char *digits_start = lexer->start + 2;
            int digits_len = (int)(lexer->current - digits_start);
            /* Check for long suffix */
            if (lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L')
            {
                lexer_advance(lexer);
                if (digits_len >= (int)sizeof(buffer))
                {
                    snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                    return lexer_error_token(lexer, error_buffer);
                }
                strncpy(buffer, digits_start, digits_len);
                buffer[digits_len] = '\0';
                Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
                int64_t value = strtoll(buffer, NULL, 8);
                token_set_int_literal(&token, value);
                return token;
            }
            if (digits_len >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, digits_start, digits_len);
            buffer[digits_len] = '\0';
            Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
            int64_t value = strtoll(buffer, NULL, 8);
            token_set_int_literal(&token, value);
            return token;
        }
    }

    while (isdigit(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }

    /* Check for decimal point (floating-point literal) */
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer)))
    {
        lexer_advance(lexer);
        while (isdigit(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }

        /* Float suffix: f or F */
        if (lexer_peek(lexer) == 'f' || lexer_peek(lexer) == 'F')
        {
            lexer_advance(lexer);
            Token token = lexer_make_token(lexer, TOKEN_FLOAT_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 1);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            double value = strtod(buffer, NULL);
            token_set_double_literal(&token, value);
            return token;
        }

        /* Double suffix: d or D (optional) */
        if (lexer_peek(lexer) == 'd' || lexer_peek(lexer) == 'D')
        {
            lexer_advance(lexer);
            Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 1);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            double value = strtod(buffer, NULL);
            token_set_double_literal(&token, value);
            return token;
        }

        /* No suffix: default to double */
        Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        double value = strtod(buffer, NULL);
        token_set_double_literal(&token, value);
        return token;
    }

    /* Integer suffixes */

    /* Long suffix: l or L */
    if (lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        int64_t value = strtoll(buffer, NULL, 10);
        token_set_int_literal(&token, value);
        return token;
    }

    /* Byte suffix: b or B */
    if (lexer_peek(lexer) == 'b' || lexer_peek(lexer) == 'B')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_BYTE_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        int64_t value = strtoll(buffer, NULL, 10);
        if (value < 0 || value > 255)
        {
            snprintf(error_buffer, sizeof(error_buffer), "Byte literal out of range (0-255)");
            return lexer_error_token(lexer, error_buffer);
        }
        token_set_int_literal(&token, value);
        return token;
    }

    /* Uint suffix: u or U (not followed by 32) */
    if ((lexer_peek(lexer) == 'u' || lexer_peek(lexer) == 'U') &&
        lexer_peek_next(lexer) != '3')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_UINT_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        uint64_t value = strtoull(buffer, NULL, 10);
        token_set_int_literal(&token, (int64_t)value);
        return token;
    }

    /* Uint32 suffix: u32 or U32 */
    if ((lexer_peek(lexer) == 'u' || lexer_peek(lexer) == 'U') &&
        lexer_peek_next(lexer) == '3')
    {
        lexer_advance(lexer); /* consume u/U */
        if (lexer_peek(lexer) == '3' && lexer_peek_next(lexer) == '2')
        {
            lexer_advance(lexer); /* consume 3 */
            lexer_advance(lexer); /* consume 2 */
            Token token = lexer_make_token(lexer, TOKEN_UINT32_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 3);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            uint64_t value = strtoull(buffer, NULL, 10);
            if (value > UINT32_MAX)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Uint32 literal out of range");
                return lexer_error_token(lexer, error_buffer);
            }
            token_set_int_literal(&token, (int64_t)value);
            return token;
        }
        /* Not u32, treat as uint */
        Token token = lexer_make_token(lexer, TOKEN_UINT_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        uint64_t value = strtoull(buffer, NULL, 10);
        token_set_int_literal(&token, (int64_t)value);
        return token;
    }

    /* Int32 suffix: i32 or I32 */
    if ((lexer_peek(lexer) == 'i' || lexer_peek(lexer) == 'I') &&
        lexer_peek_next(lexer) == '3')
    {
        lexer_advance(lexer); /* consume i/I */
        if (lexer_peek(lexer) == '3' && lexer_peek_next(lexer) == '2')
        {
            lexer_advance(lexer); /* consume 3 */
            lexer_advance(lexer); /* consume 2 */
            Token token = lexer_make_token(lexer, TOKEN_INT32_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 3);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            int64_t value = strtoll(buffer, NULL, 10);
            if (value < INT32_MIN || value > INT32_MAX)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Int32 literal out of range");
                return lexer_error_token(lexer, error_buffer);
            }
            token_set_int_literal(&token, value);
            return token;
        }
        /* Not i32, backtrack - this shouldn't happen in valid code */
        snprintf(error_buffer, sizeof(error_buffer), "Invalid number suffix");
        return lexer_error_token(lexer, error_buffer);
    }

    /* No suffix: default to int */
    Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
    char buffer[256];
    int length = (int)(lexer->current - lexer->start);
    if (length >= (int)sizeof(buffer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
        return lexer_error_token(lexer, error_buffer);
    }
    strncpy(buffer, lexer->start, length);
    buffer[length] = '\0';
    int64_t value = strtoll(buffer, NULL, 10);
    token_set_int_literal(&token, value);
    return token;
}
