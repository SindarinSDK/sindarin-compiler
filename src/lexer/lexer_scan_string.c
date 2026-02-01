// src/lexer/lexer_scan_string.c
// String and Character Literal Scanning

Token lexer_scan_string(Lexer *lexer)
{
    int buffer_size = 256;
    char *buffer = arena_alloc(lexer->arena, buffer_size);
    if (buffer == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    int buffer_index = 0;
    int brace_depth = 0;  // Track depth inside {} for interpolated strings
    int start_line = lexer->line;  // Save the line where the string starts for error reporting

    // Stack to track nested string states (simple strings vs interpolated strings)
    // Each entry: 0 = regular string, 1 = interpolated string ($"...")
    // We use a simple depth counter for regular strings and track interpolated ones
    int string_depth = 0;  // How many nested strings deep we are (within braces)
    int interpol_depth = 0;  // How many nested interpolated strings deep (within braces)

    while (!lexer_is_at_end(lexer))
    {
        char c = lexer_peek(lexer);

        // Only stop on " if we're not inside {} interpolation and not in nested string
        if (c == '"' && brace_depth == 0 && string_depth == 0)
        {
            break;
        }

        if (c == '\n')
        {
            lexer->line++;
        }

        if (c == '\\')
        {
            lexer_advance(lexer);
            if (!lexer_is_at_end(lexer))
            {
                // Handle escape sequences
                char escaped = lexer_peek(lexer);
                if (brace_depth == 0 && string_depth == 0)
                {
                    // Outside braces and nested strings, process escape sequences
                    switch (escaped)
                    {
                    case '\\':
                        buffer[buffer_index++] = '\\';
                        break;
                    case 'n':
                        buffer[buffer_index++] = '\n';
                        break;
                    case 'r':
                        buffer[buffer_index++] = '\r';
                        break;
                    case 't':
                        buffer[buffer_index++] = '\t';
                        break;
                    case '"':
                        buffer[buffer_index++] = '"';
                        break;
                    case '0':
                        buffer[buffer_index++] = '\0';
                        break;
                    case 'x':
                    {
                        /* Hex escape: \xNN where NN is exactly 2 hex digits */
                        lexer_advance(lexer); /* consume 'x' */
                        if (lexer_is_at_end(lexer))
                        {
                            snprintf(error_buffer, sizeof(error_buffer), "Incomplete hex escape");
                            return lexer_error_token(lexer, error_buffer);
                        }
                        int hi = hex_char_to_int(lexer_peek(lexer));
                        if (hi < 0)
                        {
                            snprintf(error_buffer, sizeof(error_buffer), "Invalid hex digit in escape");
                            return lexer_error_token(lexer, error_buffer);
                        }
                        lexer_advance(lexer);
                        if (lexer_is_at_end(lexer))
                        {
                            snprintf(error_buffer, sizeof(error_buffer), "Incomplete hex escape");
                            return lexer_error_token(lexer, error_buffer);
                        }
                        int lo = hex_char_to_int(lexer_peek(lexer));
                        if (lo < 0)
                        {
                            snprintf(error_buffer, sizeof(error_buffer), "Invalid hex digit in escape");
                            return lexer_error_token(lexer, error_buffer);
                        }
                        buffer[buffer_index++] = (char)((hi << 4) | lo);
                        break;
                    }
                    default:
                        snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
                        return lexer_error_token(lexer, error_buffer);
                    }
                }
                else if (brace_depth > 0 && escaped == '"')
                {
                    // Inside braces: \" is used to delimit string arguments
                    // Treat it as starting/ending a nested string (equivalent to ")
                    buffer[buffer_index++] = '"';
                    if (string_depth > 0)
                    {
                        string_depth--;
                        if (interpol_depth > 0) interpol_depth--;
                    }
                    else
                    {
                        string_depth++;
                    }
                }
                else
                {
                    // Inside nested strings, keep other escape sequences as-is for sub-parser
                    buffer[buffer_index++] = '\\';
                    buffer[buffer_index++] = escaped;
                }
                lexer_advance(lexer);
            }
            else
            {
                // Backslash at end of string
                buffer[buffer_index++] = '\\';
            }
        }
        else if (c == '$' && brace_depth > 0 && string_depth == 0 && lexer_peek_next(lexer) == '"')
        {
            // Nested interpolated string: $"..." inside braces
            // Copy $" and track we're entering an interpolated string
            buffer[buffer_index++] = '$';
            lexer_advance(lexer);
            buffer[buffer_index++] = '"';
            lexer_advance(lexer);
            string_depth++;
            interpol_depth++;
        }
        else if (c == '"' && brace_depth > 0)
        {
            // Quote inside braces - could be start or end of nested string
            if (string_depth > 0)
            {
                // Closing a nested string
                buffer[buffer_index++] = '"';
                lexer_advance(lexer);
                string_depth--;
                if (interpol_depth > 0) interpol_depth--;
            }
            else
            {
                // Opening a regular nested string (not interpolated)
                buffer[buffer_index++] = '"';
                lexer_advance(lexer);
                string_depth++;
            }
        }
        else if (c == '{' && string_depth == 0)
        {
            // Only track braces when not inside a nested string literal
            brace_depth++;
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }
        else if (c == '}' && string_depth == 0)
        {
            // Only track braces when not inside a nested string literal
            if (brace_depth > 0) brace_depth--;
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }
        else
        {
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }

        if (buffer_index >= buffer_size - 2)  // -2 for potential $" pair
        {
            buffer_size *= 2;
            char *new_buffer = arena_alloc(lexer->arena, buffer_size);
            if (new_buffer == NULL)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
                return lexer_error_token(lexer, error_buffer);
            }
            memcpy(new_buffer, buffer, buffer_index);
            buffer = new_buffer;
        }
    }
    if (lexer_is_at_end(lexer))
    {
        // Report error at the line where the string started
        int saved_line = lexer->line;
        lexer->line = start_line;
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated string starting at line %d", start_line);
        Token error_tok = lexer_error_token(lexer, error_buffer);
        lexer->line = saved_line;
        return error_tok;
    }
    lexer_advance(lexer);
    buffer[buffer_index] = '\0';
    Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);
    char *str_copy = arena_strdup(lexer->arena, buffer);
    if (str_copy == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    token_set_string_literal(&token, str_copy);
    return token;
}

