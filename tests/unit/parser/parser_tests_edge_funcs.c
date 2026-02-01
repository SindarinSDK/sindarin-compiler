// parser_tests_edge_funcs.c
// Function declaration and call expression tests

/* ============================================================================
 * Function Declaration Tests
 * ============================================================================ */

static void test_parse_function_one_param(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(x: int): int =>\n"
        "  return x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_multiple_params(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn add(a: int, b: int): int =>\n"
        "  return a + b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 2);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_void_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn greet(): void =>\n"
        "  print(\"hello\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_VOID);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_string_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn getName(): str =>\n"
        "  return \"test\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_STRING);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_bool_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn isValid(): bool =>\n"
        "  return true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_BOOL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_multiple_statements(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): void =>\n"
        "  var x: int = 1\n"
        "  var y: int = 2\n"
        "  print(x + y)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.body_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Call Expression Tests
 * ============================================================================ */

static void test_parse_call_no_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_one_arg(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(42)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_multiple_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(1, 2, 3)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_expression_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(1 + 2, x * y)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 2);
    assert(call->as.call.arguments[0]->type == EXPR_BINARY);
    assert(call->as.call.arguments[1]->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nested_calls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(bar(baz()))\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arguments[0]->type == EXPR_CALL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
