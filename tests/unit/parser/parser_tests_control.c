// tests/parser_tests_control.c
// Control flow parser tests (while, for, interpolated_string, literals, recursive)

static void test_while_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while i < 10 =>\n"
        "  i = i + 1\n"
        "  print(i)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *while_stmt = module->statements[0];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.condition->type == EXPR_BINARY);
    assert(while_stmt->as.while_stmt.condition->as.binary.operator == TOKEN_LESS);
    assert(strcmp(while_stmt->as.while_stmt.condition->as.binary.left->as.variable.name.start, "i") == 0);
    assert(while_stmt->as.while_stmt.condition->as.binary.right->as.literal.value.int_value == 10);
    assert(while_stmt->as.while_stmt.body->type == STMT_BLOCK);
    assert(while_stmt->as.while_stmt.body->as.block.count == 2);
    Stmt *assign = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(assign->type == STMT_EXPR);
    assert(assign->as.expression.expression->type == EXPR_ASSIGN);
    assert(strcmp(assign->as.expression.expression->as.assign.name.start, "i") == 0);
    assert(assign->as.expression.expression->as.assign.value->type == EXPR_BINARY);
    assert(assign->as.expression.expression->as.assign.value->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_for_loop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "for var j:int = 0; j < 5; j++ =>\n"
        "  print(j)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *for_stmt = module->statements[0];
    assert(for_stmt->type == STMT_FOR);
    assert(for_stmt->as.for_stmt.initializer->type == STMT_VAR_DECL);
    assert(strcmp(for_stmt->as.for_stmt.initializer->as.var_decl.name.start, "j") == 0);
    assert(for_stmt->as.for_stmt.initializer->as.var_decl.type->kind == TYPE_INT);
    assert(for_stmt->as.for_stmt.initializer->as.var_decl.initializer->as.literal.value.int_value == 0);
    assert(for_stmt->as.for_stmt.condition->type == EXPR_BINARY);
    assert(for_stmt->as.for_stmt.condition->as.binary.operator == TOKEN_LESS);
    assert(for_stmt->as.for_stmt.increment->type == EXPR_INCREMENT);
    assert(strcmp(for_stmt->as.for_stmt.increment->as.operand->as.variable.name.start, "j") == 0);
    assert(for_stmt->as.for_stmt.body->type == STMT_BLOCK);
    assert(for_stmt->as.for_stmt.body->as.block.count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "print($\"Value is {x} and {y * 2}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 5);
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Value is ") == 0);
    assert(arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(arg->as.interpol.parts[1]->as.variable.name.start, "x") == 0);
    assert(arg->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[2]->as.literal.value.string_value, " and ") == 0);
    assert(arg->as.interpol.parts[3]->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[3]->as.binary.operator == TOKEN_STAR);
    assert(strcmp(arg->as.interpol.parts[3]->as.binary.left->as.variable.name.start, "y") == 0);
    assert(arg->as.interpol.parts[3]->as.binary.right->as.literal.value.int_value == 2);
    assert(arg->as.interpol.parts[4]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[4]->as.literal.value.string_value, "\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_literal_types_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var i:int = 42\n"
        "var l:long = 123456789012\n"
        "var d:double = 3.14159\n"
        "var c:char = 'A'\n"
        "var b:bool = true\n"
        "var s:str = \"hello\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 6);

    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_VAR_DECL);
    assert(stmt1->as.var_decl.type->kind == TYPE_INT);
    assert(stmt1->as.var_decl.initializer->as.literal.value.int_value == 42);

    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_VAR_DECL);
    assert(stmt2->as.var_decl.type->kind == TYPE_LONG);
    assert(stmt2->as.var_decl.initializer->as.literal.value.int_value == 123456789012LL);

    Stmt *stmt3 = module->statements[2];
    assert(stmt3->type == STMT_VAR_DECL);
    assert(stmt3->as.var_decl.type->kind == TYPE_DOUBLE);
    assert(stmt3->as.var_decl.initializer->as.literal.value.double_value == 3.14159);

    Stmt *stmt4 = module->statements[3];
    assert(stmt4->type == STMT_VAR_DECL);
    assert(stmt4->as.var_decl.type->kind == TYPE_CHAR);
    assert(stmt4->as.var_decl.initializer->as.literal.value.char_value == 'A');

    Stmt *stmt5 = module->statements[4];
    assert(stmt5->type == STMT_VAR_DECL);
    assert(stmt5->as.var_decl.type->kind == TYPE_BOOL);
    assert(stmt5->as.var_decl.initializer->as.literal.value.bool_value == 1);

    Stmt *stmt6 = module->statements[5];
    assert(stmt6->type == STMT_VAR_DECL);
    assert(stmt6->as.var_decl.type->kind == TYPE_STRING);
    assert(strcmp(stmt6->as.var_decl.initializer->as.literal.value.string_value, "hello") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_recursive_function_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn factorial(n:int):int =>\n"
        "  if n <= 1 =>\n"
        "    return 1\n"
        "  return n * factorial(n - 1)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "factorial") == 0);
    assert(fn->as.function.param_count == 1);
    assert(strcmp(fn->as.function.params[0].name.start, "n") == 0);
    assert(fn->as.function.params[0].type->kind == TYPE_INT);
    assert(fn->as.function.return_type->kind == TYPE_INT);
    assert(fn->as.function.body_count == 2);
    Stmt *if_stmt = fn->as.function.body[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition->as.binary.operator == TOKEN_LESS_EQUAL);
    assert(if_stmt->as.if_stmt.then_branch->as.block.count == 1);
    assert(if_stmt->as.if_stmt.then_branch->as.block.statements[0]->type == STMT_RETURN);
    Stmt *return_stmt = fn->as.function.body[1];
    assert(return_stmt->type == STMT_RETURN);
    assert(return_stmt->as.return_stmt.value->type == EXPR_BINARY);
    assert(return_stmt->as.return_stmt.value->as.binary.operator == TOKEN_STAR);
    assert(return_stmt->as.return_stmt.value->as.binary.right->type == EXPR_CALL);
    assert(strcmp(return_stmt->as.return_stmt.value->as.binary.right->as.call.callee->as.variable.name.start, "factorial") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_postfix_decrement_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test postfix decrement operator inside string interpolation: $"{x--}"
     * NOTE: Sindarin only supports POSTFIX increment/decrement, not prefix.
     * Prefix --x is NOT supported by the language. */
    const char *source = "print($\"Result: {x--}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 3);
    /* Part 0: "Result: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Result: ") == 0);
    /* Part 1: x-- postfix decrement expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_DECREMENT);
    assert(arg->as.interpol.parts[1]->as.operand->type == EXPR_VARIABLE);
    assert(strncmp(arg->as.interpol.parts[1]->as.operand->as.variable.name.start, "x", 1) == 0);
    /* Part 2: "\n" literal */
    assert(arg->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[2]->as.literal.value.string_value, "\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_postfix_increment_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test postfix increment operator inside string interpolation: $"{count++}"
     * NOTE: Sindarin only supports POSTFIX increment/decrement, not prefix. */
    const char *source = "print($\"Count: {count++}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "Count: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    /* Part 1: count++ postfix increment expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_INCREMENT);
    assert(arg->as.interpol.parts[1]->as.operand->type == EXPR_VARIABLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_interpolated_string_with_unary_negate_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    /* Test unary negate inside string interpolation: $"{-x}" */
    const char *source = "print($\"Negated: {-x}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 2);
    /* Part 0: "Negated: " literal */
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    /* Part 1: -x unary negate expression */
    assert(arg->as.interpol.parts[1]->type == EXPR_UNARY);
    assert(arg->as.interpol.parts[1]->as.unary.operator == TOKEN_MINUS);
    assert(arg->as.interpol.parts[1]->as.unary.operand->type == EXPR_VARIABLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ====== Tests for escaped quotes and escape sequences in interpolation ====== */

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

/* ====== Tests for nested interpolated strings ====== */

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

void test_parser_control_main()
{
    TEST_SECTION("Parser Control Flow Tests");

    TEST_RUN("while_loop_parsing", test_while_loop_parsing);
    TEST_RUN("for_loop_parsing", test_for_loop_parsing);
    TEST_RUN("interpolated_string_parsing", test_interpolated_string_parsing);
    TEST_RUN("interpolated_string_with_postfix_decrement_parsing", test_interpolated_string_with_postfix_decrement_parsing);
    TEST_RUN("interpolated_string_with_postfix_increment_parsing", test_interpolated_string_with_postfix_increment_parsing);
    TEST_RUN("interpolated_string_with_unary_negate_parsing", test_interpolated_string_with_unary_negate_parsing);
    TEST_RUN("literal_types_parsing", test_literal_types_parsing);
    TEST_RUN("recursive_function_parsing", test_recursive_function_parsing);
    TEST_RUN("interpolated_string_with_string_literal_in_braces", test_interpolated_string_with_string_literal_in_braces);
    TEST_RUN("interpolated_string_with_func_call_string_arg", test_interpolated_string_with_func_call_string_arg);
    TEST_RUN("interpolated_string_with_newline_escape_in_braces", test_interpolated_string_with_newline_escape_in_braces);
    TEST_RUN("interpolated_string_with_tab_escape_in_braces", test_interpolated_string_with_tab_escape_in_braces);
    TEST_RUN("interpolated_string_with_backslash_escape_in_braces", test_interpolated_string_with_backslash_escape_in_braces);
    TEST_RUN("interpolated_string_with_carriage_return_escape_in_braces", test_interpolated_string_with_carriage_return_escape_in_braces);
    TEST_RUN("interpolated_string_with_empty_string_in_braces", test_interpolated_string_with_empty_string_in_braces);
    TEST_RUN("interpolated_string_with_nested_parens", test_interpolated_string_with_nested_parens);
    TEST_RUN("interpolated_string_with_multiple_string_literals", test_interpolated_string_with_multiple_string_literals);
    TEST_RUN("interpolated_string_with_string_method_on_literal", test_interpolated_string_with_string_method_on_literal);
    TEST_RUN("interpolated_string_with_complex_escape_sequence", test_interpolated_string_with_complex_escape_sequence);
    TEST_RUN("interpolated_string_with_braces_escape", test_interpolated_string_with_braces_escape);
    TEST_RUN("nested_interpolated_string_basic", test_nested_interpolated_string_basic);
    TEST_RUN("nested_interpolated_string_with_expression", test_nested_interpolated_string_with_expression);
    TEST_RUN("nested_interpolated_string_double_nested", test_nested_interpolated_string_double_nested);
    TEST_RUN("nested_interpolated_string_with_func_call", test_nested_interpolated_string_with_func_call);
    TEST_RUN("nested_interpolated_string_adjacent", test_nested_interpolated_string_adjacent);
    TEST_RUN("interpolated_string_with_format_specifier", test_interpolated_string_with_format_specifier);
    TEST_RUN("interpolated_string_with_format_specifier_float", test_interpolated_string_with_format_specifier_float);
    TEST_RUN("interpolated_string_with_format_specifier_and_expr", test_interpolated_string_with_format_specifier_and_expr);
    TEST_RUN("format_specifier_not_detected_in_nested", test_format_specifier_not_detected_in_nested);
}
