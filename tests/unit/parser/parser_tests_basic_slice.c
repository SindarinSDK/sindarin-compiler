// tests/unit/parser/parser_tests_basic_slice.c
// Pointer slice parser tests

static void test_pointer_slice_basic_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // Test that pointer slice syntax is accepted and creates EXPR_ARRAY_SLICE
    const char *source = "var data: byte[] = ptr[0..10]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_ARRAY_SLICE
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY_SLICE);
    // The base should be 'ptr' (a variable)
    assert(stmt->as.var_decl.initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(stmt->as.var_decl.initializer->as.array_slice.array->as.variable.name.start, "ptr") == 0);
    // Start should be 0
    assert(stmt->as.var_decl.initializer->as.array_slice.start != NULL);
    assert(stmt->as.var_decl.initializer->as.array_slice.start->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array_slice.start->as.literal.value.int_value == 0);
    // End should be 10
    assert(stmt->as.var_decl.initializer->as.array_slice.end != NULL);
    assert(stmt->as.var_decl.initializer->as.array_slice.end->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array_slice.end->as.literal.value.int_value == 10);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_pointer_slice_with_call_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // Test that pointer from function call can be sliced
    const char *source = "var data: byte[] = get_ptr()[0..len]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_ARRAY_SLICE
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY_SLICE);
    // The base should be a function call
    assert(stmt->as.var_decl.initializer->as.array_slice.array->type == EXPR_CALL);
    assert(strcmp(stmt->as.var_decl.initializer->as.array_slice.array->as.call.callee->as.variable.name.start, "get_ptr") == 0);
    // Start should be 0
    assert(stmt->as.var_decl.initializer->as.array_slice.start != NULL);
    assert(stmt->as.var_decl.initializer->as.array_slice.start->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array_slice.start->as.literal.value.int_value == 0);
    // End should be 'len' (a variable)
    assert(stmt->as.var_decl.initializer->as.array_slice.end != NULL);
    assert(stmt->as.var_decl.initializer->as.array_slice.end->type == EXPR_VARIABLE);
    assert(strcmp(stmt->as.var_decl.initializer->as.array_slice.end->as.variable.name.start, "len") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_pointer_slice_with_as_val_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // Test that pointer slice followed by 'as val' works (for interop buffer copy)
    const char *source = "var data: byte[] = ptr[0..len] as val\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_AS_VAL
    assert(stmt->as.var_decl.initializer->type == EXPR_AS_VAL);
    // The operand should be an EXPR_ARRAY_SLICE
    Expr *slice = stmt->as.var_decl.initializer->as.as_val.operand;
    assert(slice->type == EXPR_ARRAY_SLICE);
    // The base should be 'ptr'
    assert(slice->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(slice->as.array_slice.array->as.variable.name.start, "ptr") == 0);
    // Start should be 0
    assert(slice->as.array_slice.start != NULL);
    assert(slice->as.array_slice.start->type == EXPR_LITERAL);
    assert(slice->as.array_slice.start->as.literal.value.int_value == 0);
    // End should be 'len'
    assert(slice->as.array_slice.end != NULL);
    assert(slice->as.array_slice.end->type == EXPR_VARIABLE);
    assert(strcmp(slice->as.array_slice.end->as.variable.name.start, "len") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_pointer_slice_from_start_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // Test that pointer slice from start is accepted
    const char *source = "var data: byte[] = ptr[..len]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    // The initializer should be an EXPR_ARRAY_SLICE
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY_SLICE);
    // The base should be 'ptr'
    assert(stmt->as.var_decl.initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(stmt->as.var_decl.initializer->as.array_slice.array->as.variable.name.start, "ptr") == 0);
    // Start should be NULL (from beginning)
    assert(stmt->as.var_decl.initializer->as.array_slice.start == NULL);
    // End should be 'len'
    assert(stmt->as.var_decl.initializer->as.array_slice.end != NULL);
    assert(stmt->as.var_decl.initializer->as.array_slice.end->type == EXPR_VARIABLE);
    assert(strcmp(stmt->as.var_decl.initializer->as.array_slice.end->as.variable.name.start, "len") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_inline_pointer_call_slice_as_val_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // Test inline pointer usage: function call returning pointer, sliced, then unwrapped
    // This is the pattern from INTEROP.md: var data: byte[] = get_data()[0..len] as val
    const char *source = "var data: byte[] = get_buffer()[0..len] as val\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);

    // The initializer should be an EXPR_AS_VAL (outermost)
    Expr *as_val_expr = stmt->as.var_decl.initializer;
    assert(as_val_expr->type == EXPR_AS_VAL);

    // The operand of as_val should be an EXPR_ARRAY_SLICE
    Expr *slice = as_val_expr->as.as_val.operand;
    assert(slice->type == EXPR_ARRAY_SLICE);

    // The base of the slice should be an EXPR_CALL (the function call)
    Expr *call = slice->as.array_slice.array;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.callee->type == EXPR_VARIABLE);
    assert(strcmp(call->as.call.callee->as.variable.name.start, "get_buffer") == 0);
    assert(call->as.call.arg_count == 0);

    // Start should be 0
    assert(slice->as.array_slice.start != NULL);
    assert(slice->as.array_slice.start->type == EXPR_LITERAL);
    assert(slice->as.array_slice.start->as.literal.value.int_value == 0);

    // End should be 'len' (a variable)
    assert(slice->as.array_slice.end != NULL);
    assert(slice->as.array_slice.end->type == EXPR_VARIABLE);
    assert(strcmp(slice->as.array_slice.end->as.variable.name.start, "len") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_basic_slice_main()
{
    TEST_SECTION("Parser Basic Slice Tests");
    TEST_RUN("pointer_slice_basic_parsing", test_pointer_slice_basic_parsing);
    TEST_RUN("pointer_slice_with_call_parsing", test_pointer_slice_with_call_parsing);
    TEST_RUN("pointer_slice_with_as_val_parsing", test_pointer_slice_with_as_val_parsing);
    TEST_RUN("pointer_slice_from_start_parsing", test_pointer_slice_from_start_parsing);
    TEST_RUN("inline_pointer_call_slice_as_val_parsing", test_inline_pointer_call_slice_as_val_parsing);
}
