// tests/parser_tests_static.c
// Parser tests for static method call syntax (TypeName.method())

/* Register a struct type name in the symbol table so the parser recognizes it
 * as a static type for TypeName.method() syntax. SDK types (Path, Directory,
 * Bytes) are no longer hardcoded in the parser. */
static void register_static_type(Arena *arena, SymbolTable *symbol_table, const char *name)
{
    Token type_tok;
    type_tok.start = name;
    type_tok.length = (int)strlen(name);
    type_tok.type = TOKEN_IDENTIFIER;
    type_tok.line = 0;
    type_tok.filename = "test.sn";
    Type *struct_type = ast_create_struct_type(arena, name, NULL, 0, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(symbol_table, type_tok, struct_type);
}

static void test_static_call_no_args_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "Path.separator()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Path");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *expr = stmt->as.expression.expression;
    assert(expr->type == EXPR_STATIC_CALL);
    assert(strncmp(expr->as.static_call.type_name.start, "Path", 4) == 0);
    assert(strncmp(expr->as.static_call.method_name.start, "separator", 9) == 0);
    assert(expr->as.static_call.arg_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_one_arg_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "Path.exists(\"/tmp/data.txt\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Path");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *expr = stmt->as.expression.expression;
    assert(expr->type == EXPR_STATIC_CALL);
    assert(strncmp(expr->as.static_call.type_name.start, "Path", 4) == 0);
    assert(strncmp(expr->as.static_call.method_name.start, "exists", 6) == 0);
    assert(expr->as.static_call.arg_count == 1);
    assert(expr->as.static_call.arguments[0]->type == EXPR_LITERAL);
    assert(strcmp(expr->as.static_call.arguments[0]->as.literal.value.string_value, "/tmp/data.txt") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_multiple_args_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "Path.join(\"home\", \"user\", \"file.txt\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Path");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *expr = stmt->as.expression.expression;
    assert(expr->type == EXPR_STATIC_CALL);
    assert(strncmp(expr->as.static_call.type_name.start, "Path", 4) == 0);
    assert(strncmp(expr->as.static_call.method_name.start, "join", 4) == 0);
    assert(expr->as.static_call.arg_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_in_var_decl_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var exists: bool = Path.exists(\"/tmp\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Path");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_STATIC_CALL);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.type_name.start, "Path", 4) == 0);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.method_name.start, "exists", 6) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_bytes_from_hex_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var data: byte[] = Bytes.fromHex(\"48656c6c6f\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Bytes");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_STATIC_CALL);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.type_name.start, "Bytes", 5) == 0);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.method_name.start, "fromHex", 7) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_directory_list_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var files: str[] = Directory.list(\"/home\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);
    register_static_type(&arena, &symbol_table, "Directory");

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_STATIC_CALL);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.type_name.start, "Directory", 9) == 0);
    assert(strncmp(stmt->as.var_decl.initializer->as.static_call.method_name.start, "list", 4) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_static_call_vs_instance_call_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Note: 'file' is a regular variable, so file.close() is an instance method call,
     * but TextFile.open() is a static call */
    const char *source = "var file: str = \"test\"\n"
                         "var len: int = file.length\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    /* Second statement should be a member expression (not static call) */
    Stmt *stmt = module->statements[1];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_MEMBER);
    assert(strncmp(stmt->as.var_decl.initializer->as.member.member_name.start, "length", 6) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_static_main()
{
    TEST_SECTION("Parser Static Method Tests");
    TEST_RUN("static_call_no_args_parsing", test_static_call_no_args_parsing);
    TEST_RUN("static_call_one_arg_parsing", test_static_call_one_arg_parsing);
    TEST_RUN("static_call_multiple_args_parsing", test_static_call_multiple_args_parsing);
    TEST_RUN("static_call_in_var_decl_parsing", test_static_call_in_var_decl_parsing);
    TEST_RUN("static_call_bytes_from_hex_parsing", test_static_call_bytes_from_hex_parsing);
    TEST_RUN("static_call_directory_list_parsing", test_static_call_directory_list_parsing);
    TEST_RUN("static_call_vs_instance_call_parsing", test_static_call_vs_instance_call_parsing);
}
