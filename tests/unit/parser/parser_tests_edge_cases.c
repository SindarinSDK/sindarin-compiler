// tests/parser_tests_edge_cases.c
// Edge case tests for the parser

/* ============================================================================
 * Expression Precedence Tests
 * ============================================================================ */

static void test_parse_precedence_mul_over_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1 + 2 * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    /* Should parse as 1 + (2 * 3), so top-level is + */
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_parens_override(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = (1 + 2) * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (1 + 2) * 3, so top-level is * */
    assert(init->as.binary.operator == TOKEN_STAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_comparison_lower(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = 1 + 2 > 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (1 + 2) > 3, so top-level is > */
    assert(init->as.binary.operator == TOKEN_GREATER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_and_lower_than_comparison(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a > b and c > d\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (a > b) and (c > d), so top-level is and */
    assert(init->as.binary.operator == TOKEN_AND);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_or_lower_than_and(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a and b or c and d\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (a and b) or (c and d), so top-level is or */
    assert(init->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_unary_precedence(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = -2 * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (-2) * 3, top-level is * */
    assert(init->as.binary.operator == TOKEN_STAR);
    assert(init->as.binary.left->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_double_negation(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = --5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);
    assert(init->as.unary.operator == TOKEN_MINUS);
    assert(init->as.unary.operand->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_not_and_comparison(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = not a > b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);
    assert(init->as.unary.operator == TOKEN_NOT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Literal Tests
 * ============================================================================ */

static void test_parse_int_literal_zero(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.int_value == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_int_literal_negative(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = -42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_double_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: double = 3.14\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_DOUBLE);
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_bool_true(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.bool_value == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_bool_false(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = false\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.bool_value == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_char_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: char = 'a'\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_CHAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_string_empty(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: str = \"\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(strcmp(init->as.literal.value.string_value, "") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_string_with_escapes(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: str = \"hello\\nworld\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(strstr(init->as.literal.value.string_value, "\n") != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Statement Tests
 * ============================================================================ */

static void test_parse_multiple_var_decls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1\nvar y: int = 2\nvar z: int = 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 3);
    for (int i = 0; i < 3; i++) {
        assert(module->statements[i]->type == STMT_VAR_DECL);
    }

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nested_if(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if a =>\n"
        "  if b =>\n"
        "    print(\"nested\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK);
    /* The inner if is inside the then_branch */
    Stmt *inner = if_stmt->as.if_stmt.then_branch->as.block.statements[0];
    assert(inner->type == STMT_IF);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_if_elif_else(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if a =>\n"
        "  print(\"a\")\n"
        "elif b =>\n"
        "  print(\"b\")\n"
        "else =>\n"
        "  print(\"c\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    /* Check that elif creates a nested if */
    assert(if_stmt->as.if_stmt.else_branch != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_while_loop(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while x > 0 =>\n"
        "  x = x - 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *while_stmt = module->statements[0];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.condition->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_for_range(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "for i in 0..10 =>\n"
        "  print(i)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *for_stmt = module->statements[0];
    assert(for_stmt->type == STMT_FOR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_return_value(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): int =>\n"
        "  return 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    Stmt *ret = fn->as.function.body[0];
    assert(ret->type == STMT_RETURN);
    assert(ret->as.return_stmt.value != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_return_void(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): void =>\n"
        "  return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    Stmt *ret = fn->as.function.body[0];
    assert(ret->type == STMT_RETURN);
    assert(ret->as.return_stmt.value == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_break_statement(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while true =>\n"
        "  break\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *while_stmt = module->statements[0];
    Stmt *body = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(body->type == STMT_BREAK);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_continue_statement(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while true =>\n"
        "  continue\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *while_stmt = module->statements[0];
    Stmt *body = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(body->type == STMT_CONTINUE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Function Declaration Tests
 * ============================================================================ */

static void test_parse_function_one_param(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(x: int): int =>\n"
        "  return x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_multiple_params(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn add(a: int, b: int): int =>\n"
        "  return a + b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 2);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_void_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn greet(): void =>\n"
        "  print(\"hello\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_VOID);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_string_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn getName(): str =>\n"
        "  return \"test\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_STRING);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_bool_return(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn isValid(): bool =>\n"
        "  return true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.return_type->kind == TYPE_BOOL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_function_multiple_statements(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): void =>\n"
        "  var x: int = 1\n"
        "  var y: int = 2\n"
        "  print(x + y)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    assert(fn->as.function.body_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Call Expression Tests
 * ============================================================================ */

static void test_parse_call_no_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_one_arg(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(42)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_multiple_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(1, 2, 3)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_call_expression_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(1 + 2, x * y)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 2);
    assert(call->as.call.arguments[0]->type == EXPR_BINARY);
    assert(call->as.call.arguments[1]->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nested_calls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "foo(bar(baz()))\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arguments[0]->type == EXPR_CALL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Type Tests
 * ============================================================================ */

static void test_parse_array_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: [int] = [1, 2, 3]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_pointer_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: *int = null\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_POINTER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nullable_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int? = null\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_NULLABLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_long_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: long = 100000000000\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_LONG);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_byte_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: byte = 255\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_BYTE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Complex Expression Tests
 * ============================================================================ */

static void test_parse_chained_method_calls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x.foo().bar().baz()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_array_indexing(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x[0]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.expression.expression;
    assert(expr->type == EXPR_INDEX);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_compound_assignment(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x += 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_parser_edge_cases_main(void)
{
    TEST_SECTION("Parser - Expression Precedence");
    TEST_RUN("precedence_mul_over_add", test_parse_precedence_mul_over_add);
    TEST_RUN("precedence_parens_override", test_parse_precedence_parens_override);
    TEST_RUN("precedence_comparison_lower", test_parse_precedence_comparison_lower);
    TEST_RUN("precedence_and_lower_than_comparison", test_parse_precedence_and_lower_than_comparison);
    TEST_RUN("precedence_or_lower_than_and", test_parse_precedence_or_lower_than_and);
    TEST_RUN("unary_precedence", test_parse_unary_precedence);
    TEST_RUN("double_negation", test_parse_double_negation);
    TEST_RUN("not_and_comparison", test_parse_not_and_comparison);

    TEST_SECTION("Parser - Literals");
    TEST_RUN("int_literal_zero", test_parse_int_literal_zero);
    TEST_RUN("int_literal_negative", test_parse_int_literal_negative);
    TEST_RUN("double_literal", test_parse_double_literal);
    TEST_RUN("bool_true", test_parse_bool_true);
    TEST_RUN("bool_false", test_parse_bool_false);
    TEST_RUN("char_literal", test_parse_char_literal);
    TEST_RUN("string_empty", test_parse_string_empty);
    TEST_RUN("string_with_escapes", test_parse_string_with_escapes);

    TEST_SECTION("Parser - Statements");
    TEST_RUN("multiple_var_decls", test_parse_multiple_var_decls);
    TEST_RUN("nested_if", test_parse_nested_if);
    TEST_RUN("if_elif_else", test_parse_if_elif_else);
    TEST_RUN("while_loop", test_parse_while_loop);
    TEST_RUN("for_range", test_parse_for_range);
    TEST_RUN("return_value", test_parse_return_value);
    TEST_RUN("return_void", test_parse_return_void);
    TEST_RUN("break_statement", test_parse_break_statement);
    TEST_RUN("continue_statement", test_parse_continue_statement);

    TEST_SECTION("Parser - Function Declarations");
    TEST_RUN("function_one_param", test_parse_function_one_param);
    TEST_RUN("function_multiple_params", test_parse_function_multiple_params);
    TEST_RUN("function_void_return", test_parse_function_void_return);
    TEST_RUN("function_string_return", test_parse_function_string_return);
    TEST_RUN("function_bool_return", test_parse_function_bool_return);
    TEST_RUN("function_multiple_statements", test_parse_function_multiple_statements);

    TEST_SECTION("Parser - Call Expressions");
    TEST_RUN("call_no_args", test_parse_call_no_args);
    TEST_RUN("call_one_arg", test_parse_call_one_arg);
    TEST_RUN("call_multiple_args", test_parse_call_multiple_args);
    TEST_RUN("call_expression_args", test_parse_call_expression_args);
    TEST_RUN("nested_calls", test_parse_nested_calls);

    TEST_SECTION("Parser - Types");
    TEST_RUN("array_type", test_parse_array_type);
    TEST_RUN("pointer_type", test_parse_pointer_type);
    TEST_RUN("nullable_type", test_parse_nullable_type);
    TEST_RUN("long_type", test_parse_long_type);
    TEST_RUN("byte_type", test_parse_byte_type);

    TEST_SECTION("Parser - Complex Expressions");
    TEST_RUN("chained_method_calls", test_parse_chained_method_calls);
    TEST_RUN("array_indexing", test_parse_array_indexing);
    TEST_RUN("compound_assignment", test_parse_compound_assignment);
}
