// parser_init.c
// Parser initialization with built-in function registration

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
    parser->arena = arena;
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table;
    parser->sized_array_pending = 0;
    parser->sized_array_size = NULL;
    parser->in_native_function = 0;
    parser->pack_alignment = 0;  /* 0 = default alignment, 1 = packed */
    parser->pending_alias = NULL;  /* No pending alias initially */
    parser->pending_comments = NULL;  /* No pending comments initially */
    parser->pending_comment_count = 0;
    parser->pending_comment_capacity = 0;
    parser->continuation_indent_depth = 0;  /* No pending continuation dedents */

    Token print_token;
    print_token.start = arena_strdup(arena, "print");
    print_token.length = 5;
    print_token.type = TOKEN_IDENTIFIER;
    print_token.line = 0;
    print_token.filename = arena_strdup(arena, "<built-in>");

    Type *any_type = ast_create_primitive_type(arena, TYPE_ANY);
    Type **builtin_params = arena_alloc(arena, sizeof(Type *));
    builtin_params[0] = any_type;

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);

    // Array built-in: len(arr) -> int (works on arrays and strings)
    Token len_token;
    len_token.start = arena_strdup(arena, "len");
    len_token.length = 3;
    len_token.type = TOKEN_IDENTIFIER;
    len_token.line = 0;
    len_token.filename = arena_strdup(arena, "<built-in>");
    Type *len_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_INT), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, len_token, len_type, SYMBOL_GLOBAL);

    // Console I/O convenience functions
    // readLine() -> str
    Token readLine_token;
    readLine_token.start = arena_strdup(arena, "readLine");
    readLine_token.length = 8;
    readLine_token.type = TOKEN_IDENTIFIER;
    readLine_token.line = 0;
    readLine_token.filename = arena_strdup(arena, "<built-in>");
    Type *readLine_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), NULL, 0);
    symbol_table_add_symbol_with_kind(parser->symbol_table, readLine_token, readLine_type, SYMBOL_GLOBAL);

    // println(any) -> void
    Token println_token;
    println_token.start = arena_strdup(arena, "println");
    println_token.length = 7;
    println_token.type = TOKEN_IDENTIFIER;
    println_token.line = 0;
    println_token.filename = arena_strdup(arena, "<built-in>");
    Type *println_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, println_token, println_type, SYMBOL_GLOBAL);

    // printErr(any) -> void
    Token printErr_token;
    printErr_token.start = arena_strdup(arena, "printErr");
    printErr_token.length = 8;
    printErr_token.type = TOKEN_IDENTIFIER;
    printErr_token.line = 0;
    printErr_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErr_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErr_token, printErr_type, SYMBOL_GLOBAL);

    // printErrLn(any) -> void
    Token printErrLn_token;
    printErrLn_token.start = arena_strdup(arena, "printErrLn");
    printErrLn_token.length = 10;
    printErrLn_token.type = TOKEN_IDENTIFIER;
    printErrLn_token.line = 0;
    printErrLn_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErrLn_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErrLn_token, printErrLn_type, SYMBOL_GLOBAL);

    // exit(code: int) -> void
    Token exit_token;
    exit_token.start = arena_strdup(arena, "exit");
    exit_token.length = 4;
    exit_token.type = TOKEN_IDENTIFIER;
    exit_token.line = 0;
    exit_token.filename = arena_strdup(arena, "<built-in>");
    Type **exit_params = arena_alloc(arena, sizeof(Type *));
    exit_params[0] = ast_create_primitive_type(arena, TYPE_INT);
    Type *exit_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), exit_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, exit_token, exit_type, SYMBOL_GLOBAL);

    // assert(condition: bool, message: str) -> void
    Token assert_token;
    assert_token.start = arena_strdup(arena, "assert");
    assert_token.length = 6;
    assert_token.type = TOKEN_IDENTIFIER;
    assert_token.line = 0;
    assert_token.filename = arena_strdup(arena, "<built-in>");
    Type **assert_params = arena_alloc(arena, sizeof(Type *) * 2);
    assert_params[0] = ast_create_primitive_type(arena, TYPE_BOOL);
    assert_params[1] = ast_create_primitive_type(arena, TYPE_STRING);
    Type *assert_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), assert_params, 2);
    symbol_table_add_symbol_with_kind(parser->symbol_table, assert_token, assert_type, SYMBOL_GLOBAL);

    // Note: Other array operations (push, pop, rev, rem, ins) are now method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->import_ctx = NULL;
}

void parser_cleanup(Parser *parser)
{
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        return NULL;
    }

    return module;
}
