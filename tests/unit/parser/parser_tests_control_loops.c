// parser_tests_control_loops.c
// Loop, literal, recursive function, and basic interpolated string tests

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
