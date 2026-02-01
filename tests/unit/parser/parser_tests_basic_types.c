// tests/unit/parser/parser_tests_basic_types.c
// Interop and pointer type parser tests

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

static void test_parser_basic_types_main()
{
    TEST_SECTION("Parser Basic Types Tests");
    TEST_RUN("interop_type_var_decl_parsing", test_interop_type_var_decl_parsing);
    TEST_RUN("interop_type_function_parsing", test_interop_type_function_parsing);
    TEST_RUN("pointer_type_var_decl_parsing", test_pointer_type_var_decl_parsing);
    TEST_RUN("pointer_type_function_parsing", test_pointer_type_function_parsing);
}
