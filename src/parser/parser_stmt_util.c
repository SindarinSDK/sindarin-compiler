// parser_stmt_util.c
// Memory qualifiers, function modifiers, and block parsing

/* Parse optional "as val" or "as ref" memory qualifier */
MemoryQualifier parser_memory_qualifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_AS))
    {
        if (parser_match(parser, TOKEN_VAL))
        {
            return MEM_AS_VAL;
        }
        else if (parser_match(parser, TOKEN_REF))
        {
            return MEM_AS_REF;
        }
        else
        {
            parser_error_at_current(parser, "Expected 'val' or 'ref' after 'as'");
        }
    }
    return MEM_DEFAULT;
}

/* Parse optional "shared" or "private" function modifier */
FunctionModifier parser_function_modifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_SHARED))
    {
        return FUNC_SHARED;
    }
    else if (parser_match(parser, TOKEN_PRIVATE))
    {
        return FUNC_PRIVATE;
    }
    return FUNC_DEFAULT;
}

int is_at_function_boundary(Parser *parser)
{
    if (parser_check(parser, TOKEN_DEDENT))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        return 1;
    }
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    /* Skip any comments that appear before INDENT */
    while (parser_check(parser, TOKEN_COMMENT))
    {
        /* Collect the comment for later attachment */
        const char *comment_text = parser->current.literal.string_value;
        if (comment_text != NULL)
        {
            if (parser->pending_comment_count >= parser->pending_comment_capacity)
            {
                int new_capacity = parser->pending_comment_capacity == 0 ? 4 : parser->pending_comment_capacity * 2;
                const char **new_comments = arena_alloc(parser->arena, sizeof(const char *) * new_capacity);
                if (new_comments != NULL)
                {
                    if (parser->pending_comments != NULL && parser->pending_comment_count > 0)
                    {
                        memcpy(new_comments, parser->pending_comments, sizeof(const char *) * parser->pending_comment_count);
                    }
                    parser->pending_comments = new_comments;
                    parser->pending_comment_capacity = new_capacity;
                }
            }
            if (parser->pending_comment_count < parser->pending_comment_capacity)
            {
                parser->pending_comments[parser->pending_comment_count++] = arena_strdup(parser->arena, comment_text);
            }
        }
        parser_advance(parser);  /* Consume comment */
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
    }

    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        return NULL;
    }
    parser_advance(parser);

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            break;
        }

        Stmt *stmt = parser_declaration(parser);

        /* Synchronize on error to prevent infinite loops */
        if (parser->panic_mode)
        {
            synchronize(parser);
        }

        if (stmt == NULL)
        {
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
    }

    return ast_create_block_stmt(parser->arena, statements, count, NULL);
}
