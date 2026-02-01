// tests/unit/parser/parser_tests_basic_misc.c
// Opaque type, pragma, as_ref, and additional variadic parser tests

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

static void test_parser_basic_misc_main()
{
    TEST_SECTION("Parser Basic Misc Tests");
    TEST_RUN("opaque_type_decl_parsing", test_opaque_type_decl_parsing);
    TEST_RUN("opaque_type_in_function_param", test_opaque_type_in_function_param);
    TEST_RUN("pragma_include_parsing", test_pragma_include_parsing);
    TEST_RUN("pragma_link_parsing", test_pragma_link_parsing);
    TEST_RUN("multiple_pragmas_parsing", test_multiple_pragmas_parsing);
    TEST_RUN("as_ref_parameter_parsing", test_as_ref_parameter_parsing);
    TEST_RUN("variadic_with_multiple_fixed_params_parsing", test_variadic_with_multiple_fixed_params_parsing);
}
