// tests/unit/parser/parser_tests_basic_callback.c
// Variadic, native callback, and lambda parser tests

static void test_variadic_native_function_parsing()
{
    // Test basic variadic native function: native fn printf(format: str, ...): int
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "native fn printf(format: str, ...): int\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(strncmp(fn->as.function.name.start, "printf", 6) == 0);
        assert(fn->as.function.is_native == true);
        assert(fn->as.function.is_variadic == true);
        assert(fn->as.function.param_count == 1); // Only the fixed params before ...
        assert(fn->as.function.params[0].type->kind == TYPE_STRING);
        assert(fn->as.function.return_type->kind == TYPE_INT);
        assert(fn->as.function.body_count == 0); // No body

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test variadic with multiple fixed params
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "native fn sprintf(buf: *char, format: str, ...): int\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.is_native == true);
        assert(fn->as.function.is_variadic == true);
        assert(fn->as.function.param_count == 2); // Two fixed params before ...
        assert(fn->as.function.params[0].type->kind == TYPE_POINTER);
        assert(fn->as.function.params[1].type->kind == TYPE_STRING);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test variadic with no fixed params (just ...)
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "native fn vararg(...): void\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.is_native == true);
        assert(fn->as.function.is_variadic == true);
        assert(fn->as.function.param_count == 0); // No fixed params

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test non-variadic function still has is_variadic == false
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "native fn puts(s: str): int\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.is_native == true);
        assert(fn->as.function.is_variadic == false);
        assert(fn->as.function.param_count == 1);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

static void test_native_callback_type_alias_basic_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "type Comparator = native fn(a: *void, b: *void): int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *type_decl = module->statements[0];
    assert(type_decl->type == STMT_TYPE_DECL);
    assert(strncmp(type_decl->as.type_decl.name.start, "Comparator", 10) == 0);

    /* The declared type should be a function type with is_native = true */
    Type *func_type = type_decl->as.type_decl.type;
    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.is_native == true);
    assert(func_type->as.function.param_count == 2);

    /* Check parameter types are *void */
    assert(func_type->as.function.param_types[0]->kind == TYPE_POINTER);
    assert(func_type->as.function.param_types[0]->as.pointer.base_type->kind == TYPE_VOID);
    assert(func_type->as.function.param_types[1]->kind == TYPE_POINTER);
    assert(func_type->as.function.param_types[1]->as.pointer.base_type->kind == TYPE_VOID);

    /* Check return type is int */
    assert(func_type->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_callback_type_alias_simple_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "type SignalHandler = native fn(sig: int): void\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *type_decl = module->statements[0];
    assert(type_decl->type == STMT_TYPE_DECL);
    assert(strncmp(type_decl->as.type_decl.name.start, "SignalHandler", 13) == 0);

    Type *func_type = type_decl->as.type_decl.type;
    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.is_native == true);
    assert(func_type->as.function.param_count == 1);

    /* Check parameter type is int */
    assert(func_type->as.function.param_types[0]->kind == TYPE_INT);

    /* Check return type is void */
    assert(func_type->as.function.return_type->kind == TYPE_VOID);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_callback_type_alias_no_params_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "type Callback = native fn(): int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *type_decl = module->statements[0];
    assert(type_decl->type == STMT_TYPE_DECL);

    Type *func_type = type_decl->as.type_decl.type;
    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.is_native == true);
    assert(func_type->as.function.param_count == 0);
    assert(func_type->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_callback_type_alias_with_userdata_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "type EventCallback = native fn(event: int, userdata: *void): void\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *type_decl = module->statements[0];
    assert(type_decl->type == STMT_TYPE_DECL);

    Type *func_type = type_decl->as.type_decl.type;
    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.is_native == true);
    assert(func_type->as.function.param_count == 2);

    /* First param is int */
    assert(func_type->as.function.param_types[0]->kind == TYPE_INT);
    /* Second param is *void */
    assert(func_type->as.function.param_types[1]->kind == TYPE_POINTER);
    assert(func_type->as.function.param_types[1]->as.pointer.base_type->kind == TYPE_VOID);

    assert(func_type->as.function.return_type->kind == TYPE_VOID);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_callback_type_alias_parsing()
{
    test_native_callback_type_alias_basic_parsing();
    test_native_callback_type_alias_simple_parsing();
    test_native_callback_type_alias_no_params_parsing();
    test_native_callback_type_alias_with_userdata_parsing();
}

static void test_native_lambda_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Native function containing a lambda with pointer params */
    const char *source = "native fn test(): void =>\n"
                         "    var cmp = fn(a: *void, b: *void): int => 0\n"
                         "    return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *func = module->statements[0];
    assert(func->type == STMT_FUNCTION);
    assert(func->as.function.is_native == true);
    assert(func->as.function.body_count > 0);

    /* Find the var declaration with the lambda */
    Stmt *var_decl = func->as.function.body[0];
    assert(var_decl->type == STMT_VAR_DECL);
    Expr *init = var_decl->as.var_decl.initializer;
    assert(init != NULL);
    assert(init->type == EXPR_LAMBDA);

    /* Check the lambda is marked as native */
    assert(init->as.lambda.is_native == true);
    assert(init->as.lambda.param_count == 2);

    /* Check parameters are *void types */
    assert(init->as.lambda.params[0].type->kind == TYPE_POINTER);
    assert(init->as.lambda.params[0].type->as.pointer.base_type->kind == TYPE_VOID);
    assert(init->as.lambda.params[1].type->kind == TYPE_POINTER);
    assert(init->as.lambda.params[1].type->as.pointer.base_type->kind == TYPE_VOID);

    /* Check return type is int */
    assert(init->as.lambda.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_non_native_lambda_is_not_marked_native()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Regular function with a lambda */
    const char *source = "fn test(): void =>\n"
                         "    var f = fn(x: int): int => x * 2\n"
                         "    return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *func = module->statements[0];
    assert(func->type == STMT_FUNCTION);
    assert(func->as.function.is_native == false);

    /* Find the lambda */
    Stmt *var_decl = func->as.function.body[0];
    assert(var_decl->type == STMT_VAR_DECL);
    Expr *init = var_decl->as.var_decl.initializer;
    assert(init != NULL);
    assert(init->type == EXPR_LAMBDA);

    /* Check the lambda is NOT marked as native */
    assert(init->as.lambda.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_lambda_with_pointer_params_parsing()
{
    test_native_lambda_parsing();
    test_non_native_lambda_is_not_marked_native();
}

static void test_parser_basic_callback_main()
{
    TEST_SECTION("Parser Basic Callback Tests");
    TEST_RUN("variadic_native_function_parsing", test_variadic_native_function_parsing);
    TEST_RUN("native_callback_type_alias_parsing", test_native_callback_type_alias_parsing);
    TEST_RUN("native_lambda_with_pointer_params_parsing", test_native_lambda_with_pointer_params_parsing);
}
