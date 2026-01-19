// tests/parser_tests_namespace.c
// Parser tests for namespace/import syntax

/* Test basic import without namespace */
static void test_parse_import_basic()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mymodule\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace == NULL);
    /* Verify module name is captured correctly */
    assert(stmt->as.import.module_name.length == 8);
    assert(strncmp(stmt->as.import.module_name.start, "mymodule", 8) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test import with 'as' namespace */
static void test_parse_import_as_namespace()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"utils/string_helpers\" as strings\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    /* Verify namespace is set */
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 7);
    assert(strncmp(stmt->as.import.namespace->start, "strings", 7) == 0);
    /* Verify module path is preserved */
    assert(strncmp(stmt->as.import.module_name.start, "utils/string_helpers", 20) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test namespace with numbers in identifier */
static void test_parse_namespace_with_numbers()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"crypto\" as crypto2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 7);
    assert(strncmp(stmt->as.import.namespace->start, "crypto2", 7) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test namespace starting with underscore */
static void test_parse_namespace_underscore_start()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"internal\" as _internal\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 9);
    assert(strncmp(stmt->as.import.namespace->start, "_internal", 9) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test single-letter namespace */
static void test_parse_namespace_single_letter()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math\" as m\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 1);
    assert(stmt->as.import.namespace->start[0] == 'm');

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test long namespace name */
static void test_parse_namespace_long_name()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"database/connection\" as database_connection_manager\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 27);
    assert(strncmp(stmt->as.import.namespace->start, "database_connection_manager", 27) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test multiple imports with various namespace styles */
static void test_parse_multiple_namespace_styles()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "import \"lib1\"\n"
        "import \"lib2\" as l2\n"
        "import \"lib3\"\n"
        "import \"lib4\" as _l4\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 4);

    /* First: no namespace */
    assert(module->statements[0]->type == STMT_IMPORT);
    assert(module->statements[0]->as.import.namespace == NULL);

    /* Second: with namespace 'l2' */
    assert(module->statements[1]->type == STMT_IMPORT);
    assert(module->statements[1]->as.import.namespace != NULL);
    assert(strncmp(module->statements[1]->as.import.namespace->start, "l2", 2) == 0);

    /* Third: no namespace */
    assert(module->statements[2]->type == STMT_IMPORT);
    assert(module->statements[2]->as.import.namespace == NULL);

    /* Fourth: with namespace '_l4' */
    assert(module->statements[3]->type == STMT_IMPORT);
    assert(module->statements[3]->as.import.namespace != NULL);
    assert(strncmp(module->statements[3]->as.import.namespace->start, "_l4", 3) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'var' */
static void test_parse_invalid_namespace_keyword_var()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as var\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'fn' */
static void test_parse_invalid_namespace_keyword_fn()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as fn\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'return' */
static void test_parse_invalid_namespace_keyword_return()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'import' */
static void test_parse_invalid_namespace_keyword_import()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as import\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: type keyword 'str' as namespace */
static void test_parse_invalid_namespace_keyword_str()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as str\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - 'str' is a type keyword */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: type keyword 'int' as namespace */
static void test_parse_invalid_namespace_keyword_int()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - 'int' is a type keyword */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: missing identifier after 'as' */
static void test_parse_invalid_missing_namespace()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: number as namespace (starts with digit) */
static void test_parse_invalid_namespace_starts_with_number()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as 123abc\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - number is not a valid identifier */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test AST structure: import token info preserved */
static void test_parse_import_ast_token_info()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"my_module\" as mymod\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);

    /* Verify module name token info */
    assert(stmt->as.import.module_name.type == TOKEN_STRING_LITERAL);
    assert(stmt->as.import.module_name.line == 1);
    assert(stmt->as.import.module_name.length == 9);

    /* Verify namespace token info */
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->type == TOKEN_IDENTIFIER);
    assert(stmt->as.import.namespace->line == 1);
    assert(stmt->as.import.namespace->length == 5);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test import followed by function to ensure parser continues correctly */
static void test_parse_import_followed_by_code()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "import \"math\" as m\n"
        "\n"
        "fn main(): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    /* First statement is import */
    assert(module->statements[0]->type == STMT_IMPORT);
    assert(module->statements[0]->as.import.namespace != NULL);

    /* Second statement is function */
    assert(module->statements[1]->type == STMT_FUNCTION);
    assert(strncmp(module->statements[1]->as.function.name.start, "main", 4) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Main entry point for namespace parser tests */
static void test_parser_namespace_main()
{
    TEST_SECTION("Parser Namespace Tests");
    TEST_RUN("parse_import_basic", test_parse_import_basic);
    TEST_RUN("parse_import_as_namespace", test_parse_import_as_namespace);
    TEST_RUN("parse_namespace_with_numbers", test_parse_namespace_with_numbers);
    TEST_RUN("parse_namespace_underscore_start", test_parse_namespace_underscore_start);
    TEST_RUN("parse_namespace_single_letter", test_parse_namespace_single_letter);
    TEST_RUN("parse_namespace_long_name", test_parse_namespace_long_name);
    TEST_RUN("parse_multiple_namespace_styles", test_parse_multiple_namespace_styles);
    TEST_RUN("parse_invalid_namespace_keyword_var", test_parse_invalid_namespace_keyword_var);
    TEST_RUN("parse_invalid_namespace_keyword_fn", test_parse_invalid_namespace_keyword_fn);
    TEST_RUN("parse_invalid_namespace_keyword_return", test_parse_invalid_namespace_keyword_return);
    TEST_RUN("parse_invalid_namespace_keyword_import", test_parse_invalid_namespace_keyword_import);
    TEST_RUN("parse_invalid_namespace_keyword_str", test_parse_invalid_namespace_keyword_str);
    TEST_RUN("parse_invalid_namespace_keyword_int", test_parse_invalid_namespace_keyword_int);
    TEST_RUN("parse_invalid_missing_namespace", test_parse_invalid_missing_namespace);
    TEST_RUN("parse_invalid_namespace_starts_with_number", test_parse_invalid_namespace_starts_with_number);
    TEST_RUN("parse_import_ast_token_info", test_parse_import_ast_token_info);
    TEST_RUN("parse_import_followed_by_code", test_parse_import_followed_by_code);
}
