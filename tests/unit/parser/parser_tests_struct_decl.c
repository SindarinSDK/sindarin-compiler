// parser_tests_struct_decl.c
// Basic struct declaration parser tests

static void test_struct_decl_empty_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "struct Point =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Point", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 0);
    assert(stmt->as.struct_decl.fields == NULL);
    assert(stmt->as.struct_decl.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_struct_decl_empty_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native struct ZStream =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "ZStream", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 0);
    assert(stmt->as.struct_decl.fields == NULL);
    assert(stmt->as.struct_decl.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_decl_name_only()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "struct Rectangle =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Rectangle", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multiple_struct_decls()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "struct Rectangle =>\n"
        "native struct Buffer =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 3);

    /* First struct: Point (not native) */
    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt1->as.struct_decl.name.start, "Point", stmt1->as.struct_decl.name.length) == 0);
    assert(stmt1->as.struct_decl.is_native == false);

    /* Second struct: Rectangle (not native) */
    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt2->as.struct_decl.name.start, "Rectangle", stmt2->as.struct_decl.name.length) == 0);
    assert(stmt2->as.struct_decl.is_native == false);

    /* Third struct: Buffer (native) */
    Stmt *stmt3 = module->statements[2];
    assert(stmt3->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt3->as.struct_decl.name.start, "Buffer", stmt3->as.struct_decl.name.length) == 0);
    assert(stmt3->as.struct_decl.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_two_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Point", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 2);
    assert(stmt->as.struct_decl.fields != NULL);
    assert(stmt->as.struct_decl.is_native == false);

    /* Check first field: x: double */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "x") == 0);
    assert(stmt->as.struct_decl.fields[0].type != NULL);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[0].default_value == NULL);

    /* Check second field: y: double */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "y") == 0);
    assert(stmt->as.struct_decl.fields[1].type != NULL);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[1].default_value == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_three_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    timeout: int\n"
        "    retries: int\n"
        "    verbose: bool\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Config", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 3);
    assert(stmt->as.struct_decl.fields != NULL);

    /* Check first field: timeout: int */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "timeout") == 0);
    assert(stmt->as.struct_decl.fields[0].type != NULL);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);

    /* Check second field: retries: int */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "retries") == 0);
    assert(stmt->as.struct_decl.fields[1].type != NULL);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);

    /* Check third field: verbose: bool */
    assert(strcmp(stmt->as.struct_decl.fields[2].name, "verbose") == 0);
    assert(stmt->as.struct_decl.fields[2].type != NULL);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_BOOL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_various_types()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Mixed =>\n"
        "    name: str\n"
        "    count: long\n"
        "    flag: byte\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 3);

    /* Check field types */
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_STRING);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_LONG);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_BYTE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_struct_with_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "native struct Buffer =>\n"
        "    length: int\n"
        "    capacity: int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Buffer", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 2);
    assert(stmt->as.struct_decl.is_native == true);

    /* Check fields */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "length") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "capacity") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
