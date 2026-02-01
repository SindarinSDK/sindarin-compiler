// tests/unit/parser/parser_tests_basic_core.c
// Core basic parser tests (empty, var_decl, function_no_params, if_statement)

static void test_empty_program_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "");

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 0);
    assert(strcmp(module->filename, "test.sn") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_var_decl_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x:int = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(strcmp(stmt->as.var_decl.name.start, "x") == 0);
    assert(stmt->as.var_decl.type->kind == TYPE_INT);
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.int_value == 42);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_function_no_params_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "main") == 0);
    assert(fn->as.function.param_count == 0);
    assert(fn->as.function.return_type->kind == TYPE_VOID);
    assert(fn->as.function.body_count == 1);
    Stmt *print_stmt = fn->as.function.body[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.literal.value.string_value, "hello\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_if_statement_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if x > 0 =>\n"
        "  print(\"positive\\n\")\n"
        "else =>\n"
        "  print(\"non-positive\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
        if (module->count > 0)
        {
            ast_print_stmt(&arena, module->statements[0], 0);
            Stmt *if_stmt = module->statements[0];
            if (if_stmt->type == STMT_IF)
            {
                if (if_stmt->as.if_stmt.condition->type == EXPR_BINARY)
                {
                }
                if (if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
                {
                }
                if (if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
                {
                }
            }
        }
        else
        {
            DEBUG_WARNING("No statements parsed in module.");
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition->type == EXPR_BINARY);
    assert(if_stmt->as.if_stmt.condition->as.binary.operator == TOKEN_GREATER);
    assert(strcmp(if_stmt->as.if_stmt.condition->as.binary.left->as.variable.name.start, "x") == 0);
    assert(if_stmt->as.if_stmt.condition->as.binary.right->as.literal.value.int_value == 0);
    assert(if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.then_branch->as.block.count == 1);
    assert(if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.else_branch->as.block.count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_basic_core_main()
{
    TEST_SECTION("Parser Basic Core Tests");
    TEST_RUN("empty_program_parsing", test_empty_program_parsing);
    TEST_RUN("var_decl_parsing", test_var_decl_parsing);
    TEST_RUN("function_no_params_parsing", test_function_no_params_parsing);
    TEST_RUN("if_statement_parsing", test_if_statement_parsing);
}
