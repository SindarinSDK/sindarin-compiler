#include "lexer/lexer_scan.h"
#include "lexer/lexer_util.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

static char error_buffer[128];

SnTokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, SnTokenType type)
{
    int lexeme_length = (int)(lexer->current - lexer->start);
    if (lexeme_length == start + length &&
        memcmp(lexer->start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

SnTokenType lexer_identifier_type(Lexer *lexer)
{
    switch (lexer->start[0])
    {
    case 'a':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 's':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_AS);
            case 'n':
                return lexer_check_keyword(lexer, 2, 1, "y", TOKEN_ANY);
            }
        }
        break;
    case 'b':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
            case 'r':
                return lexer_check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
            case 'y':
                return lexer_check_keyword(lexer, 2, 2, "te", TOKEN_BYTE);
            }
        }
        break;
    case 'c':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'h':
                return lexer_check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR);
            case 'o':
                return lexer_check_keyword(lexer, 2, 6, "ntinue", TOKEN_CONTINUE);
            }
        }
        break;
    case 'd':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 4, "uble", TOKEN_DOUBLE);
            }
        }
        break;
    case 'e':
        return lexer_check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 3, "lse", TOKEN_BOOL_LITERAL);
            case 'l':
                return lexer_check_keyword(lexer, 2, 3, "oat", TOKEN_FLOAT);
            case 'n':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_FN);
            case 'o':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 'i':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'f':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IF);
            case 'm':
                return lexer_check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
            case 'n':
                // Check for "in" (2 chars) vs "int" (3 chars) vs "int32" (5 chars)
                if (lexer->current - lexer->start == 2)
                {
                    return TOKEN_IN;
                }
                if (lexer->current - lexer->start == 5)
                {
                    return lexer_check_keyword(lexer, 2, 3, "t32", TOKEN_INT32);
                }
                return lexer_check_keyword(lexer, 2, 1, "t", TOKEN_INT);
            case 's':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IS);
            }
        }
        break;
    case 'l':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                // Check for "long" (4 chars) vs "lock" (4 chars)
                if (lexer->current - lexer->start == 4 && lexer->start[2] == 'c')
                {
                    return lexer_check_keyword(lexer, 2, 2, "ck", TOKEN_LOCK);
                }
                return lexer_check_keyword(lexer, 2, 2, "ng", TOKEN_LONG);
            }
        }
        break;
    case 'n':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 4, "tive", TOKEN_NATIVE);
            case 'i':
                return lexer_check_keyword(lexer, 2, 1, "l", TOKEN_NIL);
            }
        }
        break;
    case 'p':
        return lexer_check_keyword(lexer, 1, 6, "rivate", TOKEN_PRIVATE);
    case 'r':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'e':
                // Check for "ref" (3 chars) vs "return" (6 chars)
                if (lexer->current - lexer->start == 3)
                {
                    return lexer_check_keyword(lexer, 2, 1, "f", TOKEN_REF);
                }
                return lexer_check_keyword(lexer, 2, 4, "turn", TOKEN_RETURN);
            }
        }
        break;
    case 's':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 't':
                // Check for "str" (3 chars) vs "struct" (6 chars) vs "static" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    // Both "struct" and "static" are 6 chars, check 3rd char
                    if (lexer->start[2] == 'r')
                    {
                        return lexer_check_keyword(lexer, 2, 4, "ruct", TOKEN_STRUCT);
                    }
                    return lexer_check_keyword(lexer, 2, 4, "atic", TOKEN_STATIC);
                }
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_STR);
            case 'h':
                return lexer_check_keyword(lexer, 2, 4, "ared", TOKEN_SHARED);
            case 'i':
                return lexer_check_keyword(lexer, 2, 4, "zeof", TOKEN_SIZEOF);
            case 'y':
                return lexer_check_keyword(lexer, 2, 2, "nc", TOKEN_SYNC);
            }
        }
        break;
    case 't':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'r':
                return lexer_check_keyword(lexer, 2, 2, "ue", TOKEN_BOOL_LITERAL);
            case 'y':
                // Check for "type" (4 chars) vs "typeof" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    return lexer_check_keyword(lexer, 2, 4, "peof", TOKEN_TYPEOF);
                }
                return lexer_check_keyword(lexer, 2, 2, "pe", TOKEN_KEYWORD_TYPE);
            }
        }
        break;
    case 'o':
        return lexer_check_keyword(lexer, 1, 5, "paque", TOKEN_OPAQUE);
    case 'u':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'i':
                // Check for "uint" (4 chars) vs "uint32" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    return lexer_check_keyword(lexer, 2, 4, "nt32", TOKEN_UINT32);
                }
                return lexer_check_keyword(lexer, 2, 2, "nt", TOKEN_UINT);
            }
        }
        break;
    case 'v':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                // Check for "val" (3 chars) vs "var" (3 chars)
                if (lexer->current - lexer->start == 3)
                {
                    if (lexer->start[2] == 'l')
                    {
                        return TOKEN_VAL;
                    }
                    else if (lexer->start[2] == 'r')
                    {
                        return TOKEN_VAR;
                    }
                }
                break;
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
            }
        }
        break;
    case 'w':
        return lexer_check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_scan_identifier(Lexer *lexer)
{
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_')
    {
        lexer_advance(lexer);
    }
    SnTokenType type = lexer_identifier_type(lexer);
    if (type == TOKEN_BOOL_LITERAL)
    {
        Token token = lexer_make_token(lexer, type);
        if (memcmp(lexer->start, "true", 4) == 0)
        {
            token_set_bool_literal(&token, 1);
        }
        else
        {
            token_set_bool_literal(&token, 0);
        }
        return token;
    }
    return lexer_make_token(lexer, type);
}

