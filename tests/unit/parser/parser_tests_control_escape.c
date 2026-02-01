// parser_tests_control_escape.c
// Escape sequences and special characters in interpolation tests

static void test_interpolated_string_with_string_literal_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test string literal directly in interpolation: $"Result: {"hello"}" */
    const char *source = "print($\"Result: {\"hello\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "Result: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Result: ") == 0);
    /* Part 1: "hello" string literal expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(arg->as.interpol.parts[1]->as.literal.type->kind == TYPE_STRING);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "hello") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_func_call_string_arg()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test function call with string argument: $"Result: {func("arg")}" */
    const char *source = "print($\"Result: {func(\"arg\")}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "Result: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Result: ") == 0);
    /* Part 1: func("arg") call expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_CALL);
    assert(strcmp(arg->as.interpol.parts[1]->as.call.callee->as.variable.name.start, "func") == 0);
    assert(arg->as.interpol.parts[1]->as.call.arg_count == 1);
    assert(arg->as.interpol.parts[1]->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.call.arguments[0]->as.literal.value.string_value, "arg") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_newline_escape_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test \n escape sequence in nested string: $"Result: {"\n"}" */
    const char *source = "print($\"Result: {\"\\n\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "\n" string literal expression - should contain actual newline */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_tab_escape_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test \t escape sequence in nested string: $"Tab: {"\t"}" */
    const char *source = "print($\"Tab: {\"\\t\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "\t" string literal expression - should contain actual tab */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "\t") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_backslash_escape_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test \\ escape sequence in nested string: $"Slash: {"\\"}" */
    const char *source = "print($\"Slash: {\"\\\\\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "\\" string literal expression - should contain single backslash */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "\\") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_carriage_return_escape_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test \r escape sequence in nested string: $"CR: {"\r"}" */
    const char *source = "print($\"CR: {\"\\r\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "\r" string literal expression - should contain actual carriage return */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "\r") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_empty_string_in_braces()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test empty string in braces: $"Empty: {""}" */
    const char *source = "print($\"Empty: {\"\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "" empty string literal */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_nested_parens()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test nested parentheses in expression: $"Result: {((x + y) * 2)}" */
    const char *source = "print($\"Result: {((x + y) * 2)}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: ((x + y) * 2) expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[1]->as.binary.operator == TOKEN_STAR);
    /* Left side should be (x + y) */
    assert(arg->as.interpol.parts[1]->as.binary.left->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[1]->as.binary.left->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_multiple_string_literals()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test multiple string literals concatenated: $"Result: {"a" + "b" + "c"}" */
    const char *source = "print($\"Result: {\"a\" + \"b\" + \"c\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "a" + "b" + "c" binary expression chain */
    assert(arg->as.interpol.parts[1]->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[1]->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_string_method_on_literal()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test string method on literal: $"Upper: {"test".toUpper()}" */
    const char *source = "print($\"Upper: {\"test\".toUpper()}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "test".toUpper() call expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_CALL);
    assert(arg->as.interpol.parts[1]->as.call.callee->type == EXPR_MEMBER);
    assert(strcmp(arg->as.interpol.parts[1]->as.call.callee->as.member.member_name.start, "toUpper") == 0);
    /* The object of the member access should be "test" literal */
    assert(arg->as.interpol.parts[1]->as.call.callee->as.member.object->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.call.callee->as.member.object->as.literal.value.string_value, "test") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_complex_escape_sequence()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test complex escapes: $"Data: {"a\tb\nc"}" */
    const char *source = "print($\"Data: {\"a\\tb\\nc\"}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 1: "a\tb\nc" string with escapes */
    assert(arg->as.interpol.parts[1]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[1]->as.literal.value.string_value, "a\tb\nc") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_braces_escape()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test escaped braces {{ and }}: $"Braces: {{curly}}" */
    const char *source = "print($\"Braces: {{curly}}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    /* The whole string becomes a single literal since {{ and }} escape to { and } */
    assert(arg->as.interpol.part_count == 1);
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Braces: {curly}") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