Token lexer_scan_char(Lexer *lexer)
{
    char value = '\0';
    if (lexer_peek(lexer) == '\\')
    {
        lexer_advance(lexer);
        switch (lexer_peek(lexer))
        {
        case '\\':
            value = '\\';
            break;
        case 'n':
            value = '\n';
            break;
        case 'r':
            value = '\r';
            break;
        case 't':
            value = '\t';
            break;
        case '\'':
            value = '\'';
            break;
        case '0':
            value = '\0';
            break;
        case 'x':
        {
            /* Hex escape: \xNN where NN is exactly 2 hex digits */
            lexer_advance(lexer); /* consume 'x' */
            if (lexer_is_at_end(lexer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Incomplete hex escape");
                return lexer_error_token(lexer, error_buffer);
            }
            int hi = hex_char_to_int(lexer_peek(lexer));
            if (hi < 0)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Invalid hex digit in escape");
                return lexer_error_token(lexer, error_buffer);
            }
            lexer_advance(lexer);
            if (lexer_is_at_end(lexer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Incomplete hex escape");
                return lexer_error_token(lexer, error_buffer);
            }
            int lo = hex_char_to_int(lexer_peek(lexer));
            if (lo < 0)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Invalid hex digit in escape");
                return lexer_error_token(lexer, error_buffer);
            }
            value = (char)((hi << 4) | lo);
            break;
        }
        default:
            snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
            return lexer_error_token(lexer, error_buffer);
        }
    }
    else if (lexer_peek(lexer) == '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Empty character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    else
    {
        value = lexer_peek(lexer);
    }
    lexer_advance(lexer);
    if (lexer_peek(lexer) != '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    Token token = lexer_make_token(lexer, TOKEN_CHAR_LITERAL);
    token_set_char_literal(&token, value);
    return token;
}
