// tests/parser_tests_basic.c
// Basic parser tests (empty, var_decl, function_no_params, if_statement)

static void test_empty_program_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "");

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 0);
    assert(strcmp(module->filename, "test.sn") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_var_decl_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x:int = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(strcmp(stmt->as.var_decl.name.start, "x") == 0);
    assert(stmt->as.var_decl.type->kind == TYPE_INT);
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.int_value == 42);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_no_params_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "main") == 0);
    assert(fn->as.function.param_count == 0);
    assert(fn->as.function.return_type->kind == TYPE_VOID);
    assert(fn->as.function.body_count == 1);
    Stmt *print_stmt = fn->as.function.body[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.literal.value.string_value, "hello\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_if_statement_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if x > 0 =>\n"
        "  print(\"positive\\n\")\n"
        "else =>\n"
        "  print(\"non-positive\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
        if (module->count > 0)
        {
            ast_print_stmt(&arena, module->statements[0], 0);
            Stmt *if_stmt = module->statements[0];
            if (if_stmt->type == STMT_IF)
            {
                if (if_stmt->as.if_stmt.condition->type == EXPR_BINARY)
                {
                }
                if (if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
                {
                }
                if (if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
                {
                }
            }
        }
        else
        {
            DEBUG_WARNING("No statements parsed in module.");
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition->type == EXPR_BINARY);
    assert(if_stmt->as.if_stmt.condition->as.binary.operator == TOKEN_GREATER);
    assert(strcmp(if_stmt->as.if_stmt.condition->as.binary.left->as.variable.name.start, "x") == 0);
    assert(if_stmt->as.if_stmt.condition->as.binary.right->as.literal.value.int_value == 0);
    assert(if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.then_branch->as.block.count == 1);
    assert(if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.else_branch->as.block.count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interop_type_var_decl_parsing()
{
    // Test int32 type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var x: int32 = 42\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_INT32);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test uint type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var x: uint = 42\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_UINT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test uint32 type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var x: uint32 = 42\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_UINT32);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test float type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var x: float = 3.14\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_FLOAT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

static void test_interop_type_function_parsing()
{
    // Test function with int32 param and return
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn square(x: int32): int32 =>\n"
            "  return x\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.param_count == 1);
        assert(fn->as.function.params[0].type->kind == TYPE_INT32);
        assert(fn->as.function.return_type->kind == TYPE_INT32);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test function with uint param
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn process(n: uint): void =>\n"
            "  print(\"done\")\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.param_count == 1);
        assert(fn->as.function.params[0].type->kind == TYPE_UINT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test function with float return
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn getVal(): float =>\n"
            "  return 1.5\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.return_type->kind == TYPE_FLOAT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

static void test_pointer_type_var_decl_parsing()
{
    // Test *int pointer type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var p: *int = nil\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_POINTER);
        assert(stmt->as.var_decl.type->as.pointer.base_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test **int (pointer-to-pointer) type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var pp: **int = nil\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_POINTER);
        assert(stmt->as.var_decl.type->as.pointer.base_type->kind == TYPE_POINTER);
        assert(stmt->as.var_decl.type->as.pointer.base_type->as.pointer.base_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test *void pointer type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source = "var vp: *void = nil\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *stmt = module->statements[0];
        assert(stmt->type == STMT_VAR_DECL);
        assert(stmt->as.var_decl.type->kind == TYPE_POINTER);
        assert(stmt->as.var_decl.type->as.pointer.base_type->kind == TYPE_VOID);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

static void test_pointer_type_function_parsing()
{
    // Test function with pointer param
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn test(p: *int): void =>\n"
            "  print(\"done\")\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.param_count == 1);
        assert(fn->as.function.params[0].type->kind == TYPE_POINTER);
        assert(fn->as.function.params[0].type->as.pointer.base_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test function with pointer return type
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn getPtr(): *int =>\n"
            "  return nil\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.return_type->kind == TYPE_POINTER);
        assert(fn->as.function.return_type->as.pointer.base_type->kind == TYPE_INT);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }

    // Test function with double pointer param
    {
        Arena arena;
        Lexer lexer;
        Parser parser;
        SymbolTable symbol_table;
        const char *source =
            "fn test(pp: **int): void =>\n"
            "  print(\"done\")\n";
        setup_parser(&arena, &lexer, &parser, &symbol_table, source);

        Module *module = parser_execute(&parser, "test.sn");
        assert(module != NULL);
        assert(module->count == 1);
        Stmt *fn = module->statements[0];
        assert(fn->type == STMT_FUNCTION);
        assert(fn->as.function.param_count == 1);
        assert(fn->as.function.params[0].type->kind == TYPE_POINTER);
        assert(fn->as.function.params[0].type->as.pointer.base_type->kind == TYPE_POINTER);

        cleanup_parser(&arena, &lexer, &parser, &symbol_table);
    }
}

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

/* ==========================================================================
 * Opaque Type Declaration Tests
 * ========================================================================== */

static void test_opaque_type_decl_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "type FILE = opaque\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_TYPE_DECL);
    assert(stmt->as.type_decl.type != NULL);
    assert(stmt->as.type_decl.type->kind == TYPE_OPAQUE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_opaque_type_in_function_param()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* First declare the opaque type, then use it */
    const char *source = "type FILE = opaque\n"
                         "native fn fclose(f: *FILE): int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    /* First statement: type declaration */
    Stmt *type_stmt = module->statements[0];
    assert(type_stmt->type == STMT_TYPE_DECL);
    assert(type_stmt->as.type_decl.type->kind == TYPE_OPAQUE);

    /* Second statement: function declaration using the opaque type */
    Stmt *func_stmt = module->statements[1];
    assert(func_stmt->type == STMT_FUNCTION);
    assert(func_stmt->as.function.is_native == true);
    assert(func_stmt->as.function.param_count == 1);
    assert(func_stmt->as.function.params[0].type->kind == TYPE_POINTER);
    /* The base type should be the opaque FILE type */
    assert(func_stmt->as.function.params[0].type->as.pointer.base_type->kind == TYPE_OPAQUE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ==========================================================================
 * Pragma Parsing Tests
 * ========================================================================== */

static void test_pragma_include_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* WYSIWYG pragma syntax - no quotes needed around the include path */
    const char *source = "#pragma include <stdio.h>\nfn main(): void =>\n  return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    /* The pragma should have been parsed as a statement */
    assert(module->count >= 2);
    /* Find the pragma statement */
    int found_pragma = 0;
    for (int i = 0; i < module->count; i++) {
        if (module->statements[i]->type == STMT_PRAGMA) {
            assert(module->statements[i]->as.pragma.pragma_type == PRAGMA_INCLUDE);
            assert(strcmp(module->statements[i]->as.pragma.value, "<stdio.h>") == 0);
            found_pragma = 1;
            break;
        }
    }
    assert(found_pragma == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_pragma_link_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* WYSIWYG pragma syntax - no quotes needed around the library name */
    const char *source = "#pragma link m\nfn main(): void =>\n  return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    /* The pragma should have been parsed as a statement */
    assert(module->count >= 2);
    /* Find the pragma statement */
    int found_pragma = 0;
    for (int i = 0; i < module->count; i++) {
        if (module->statements[i]->type == STMT_PRAGMA) {
            assert(module->statements[i]->as.pragma.pragma_type == PRAGMA_LINK);
            assert(strcmp(module->statements[i]->as.pragma.value, "m") == 0);
            found_pragma = 1;
            break;
        }
    }
    assert(found_pragma == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multiple_pragmas_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* WYSIWYG pragma syntax */
    const char *source = "#pragma include <stdio.h>\n"
                         "#pragma include <math.h>\n"
                         "#pragma link m\n"
                         "fn main(): void =>\n"
                         "  return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    /* Count pragma statements */
    int include_count = 0;
    int link_count = 0;
    for (int i = 0; i < module->count; i++) {
        if (module->statements[i]->type == STMT_PRAGMA) {
            if (module->statements[i]->as.pragma.pragma_type == PRAGMA_INCLUDE) {
                include_count++;
            } else if (module->statements[i]->as.pragma.pragma_type == PRAGMA_LINK) {
                link_count++;
            }
        }
    }
    assert(include_count == 2);
    assert(link_count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ==========================================================================
 * As Ref Parameter Tests
 * ========================================================================== */

static void test_as_ref_parameter_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native fn get_value(out: int as ref): void\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *func = module->statements[0];
    assert(func->type == STMT_FUNCTION);
    assert(func->as.function.is_native == true);
    assert(func->as.function.param_count == 1);
    assert(func->as.function.params[0].mem_qualifier == MEM_AS_REF);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ==========================================================================
 * Variadic Function Tests (Additional)
 * ========================================================================== */

static void test_variadic_with_multiple_fixed_params_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native fn snprintf(buf: *char, size: int, format: str, ...): int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *func = module->statements[0];
    assert(func->type == STMT_FUNCTION);
    assert(func->as.function.is_native == true);
    assert(func->as.function.is_variadic == true);
    assert(func->as.function.param_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_basic_main()
{
    TEST_SECTION("Parser Basic Tests");
    TEST_RUN("empty_program_parsing", test_empty_program_parsing);
    TEST_RUN("var_decl_parsing", test_var_decl_parsing);
    TEST_RUN("function_no_params_parsing", test_function_no_params_parsing);
    TEST_RUN("if_statement_parsing", test_if_statement_parsing);
    TEST_RUN("interop_type_var_decl_parsing", test_interop_type_var_decl_parsing);
    TEST_RUN("interop_type_function_parsing", test_interop_type_function_parsing);
    TEST_RUN("pointer_type_var_decl_parsing", test_pointer_type_var_decl_parsing);
    TEST_RUN("pointer_type_function_parsing", test_pointer_type_function_parsing);
    TEST_RUN("native_function_without_body_parsing", test_native_function_without_body_parsing);
    TEST_RUN("native_function_with_body_parsing", test_native_function_with_body_parsing);
    TEST_RUN("native_function_with_pointer_types_parsing", test_native_function_with_pointer_types_parsing);
    TEST_RUN("regular_function_not_native_parsing", test_regular_function_not_native_parsing);
    TEST_RUN("as_val_postfix_with_call_parsing", test_as_val_postfix_with_call_parsing);
    TEST_RUN("as_val_postfix_with_array_access_parsing", test_as_val_postfix_with_array_access_parsing);
    TEST_RUN("as_val_postfix_with_variable_parsing", test_as_val_postfix_with_variable_parsing);
    TEST_RUN("as_val_postfix_precedence_parsing", test_as_val_postfix_precedence_parsing);
    TEST_RUN("pointer_slice_basic_parsing", test_pointer_slice_basic_parsing);
    TEST_RUN("pointer_slice_with_call_parsing", test_pointer_slice_with_call_parsing);
    TEST_RUN("pointer_slice_with_as_val_parsing", test_pointer_slice_with_as_val_parsing);
    TEST_RUN("pointer_slice_from_start_parsing", test_pointer_slice_from_start_parsing);
    TEST_RUN("inline_pointer_call_slice_as_val_parsing", test_inline_pointer_call_slice_as_val_parsing);
    TEST_RUN("variadic_native_function_parsing", test_variadic_native_function_parsing);
    TEST_RUN("native_callback_type_alias_parsing", test_native_callback_type_alias_parsing);
    TEST_RUN("native_lambda_with_pointer_params_parsing", test_native_lambda_with_pointer_params_parsing);
    TEST_RUN("opaque_type_decl_parsing", test_opaque_type_decl_parsing);
    TEST_RUN("opaque_type_in_function_param", test_opaque_type_in_function_param);
    TEST_RUN("pragma_include_parsing", test_pragma_include_parsing);
    TEST_RUN("pragma_link_parsing", test_pragma_link_parsing);
    TEST_RUN("multiple_pragmas_parsing", test_multiple_pragmas_parsing);
    TEST_RUN("as_ref_parameter_parsing", test_as_ref_parameter_parsing);
    TEST_RUN("variadic_with_multiple_fixed_params_parsing", test_variadic_with_multiple_fixed_params_parsing);
}
