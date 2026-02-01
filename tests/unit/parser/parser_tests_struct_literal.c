// parser_tests_struct_literal.c
// Struct literal and field access parser tests

/* ============================================================================
 * Struct Literal Tests
 * ============================================================================ */

static void test_struct_literal_empty()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "var p: Point = Point {}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);  /* struct decl + var decl */

    /* Check the var declaration has a struct literal initializer */
    Stmt *var_stmt = module->statements[1];
    assert(var_stmt->type == STMT_VAR_DECL);
    assert(var_stmt->as.var_decl.initializer != NULL);
    assert(var_stmt->as.var_decl.initializer->type == EXPR_STRUCT_LITERAL);
    assert(strncmp(var_stmt->as.var_decl.initializer->as.struct_literal.struct_name.start,
                   "Point", 5) == 0);
    assert(var_stmt->as.var_decl.initializer->as.struct_literal.field_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_with_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "var p: Point = Point { x: 1.0, y: 2.0 }\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the struct literal */
    Stmt *var_stmt = module->statements[1];
    assert(var_stmt->type == STMT_VAR_DECL);
    Expr *init = var_stmt->as.var_decl.initializer;
    assert(init != NULL);
    assert(init->type == EXPR_STRUCT_LITERAL);
    assert(init->as.struct_literal.field_count == 2);

    /* Check field names and values */
    assert(strncmp(init->as.struct_literal.fields[0].name.start, "x", 1) == 0);
    assert(init->as.struct_literal.fields[0].value->type == EXPR_LITERAL);

    assert(strncmp(init->as.struct_literal.fields[1].name.start, "y", 1) == 0);
    assert(init->as.struct_literal.fields[1].value->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_partial_init()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    timeout: int\n"
        "    retries: int\n"
        "    verbose: bool\n"
        "\n"
        "var cfg: Config = Config { timeout: 30 }\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    Stmt *var_stmt = module->statements[1];
    Expr *init = var_stmt->as.var_decl.initializer;
    assert(init->type == EXPR_STRUCT_LITERAL);
    assert(init->as.struct_literal.field_count == 1);  /* Only one field specified */
    assert(strncmp(init->as.struct_literal.fields[0].name.start, "timeout", 7) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Field Access Tests
 * ============================================================================ */

static void test_field_access_simple()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "fn test(p: Point): double =>\n"
        "    return p.x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the function body has a return statement with member access */
    Stmt *fn_stmt = module->statements[1];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);
    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_RETURN);
    assert(body_stmt->as.return_stmt.value->type == EXPR_MEMBER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_access_nested()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "struct Rectangle =>\n"
        "    origin: Point\n"
        "    width: double\n"
        "    height: double\n"
        "\n"
        "fn test(r: Rectangle): double =>\n"
        "    return r.origin.x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 3);

    /* Check the function body has nested member access (EXPR_MEMBER) */
    Stmt *fn_stmt = module->statements[2];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);
    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_RETURN);

    /* r.origin.x should be EXPR_MEMBER with an EXPR_MEMBER object */
    Expr *member_expr = body_stmt->as.return_stmt.value;
    assert(member_expr->type == EXPR_MEMBER);
    assert(member_expr->as.member.object->type == EXPR_MEMBER);  /* r.origin is also EXPR_MEMBER */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_assignment()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "fn test(p: Point): void =>\n"
        "    p.x = 5.0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the function body has a member assignment expression */
    Stmt *fn_stmt = module->statements[1];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);

    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_EXPR);
    assert(body_stmt->as.expression.expression->type == EXPR_MEMBER_ASSIGN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_assignment_nested()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "struct Rectangle =>\n"
        "    origin: Point\n"
        "    width: double\n"
        "\n"
        "fn test(r: Rectangle): void =>\n"
        "    r.origin.x = 5.0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 3);

    /* Check the function body has a nested member assignment expression */
    Stmt *fn_stmt = module->statements[2];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);

    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_EXPR);
    assert(body_stmt->as.expression.expression->type == EXPR_MEMBER_ASSIGN);

    /* The object should be a member expression (r.origin) */
    Expr *member_assign = body_stmt->as.expression.expression;
    assert(member_assign->as.member_assign.object->type == EXPR_MEMBER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Error Handling Tests for Struct Literals
 * ============================================================================ */

static void test_struct_literal_missing_colon()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "\n"
        "var p: Point = Point { x 1.0 }\n";  /* Missing colon */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_invalid_field_name()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "\n"
        "var p: Point = Point { 123: 1.0 }\n";  /* Number instead of field name */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