Token lexer_scan_number(Lexer *lexer)
{
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
                    default:
                        snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
                        return lexer_error_token(lexer, error_buffer);
                    }
                }
                else
                {
                    // Inside braces or nested strings, keep escape sequence as-is for sub-parser
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

/**
 * Scan a pipe block string: | followed by newline, then indented content.
 * Rules:
 * 1. | or $| followed by newline starts a block string
 * 2. All subsequent lines with greater indentation are included
 * 3. Block ends at first line with equal or less indentation (dedent)
 * 4. Common leading whitespace is stripped
 * 5. Trailing newline is included
 */
Token lexer_scan_pipe_string(Lexer *lexer, int is_interpolated)
{
    DEBUG_VERBOSE("Line %d: Scanning pipe block string (interpolated=%d)", lexer->line, is_interpolated);

    /* Skip whitespace after | and consume newline */
    while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
    {
        lexer_advance(lexer);
    }

    /* Must have newline after | */
    if (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\r' && !lexer_is_at_end(lexer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Pipe block string requires newline after '|'");
        return lexer_error_token(lexer, error_buffer);
    }

    /* Skip newline */
    if (lexer_peek(lexer) == '\r') lexer_advance(lexer);
    if (lexer_peek(lexer) == '\n')
    {
        lexer_advance(lexer);
        lexer->line++;
    }

    /* Get the base indentation level (the indentation of the | line) */
    int base_indent = lexer->indent_stack[lexer->indent_size - 1];

    /* Collect lines until we hit a dedent */
    int buffer_size = 1024;
    char *buffer = arena_alloc(lexer->arena, buffer_size);
    if (buffer == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    int buffer_index = 0;

    /* Track minimum indentation for stripping */
    int min_content_indent = INT_MAX;

    /* First pass: collect all lines and determine minimum indent */
    const char *content_start = lexer->current;
    int start_line = lexer->line;

    /* Count lines and find minimum indentation */
    typedef struct {
        const char *start;
        int length;
        int indent;
    } PipeLine;

    PipeLine *lines = arena_alloc(lexer->arena, sizeof(PipeLine) * 256);
    int line_count = 0;
    int line_capacity = 256;

    while (!lexer_is_at_end(lexer))
    {
        /* Count indentation of this line */
        int line_indent = 0;
        const char *line_start = lexer->current;
        while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
        {
            line_indent++;
            lexer_advance(lexer);
        }

        /* Check if this line is a blank line (only whitespace before newline) */
        int is_blank = (lexer_peek(lexer) == '\n' || lexer_peek(lexer) == '\r' || lexer_is_at_end(lexer));

        /* Check if we should stop (dedent to base level or less) */
        if (!is_blank && line_indent <= base_indent)
        {
            /* Rewind to the start of this line */
            lexer->current = line_start;
            break;
        }

        /* This line is part of the string */
        const char *content_line_start = lexer->current;
        int content_length = 0;

        /* Read until end of line */
        while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\r')
        {
            content_length++;
            lexer_advance(lexer);
        }

        /* Record this line (even if blank) */
        if (line_count >= line_capacity)
        {
            /* Expand lines array */
            line_capacity *= 2;
            PipeLine *new_lines = arena_alloc(lexer->arena, sizeof(PipeLine) * line_capacity);
            memcpy(new_lines, lines, sizeof(PipeLine) * line_count);
            lines = new_lines;
        }

        lines[line_count].start = content_line_start;
        lines[line_count].length = content_length;
        lines[line_count].indent = is_blank ? 0 : line_indent;
        line_count++;

        /* Track minimum indent for non-blank lines */
        if (!is_blank && line_indent < min_content_indent)
        {
            min_content_indent = line_indent;
        }

        /* Skip newline */
        if (lexer_peek(lexer) == '\r') lexer_advance(lexer);
        if (lexer_peek(lexer) == '\n')
        {
            lexer_advance(lexer);
            lexer->line++;
        }
    }

    /* If no content lines found, return empty string */
    if (line_count == 0 || min_content_indent == INT_MAX)
    {
        min_content_indent = base_indent + 1; /* Default */
    }

    /* Calculate the common indent to strip (min_content_indent - 0 = min_content_indent) */
    /* We strip min_content_indent characters from each line */
    int strip_indent = min_content_indent;

    /* Second pass: build the final string with stripped indentation */
    for (int i = 0; i < line_count; i++)
    {
        PipeLine *line = &lines[i];

        /* For blank lines, output just the newline */
        if (line->length == 0 && line->indent == 0)
        {
            /* Check buffer capacity */
            if (buffer_index >= buffer_size - 2)
            {
                buffer_size *= 2;
                char *new_buffer = arena_alloc(lexer->arena, buffer_size);
                memcpy(new_buffer, buffer, buffer_index);
                buffer = new_buffer;
            }
            buffer[buffer_index++] = '\n';
            continue;
        }

        /* Output relative indentation (line indent - strip indent) */
        int relative_indent = line->indent - strip_indent;
        if (relative_indent > 0)
        {
            /* Check buffer capacity */
            while (buffer_index + relative_indent >= buffer_size - 2)
            {
                buffer_size *= 2;
                char *new_buffer = arena_alloc(lexer->arena, buffer_size);
                memcpy(new_buffer, buffer, buffer_index);
                buffer = new_buffer;
            }
            for (int j = 0; j < relative_indent; j++)
            {
                buffer[buffer_index++] = ' ';
            }
        }

        /* Copy content */
        const char *src = line->start;
        int remaining = line->length;

        while (remaining > 0)
        {
            /* Check buffer capacity */
            if (buffer_index >= buffer_size - 2)
            {
                buffer_size *= 2;
                char *new_buffer = arena_alloc(lexer->arena, buffer_size);
                memcpy(new_buffer, buffer, buffer_index);
                buffer = new_buffer;
            }
            buffer[buffer_index++] = *src++;
            remaining--;
        }

        /* Add newline after each line */
        if (buffer_index >= buffer_size - 2)
        {
            buffer_size *= 2;
            char *new_buffer = arena_alloc(lexer->arena, buffer_size);
            memcpy(new_buffer, buffer, buffer_index);
            buffer = new_buffer;
        }
        buffer[buffer_index++] = '\n';
    }

    buffer[buffer_index] = '\0';

    /* Set lexer state for next token */
    lexer->at_line_start = 1;

    /* Create token */
    Token token = lexer_make_token(lexer, is_interpolated ? TOKEN_INTERPOL_STRING : TOKEN_STRING_LITERAL);
    char *str_copy = arena_strdup(lexer->arena, buffer);
    if (str_copy == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    token_set_string_literal(&token, str_copy);

    DEBUG_VERBOSE("Line %d: Pipe block string scanned: %d chars", lexer->line, buffer_index);
    return token;
}
