// tests/parser_tests_memory.c
// Parser tests for memory management syntax (as val, as ref, shared, private)

static void test_var_decl_as_val_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var arr: int[] as val = {1, 2, 3}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_AS_VAL);
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_var_decl_as_ref_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int as ref = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_AS_REF);
    assert(stmt->as.var_decl.type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_var_decl_default_qualifier_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_DEFAULT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_param_as_val_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn process(arr: int[] as val): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.params[0].mem_qualifier == MEM_AS_VAL);
    assert(fn->as.function.params[0].type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_shared_modifier_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn helper() shared: int =>\n"
        "  return 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_SHARED);
    assert(fn->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_private_modifier_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn compute() private: double =>\n"
        "  return 3.14\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_PRIVATE);
    assert(fn->as.function.return_type->kind == TYPE_DOUBLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_default_modifier_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_DEFAULT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_shared_block_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  shared =>\n"
        "    var x: int = 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 1);
    Stmt *block = fn->as.function.body[0];
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.modifier == BLOCK_SHARED);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_private_block_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  private =>\n"
        "    var x: int = 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 1);
    Stmt *block = fn->as.function.body[0];
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.modifier == BLOCK_PRIVATE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_shared_while_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var i: int = 0\n"
        "  shared while i < 10 =>\n"
        "    i = i + 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *while_stmt = fn->as.function.body[1];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.is_shared == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_shared_for_each_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var arr: int[] = {1, 2, 3}\n"
        "  shared for x in arr =>\n"
        "    print($\"{x}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR_EACH);
    assert(for_stmt->as.for_each_stmt.is_shared == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_regular_while_loop_not_shared_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var i: int = 0\n"
        "  while i < 10 =>\n"
        "    i = i + 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *while_stmt = fn->as.function.body[1];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.is_shared == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_regular_for_each_loop_not_shared_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var arr: int[] = {1, 2, 3}\n"
        "  for x in arr =>\n"
        "    print($\"{x}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR_EACH);
    assert(for_stmt->as.for_each_stmt.is_shared == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_shared_cstyle_for_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var sum: int = 0\n"
        "  shared for var i: int = 0; i < 5; i++ =>\n"
        "    sum = sum + i\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR);
    assert(for_stmt->as.for_stmt.is_shared == true);
    /* Verify loop structure */
    assert(for_stmt->as.for_stmt.initializer != NULL);
    assert(for_stmt->as.for_stmt.initializer->type == STMT_VAR_DECL);
    assert(for_stmt->as.for_stmt.condition != NULL);
    assert(for_stmt->as.for_stmt.increment != NULL);
    assert(for_stmt->as.for_stmt.body != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_regular_cstyle_for_loop_not_shared_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var sum: int = 0\n"
        "  for var i: int = 0; i < 5; i++ =>\n"
        "    sum = sum + i\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR);
    assert(for_stmt->as.for_stmt.is_shared == false);
    /* Verify loop structure */
    assert(for_stmt->as.for_stmt.initializer != NULL);
    assert(for_stmt->as.for_stmt.condition != NULL);
    assert(for_stmt->as.for_stmt.increment != NULL);
    assert(for_stmt->as.for_stmt.body != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_import_without_namespace_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math_utils\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(strcmp(stmt->as.import.module_name.start, "math_utils") == 0);
    assert(stmt->as.import.namespace == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_import_with_namespace_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math_utils\" as math\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(strcmp(stmt->as.import.module_name.start, "math_utils") == 0);
    assert(stmt->as.import.namespace != NULL);
    assert(strncmp(stmt->as.import.namespace->start, "math", stmt->as.import.namespace->length) == 0);
    assert(stmt->as.import.namespace->length == 4);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_import_with_underscore_namespace_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"http_client\" as _http\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(strncmp(stmt->as.import.namespace->start, "_http", stmt->as.import.namespace->length) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multiple_imports_mixed_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "import \"strings\"\n"
        "import \"math\" as m\n"
        "import \"utils\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 3);

    /* First import: no namespace */
    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_IMPORT);
    assert(strcmp(stmt1->as.import.module_name.start, "strings") == 0);
    assert(stmt1->as.import.namespace == NULL);

    /* Second import: with namespace */
    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_IMPORT);
    assert(strcmp(stmt2->as.import.module_name.start, "math") == 0);
    assert(stmt2->as.import.namespace != NULL);
    assert(strncmp(stmt2->as.import.namespace->start, "m", stmt2->as.import.namespace->length) == 0);

    /* Third import: no namespace */
    Stmt *stmt3 = module->statements[2];
    assert(stmt3->type == STMT_IMPORT);
    assert(strcmp(stmt3->as.import.module_name.start, "utils") == 0);
    assert(stmt3->as.import.namespace == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_import_keyword_as_namespace_error()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math\" as for\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should report error and return NULL */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_import_missing_namespace_after_as_error()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math\" as\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should report error and return NULL */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_memory_main()
{
    TEST_SECTION("Parser Memory Management Tests");
    TEST_RUN("var_decl_as_val_parsing", test_var_decl_as_val_parsing);
    TEST_RUN("var_decl_as_ref_parsing", test_var_decl_as_ref_parsing);
    TEST_RUN("var_decl_default_qualifier_parsing", test_var_decl_default_qualifier_parsing);
    TEST_RUN("function_param_as_val_parsing", test_function_param_as_val_parsing);
    TEST_RUN("function_shared_modifier_parsing", test_function_shared_modifier_parsing);
    TEST_RUN("function_private_modifier_parsing", test_function_private_modifier_parsing);
    TEST_RUN("function_default_modifier_parsing", test_function_default_modifier_parsing);
    TEST_RUN("shared_block_parsing", test_shared_block_parsing);
    TEST_RUN("private_block_parsing", test_private_block_parsing);
    TEST_RUN("shared_while_loop_parsing", test_shared_while_loop_parsing);
    TEST_RUN("shared_for_each_loop_parsing", test_shared_for_each_loop_parsing);
    TEST_RUN("shared_cstyle_for_loop_parsing", test_shared_cstyle_for_loop_parsing);
    TEST_RUN("regular_while_loop_not_shared_parsing", test_regular_while_loop_not_shared_parsing);
    TEST_RUN("regular_for_each_loop_not_shared_parsing", test_regular_for_each_loop_not_shared_parsing);
    TEST_RUN("regular_cstyle_for_loop_not_shared_parsing", test_regular_cstyle_for_loop_not_shared_parsing);
    TEST_RUN("import_without_namespace_parsing", test_import_without_namespace_parsing);
    TEST_RUN("import_with_namespace_parsing", test_import_with_namespace_parsing);
    TEST_RUN("import_with_underscore_namespace_parsing", test_import_with_underscore_namespace_parsing);
    TEST_RUN("multiple_imports_mixed_parsing", test_multiple_imports_mixed_parsing);
    TEST_RUN("import_keyword_as_namespace_error", test_import_keyword_as_namespace_error);
    TEST_RUN("import_missing_namespace_after_as_error", test_import_missing_namespace_after_as_error);
}
