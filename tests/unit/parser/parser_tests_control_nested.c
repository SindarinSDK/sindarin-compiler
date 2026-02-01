// parser_tests_control_nested.c
// Nested interpolated strings and format specifier tests

static void test_nested_interpolated_string_basic()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test basic nested interpolation: $"outer {$"inner {x}"}" */
    const char *source = "print($\"outer {$\"inner {x}\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "outer " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "outer ") == 0);
    /* Part 1: nested interpolated string $"inner {x}" */
    assert(arg->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.parts[1]->as.interpol.part_count == 2);
    /* Inner Part 0: "inner " literal */
    assert(arg->as.interpol.parts[1]->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.interpol.parts[0]->as.literal.value.string_value, "inner ") == 0);
    /* Inner Part 1: x variable */
    assert(arg->as.interpol.parts[1]->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strncmp(arg->as.interpol.parts[1]->as.interpol.parts[1]->as.variable.name.start, "x", 1) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_nested_interpolated_string_with_expression()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test nested interpolation with expression: $"Result: {$"Value: {x + 1}"}" */
    const char *source = "print($\"Result: {$\"Value: {x + 1}\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: nested interpolated string */
    assert(arg->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.parts[1]->as.interpol.part_count == 2);
    /* Inner Part 1: x + 1 binary expression */
    assert(arg->as.interpol.parts[1]->as.interpol.parts[1]->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[1]->as.interpol.parts[1]->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_nested_interpolated_string_double_nested()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test double nesting: $"a{$"b{$"c{x}"}"}d" */
    const char *source = "print($\"a{$\"b{$\"c{x}\"}\"}d\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 3);  /* "a", nested, "d" */
    /* Part 0: "a" literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "a") == 0);
    /* Part 1: first nested level */
    assert(arg->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.parts[1]->as.interpol.part_count == 2);  /* "b", nested */
    /* Part 1 -> Part 1: second nested level */
    assert(arg->as.interpol.parts[1]->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    /* Part 2: "d" literal */
    assert(arg->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[2]->as.literal.value.string_value, "d") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_nested_interpolated_string_with_func_call()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test nested interpolation with function: $"outer {format($"inner {x}")}" */
    const char *source = "print($\"outer {format($\"inner {x}\")}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: format($"inner {x}") call expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_CALL);
    assert(strcmp(arg->as.interpol.parts[1]->as.call.callee->as.variable.name.start, "format") == 0);
    assert(arg->as.interpol.parts[1]->as.call.arg_count == 1);
    /* The argument to format() is the nested interpolated string */
    assert(arg->as.interpol.parts[1]->as.call.arguments[0]->type == EXPR_INTERPOLATED);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_nested_interpolated_string_adjacent()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test adjacent nested strings: $"a{$"x"}{$"y"}b" */
    const char *source = "print($\"a{$\"x\"}{$\"y\"}b\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 4);  /* "a", $"x", $"y", "b" */
    /* Part 0: "a" literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    /* Part 1: $"x" nested interp */
    assert(arg->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    /* Part 2: $"y" nested interp */
    assert(arg->as.interpol.parts[2]->type == EXPR_INTERPOLATED);
    /* Part 3: "b" literal */
    assert(arg->as.interpol.parts[3]->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ====== Tests for format specifiers in interpolation ====== */

static void test_interpolated_string_with_format_specifier()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test format specifier: $"Value: {x:05d}" */
    const char *source = "print($\"Value: {x:05d}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "Value: " literal - no format */
    assert(arg->as.interpol.format_specs[0] == NULL);
    /* Part 1: x variable with format "05d" */
    assert(arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strncmp(arg->as.interpol.parts[1]->as.variable.name.start, "x", 1) == 0);
    assert(arg->as.interpol.format_specs[1] != NULL);
    assert(strcmp(arg->as.interpol.format_specs[1], "05d") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_format_specifier_float()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test float format: $"Pi: {pi:.2f}" */
    const char *source = "print($\"Pi: {pi:.2f}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: pi variable with format ".2f" */
    assert(arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(arg->as.interpol.format_specs[1] != NULL);
    assert(strcmp(arg->as.interpol.format_specs[1], ".2f") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_format_specifier_and_expr()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test format on expression: $"Result: {x + 1:x}" */
    const char *source = "print($\"Result: {x + 1:x}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: x + 1 expression with format "x" */
    assert(arg->as.interpol.parts[1]->type == EXPR_BINARY);
    assert(arg->as.interpol.format_specs[1] != NULL);
    assert(strcmp(arg->as.interpol.format_specs[1], "x") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_format_specifier_not_detected_in_nested()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test that colon in nested string is not a format specifier: $"A: {$"B: {x}"}" */
    const char *source = "print($\"A: {$\"B: {x}\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "A: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    /* Part 1: nested interp string - no format specifier (colon is inside nested) */
    assert(arg->as.interpol.parts[1]->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.format_specs[1] == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
