// parser_stmt_decl_handler.c
// Declaration parsing handler

Stmt *parser_declaration(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    /* Collect any pending // comments */
    parser_collect_comments(parser);

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    Stmt *result = NULL;

    if (parser_match(parser, TOKEN_VAR))
    {
        result = parser_var_declaration(parser, SYNC_NONE);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_STATIC))
    {
        /* 'static var' or 'static sync var' at module level for static module variables */
        SyncModifier sync_mod = SYNC_NONE;
        if (parser_match(parser, TOKEN_SYNC))
        {
            sync_mod = SYNC_ATOMIC;
        }
        if (parser_match(parser, TOKEN_VAR))
        {
            result = parser_var_declaration(parser, sync_mod);
            if (result != NULL && result->type == STMT_VAR_DECL)
            {
                result->as.var_decl.is_static = true;
            }
            goto attach_comments;
        }
        else
        {
            parser_error_at_current(parser,
                "'static' at module level can only be used with 'var' or 'sync var'. "
                "Did you mean 'static var' or 'static sync var'?");
            return NULL;
        }
    }

    if (parser_match(parser, TOKEN_SYNC))
    {
        /* 'sync var' or 'sync static var' at module level */
        bool is_static = false;
        if (parser_match(parser, TOKEN_STATIC))
        {
            is_static = true;
        }
        if (parser_match(parser, TOKEN_VAR))
        {
            result = parser_var_declaration(parser, SYNC_ATOMIC);
            if (result != NULL && result->type == STMT_VAR_DECL)
            {
                result->as.var_decl.is_static = is_static;
            }
            goto attach_comments;
        }
        else
        {
            parser_error_at_current(parser,
                "'sync' at module level can only be used with 'var' or 'static var'. "
                "Did you mean 'sync var' or 'sync static var'?");
            return NULL;
        }
    }

    if (parser_match(parser, TOKEN_FN))
    {
        result = parser_function_declaration(parser, FUNC_DEFAULT);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_NATIVE))
    {
        /* Check for 'native fn' or 'native struct' */
        if (parser_match(parser, TOKEN_FN))
        {
            result = parser_native_function_declaration(parser, FUNC_DEFAULT);
            goto attach_comments;
        }
        else if (parser_match(parser, TOKEN_STRUCT))
        {
            result = parser_struct_declaration(parser, true);
            goto attach_comments;
        }
        else
        {
            parser_error_at_current(parser, "Expected 'fn' or 'struct' after 'native'");
            return NULL;
        }
    }
    if (parser_match(parser, TOKEN_STRUCT))
    {
        result = parser_struct_declaration(parser, false);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        result = parser_import_statement(parser);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_PRAGMA_INCLUDE))
    {
        result = parser_pragma_statement(parser, PRAGMA_INCLUDE);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_PRAGMA_LINK))
    {
        result = parser_pragma_statement(parser, PRAGMA_LINK);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_PRAGMA_SOURCE))
    {
        result = parser_pragma_statement(parser, PRAGMA_SOURCE);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_PRAGMA_PACK))
    {
        result = parser_pragma_pack_statement(parser);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_PRAGMA_ALIAS))
    {
        result = parser_pragma_alias_statement(parser);
        goto attach_comments;
    }
    if (parser_match(parser, TOKEN_KEYWORD_TYPE))
    {
        result = parser_type_declaration(parser);
        goto attach_comments;
    }

    result = parser_statement(parser);

attach_comments:
    parser_attach_comments(parser, result);
    return result;
}
