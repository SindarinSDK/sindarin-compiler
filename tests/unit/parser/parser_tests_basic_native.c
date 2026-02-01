// tests/unit/parser/parser_tests_basic_native.c
// Native function and as_val postfix parser tests

static void test_native_function_without_body_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native fn sin(x: double): double\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "sin") == 0);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.params[0].type->kind == TYPE_DOUBLE);
    assert(fn->as.function.return_type->kind == TYPE_DOUBLE);
    assert(fn->as.function.body_count == 0);
    assert(fn->as.function.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_function_with_body_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "native fn my_abs(x: int): int =>\n"
        "  if x < 0 =>\n"
        "    return -x\n"
        "  return x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "my_abs") == 0);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.return_type->kind == TYPE_INT);
    assert(fn->as.function.body_count > 0);  // Has body
    assert(fn->as.function.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_function_with_pointer_types_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native fn malloc(size: uint): *void\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "malloc") == 0);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.params[0].type->kind == TYPE_UINT);
    assert(fn->as.function.return_type->kind == TYPE_POINTER);
    assert(fn->as.function.return_type->as.pointer.base_type->kind == TYPE_VOID);
    assert(fn->as.function.body_count == 0);
    assert(fn->as.function.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_regular_function_not_native_parsing()
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
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_as_val_postfix_with_call_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = get_ptr() as val\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_AS_VAL
    assert(stmt->as.var_decl.initializer->type == EXPR_AS_VAL);
    // The operand should be a function call
    assert(stmt->as.var_decl.initializer->as.as_val.operand->type == EXPR_CALL);
    // The callee should be get_ptr
    assert(strcmp(stmt->as.var_decl.initializer->as.as_val.operand->as.call.callee->as.variable.name.start, "get_ptr") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_as_val_postfix_with_array_access_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = arr[i] as val\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_AS_VAL
    assert(stmt->as.var_decl.initializer->type == EXPR_AS_VAL);
    // The operand should be an array access
    assert(stmt->as.var_decl.initializer->as.as_val.operand->type == EXPR_ARRAY_ACCESS);
    // The array should be 'arr'
    assert(strcmp(stmt->as.var_decl.initializer->as.as_val.operand->as.array_access.array->as.variable.name.start, "arr") == 0);
    // The index should be 'i'
    assert(strcmp(stmt->as.var_decl.initializer->as.as_val.operand->as.array_access.index->as.variable.name.start, "i") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_as_val_postfix_with_variable_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = ptr as val\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_AS_VAL
    assert(stmt->as.var_decl.initializer->type == EXPR_AS_VAL);
    // The operand should be a variable
    assert(stmt->as.var_decl.initializer->as.as_val.operand->type == EXPR_VARIABLE);
    assert(strcmp(stmt->as.var_decl.initializer->as.as_val.operand->as.variable.name.start, "ptr") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_as_val_postfix_precedence_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // This tests that 'as val' binds tighter than '+' (as a postfix after array access)
    const char *source = "var x: int = arr[0] as val + 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be a binary expression (addition)
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_PLUS);
    // The left side should be 'arr[0] as val'
    assert(stmt->as.var_decl.initializer->as.binary.left->type == EXPR_AS_VAL);
    assert(stmt->as.var_decl.initializer->as.binary.left->as.as_val.operand->type == EXPR_ARRAY_ACCESS);
    // The right side should be literal 1
    assert(stmt->as.var_decl.initializer->as.binary.right->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.binary.right->as.literal.value.int_value == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_basic_native_main()
{
    TEST_SECTION("Parser Basic Native Tests");
    TEST_RUN("native_function_without_body_parsing", test_native_function_without_body_parsing);
    TEST_RUN("native_function_with_body_parsing", test_native_function_with_body_parsing);
    TEST_RUN("native_function_with_pointer_types_parsing", test_native_function_with_pointer_types_parsing);
    TEST_RUN("regular_function_not_native_parsing", test_regular_function_not_native_parsing);
    TEST_RUN("as_val_postfix_with_call_parsing", test_as_val_postfix_with_call_parsing);
    TEST_RUN("as_val_postfix_with_array_access_parsing", test_as_val_postfix_with_array_access_parsing);
    TEST_RUN("as_val_postfix_with_variable_parsing", test_as_val_postfix_with_variable_parsing);
    TEST_RUN("as_val_postfix_precedence_parsing", test_as_val_postfix_precedence_parsing);
}
