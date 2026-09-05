// src/lexer/lexer_scan_pipe.c
// Pipe Block String Scanning

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
