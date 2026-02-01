// parser_stmt_parse.c
// Statement parsing and comment handling

Stmt *parser_statement(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser, SYNC_NONE);
    }
    if (parser_match(parser, TOKEN_SYNC))
    {
        if (parser_match(parser, TOKEN_VAR))
        {
            return parser_var_declaration(parser, SYNC_ATOMIC);
        }
        else
        {
            parser_error_at_current(parser,
                "'sync' in statement context can only be used with 'var'. "
                "Did you mean 'sync var'?");
            return NULL;
        }
    }
    if (parser_match(parser, TOKEN_IF))
    {
        return parser_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        return parser_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        return parser_for_statement(parser);
    }
    if (parser_match(parser, TOKEN_BREAK))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'break'");
        }
        return ast_create_break_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_CONTINUE))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'continue'");
        }
        return ast_create_continue_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parser_block_statement(parser);
    }

    // Disallow shared/private blocks - these modifiers only apply to functions now
    if (parser_check(parser, TOKEN_SHARED))
    {
        parser_advance(parser);  // consume shared

        // Check if it's a loop (already disallowed) or block
        if (parser_check(parser, TOKEN_WHILE) || parser_check(parser, TOKEN_FOR))
        {
            parser_error_at_current(parser,
                "'shared' modifier on loops is no longer supported. "
                "All loops now share the function's arena by default.");
        }
        else
        {
            parser_error_at_current(parser,
                "'shared' blocks are no longer supported. "
                "Use 'shared' modifier on functions instead.");
        }
        return NULL;
    }

    if (parser_check(parser, TOKEN_PRIVATE))
    {
        parser_advance(parser);  // consume private

        parser_error_at_current(parser,
            "'private' blocks are no longer supported. "
            "Use 'private' modifier on functions instead.");
        return NULL;
    }

    // Parse lock(expr) => block
    if (parser_match(parser, TOKEN_LOCK))
    {
        Token lock_token = parser->previous;

        parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'lock'");
        Expr *lock_expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after lock expression");
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after lock(...)");
        skip_newlines(parser);

        Stmt *body = parser_indented_block(parser);
        if (body == NULL)
        {
            body = ast_create_block_stmt(parser->arena, NULL, 0, &lock_token);
        }

        return ast_create_lock_stmt(parser->arena, lock_expr, body, &lock_token);
    }

    return parser_expression_statement(parser);
}

/* Helper to collect pending // comments before a statement */
static void parser_collect_comments(Parser *parser)
{
    while (parser_check(parser, TOKEN_COMMENT))
    {
        /* Get the comment text from the token literal */
        const char *comment_text = parser->current.literal.string_value;
        if (comment_text == NULL)
        {
            comment_text = "";
        }

        /* Grow pending_comments array if needed */
        if (parser->pending_comment_count >= parser->pending_comment_capacity)
        {
            int new_capacity = parser->pending_comment_capacity == 0 ? 4 : parser->pending_comment_capacity * 2;
            const char **new_comments = arena_alloc(parser->arena, sizeof(const char *) * new_capacity);
            if (new_comments == NULL)
            {
                parser_advance(parser);  /* Skip comment on allocation failure */
                continue;
            }
            if (parser->pending_comments != NULL && parser->pending_comment_count > 0)
            {
                memcpy(new_comments, parser->pending_comments, sizeof(const char *) * parser->pending_comment_count);
            }
            parser->pending_comments = new_comments;
            parser->pending_comment_capacity = new_capacity;
        }

        /* Store the comment */
        parser->pending_comments[parser->pending_comment_count++] = arena_strdup(parser->arena, comment_text);

        parser_advance(parser);  /* Consume the comment token */

        /* Skip newlines after comment */
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
    }
}

/* Helper to attach collected comments to a statement */
static void parser_attach_comments(Parser *parser, Stmt *stmt)
{
    if (stmt == NULL || parser->pending_comment_count == 0)
    {
        /* Clear pending comments even if stmt is NULL */
        parser->pending_comment_count = 0;
        return;
    }

    /* Copy pending comments to the statement */
    stmt->comments = arena_alloc(parser->arena, sizeof(const char *) * parser->pending_comment_count);
    if (stmt->comments != NULL)
    {
        memcpy((void *)stmt->comments, parser->pending_comments, sizeof(const char *) * parser->pending_comment_count);
        stmt->comment_count = parser->pending_comment_count;
    }

    /* Clear pending comments */
    parser->pending_comment_count = 0;
}

Stmt *parser_block_statement(Parser *parser)
{
    Token brace = parser->previous;
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser) || parser_check(parser, TOKEN_DEDENT))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
            continue;

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

    symbol_table_pop_scope(parser->symbol_table);

    return ast_create_block_stmt(parser->arena, statements, count, &brace);
}

Stmt *parser_expression_statement(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    /* Match expressions consume their own block structure (INDENT/DEDENT),
     * so they don't need an additional trailing terminator */
    if (expr != NULL && expr->type == EXPR_MATCH)
    {
        /* Skip optional trailing newline */
        parser_match(parser, TOKEN_NEWLINE);
    }
    else if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) &&
        !parser_check(parser, TOKEN_DEDENT) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after expression");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_expr_stmt(parser->arena, expr, &parser->previous);
}
