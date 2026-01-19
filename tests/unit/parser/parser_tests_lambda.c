// tests/parser_tests_lambda.c
// Lambda parser tests - single-line and multi-line lambdas

static void test_single_line_lambda_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var double_it: fn(int): int = fn(x: int): int => x * 2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 1);
    assert(lambda->has_stmt_body == 0);  /* Single-line lambda uses expression body */
    assert(lambda->body != NULL);
    assert(lambda->body->type == EXPR_BINARY);  /* x * 2 */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multi_line_lambda_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var abs_val: fn(int): int = fn(x: int): int =>\n"
        "    if x < 0 =>\n"
        "        return 0 - x\n"
        "    return x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 1);
    assert(lambda->has_stmt_body == 1);  /* Multi-line lambda uses statement body */
    assert(lambda->body_stmt_count == 2);  /* if statement + return statement */
    assert(lambda->body_stmts != NULL);
    assert(lambda->body_stmts[0]->type == STMT_IF);
    assert(lambda->body_stmts[1]->type == STMT_RETURN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multi_line_lambda_with_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var make_range: fn(int, int): int[] = fn(start: int, end: int): int[] =>\n"
        "    var result: int[] = {}\n"
        "    for var i: int = start; i < end; i++ =>\n"
        "        result.push(i)\n"
        "    return result\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 2);
    assert(lambda->has_stmt_body == 1);
    assert(lambda->body_stmt_count == 3);  /* var decl + for loop + return */
    assert(lambda->body_stmts[0]->type == STMT_VAR_DECL);
    assert(lambda->body_stmts[1]->type == STMT_FOR);
    assert(lambda->body_stmts[2]->type == STMT_RETURN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parenthesized_function_type_array()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test: (fn(int): int)[] should parse as array of functions */
    const char *source =
        "var callbacks: (fn(int): int)[] = {}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);

    /* The type should be: array of (function from int to int) */
    Type *type = stmt->as.var_decl.type;
    assert(type != NULL);
    assert(type->kind == TYPE_ARRAY);  /* Outer type is array */
    assert(type->as.array.element_type != NULL);
    assert(type->as.array.element_type->kind == TYPE_FUNCTION);  /* Element is function type */

    /* Verify the function type: fn(int): int */
    Type *fn_type = type->as.array.element_type;
    assert(fn_type->as.function.param_count == 1);
    assert(fn_type->as.function.param_types[0]->kind == TYPE_INT);
    assert(fn_type->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_returning_array_vs_array_of_functions()
{
    /* Test 1: fn(int): int[] should parse as function returning array */
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var f: fn(int): int[] = fn(x: int): int[] => {}\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        Stmt *stmt = module->statements[0];
        Type *type = stmt->as.var_decl.type;

        /* fn(int): int[] -> function returning int[] */
        assert(type->kind == TYPE_FUNCTION);
        assert(type->as.function.return_type->kind == TYPE_ARRAY);
        assert(type->as.function.return_type->as.array.element_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    /* Test 2: (fn(int): int)[] should parse as array of functions */
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var g: (fn(int): int)[] = {}\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        Stmt *stmt = module->statements[0];
        Type *type = stmt->as.var_decl.type;

        /* (fn(int): int)[] -> array of functions returning int */
        assert(type->kind == TYPE_ARRAY);
        assert(type->as.array.element_type->kind == TYPE_FUNCTION);
        assert(type->as.array.element_type->as.function.return_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

static void test_parser_lambda_main()
{
    TEST_SECTION("Parser Lambda Tests");
    TEST_RUN("single_line_lambda_parsing", test_single_line_lambda_parsing);
    TEST_RUN("multi_line_lambda_parsing", test_multi_line_lambda_parsing);
    TEST_RUN("multi_line_lambda_with_loop_parsing", test_multi_line_lambda_with_loop_parsing);
    TEST_RUN("parenthesized_function_type_array", test_parenthesized_function_type_array);
    TEST_RUN("function_returning_array_vs_array_of_functions", test_function_returning_array_vs_array_of_functions);
}
