// tests/parser_tests_program.c
// Full program and simple program parser tests

static void test_full_program_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn factorial(n: int): int =>\n"
        "  print($\"factorial: n={n}\\n\")\n"
        "  if n <= 1 =>\n"
        "    print($\"factorial: n <= 1 returning 1\\n\")\n"
        "    return 1\n"
        "  var j: int = n * factorial(n - 1)\n"
        "  print($\"factorial: j={j}\\n\")\n"
        "  return j\n"
        "fn is_prime(num: int): bool =>\n"
        "  if num <= 1 =>\n"
        "    print($\"is_prime: num={num}\\n\")\n"
        "    return false\n"
        "  var i: int = 2\n"
        "  print($\"is_prime: i={i}\\n\")\n"
        "  while i * i <= num =>\n"
        "    if num % i == 0 =>\n"
        "      print($\"is_prime: num % i == 0, returning false\\n\")\n"
        "      return false\n"
        "    i = i + 1\n"
        "    print($\"is_prime: i={i} (after increment)\\n\")\n"
        "  return true\n"
        "fn repeat_string(text: str, count: int): str =>\n"
        "  var result: str = \"\"\n"
        "  for var j: int = 0; j < count; j++ =>\n"
        "    print($\"repeat_string: j={j}\\n\")\n"
        "    print($\"repeat_string: count={count}\\n\")\n"
        "    result = result + text\n"
        "  return result\n"
        "fn main(): void =>\n"
        "  print(\"Starting main method ... \\n\")\n"
        "  var num: int = 5\n"
        "  var fact: int = factorial(num)\n"
        "  print($\"Factorial of {num} is {fact}\\n\")\n"
        "  if is_prime(7) =>\n"
        "    print(\"7 is prime\\n\")\n"
        "  else =>\n"
        "    print(\"7 is not prime\\n\")\n"
        "  var repeated: str = repeat_string(\"hello \", 3)\n"
        "  print(repeated + \"world!\\n\")\n"
        "  var sum: int = 0\n"
        "  for var k: int = 1; k <= 10; k++ =>\n"
        "    sum = sum + k\n"
        "  print($\"Sum 1 to 10: {sum}\\n\")\n"
        "  var pi_approx: double = 3.14159\n"
        "  print($\"Pi approx: {pi_approx}\\n\")\n"
        "  var ch: char = 'A'\n"
        "  print($\"Char: {ch}\\n\")\n"
        "  var flag: bool = true\n"
        "  print($\"Flag: {flag}\\n\")\n"
        "  print(\"Complete main method ... \\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
        for (int i = 0; i < module->count; i++)
        {
            ast_print_stmt(&arena, module->statements[i], 0);
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 4);

    Stmt *fact_fn = module->statements[0];
    assert(fact_fn->type == STMT_FUNCTION);
    assert(strcmp(fact_fn->as.function.name.start, "factorial") == 0);
    assert(fact_fn->as.function.param_count == 1);
    assert(fact_fn->as.function.return_type->kind == TYPE_INT);
    assert(fact_fn->as.function.body_count == 5);
    assert(fact_fn->as.function.body[0]->type == STMT_EXPR);
    assert(fact_fn->as.function.body[1]->type == STMT_IF);
    assert(fact_fn->as.function.body[2]->type == STMT_VAR_DECL);
    assert(fact_fn->as.function.body[3]->type == STMT_EXPR);
    assert(fact_fn->as.function.body[4]->type == STMT_RETURN);

    Stmt *prime_fn = module->statements[1];
    assert(strcmp(prime_fn->as.function.name.start, "is_prime") == 0);
    assert(prime_fn->as.function.return_type->kind == TYPE_BOOL);

    Stmt *repeat_fn = module->statements[2];
    assert(strcmp(repeat_fn->as.function.name.start, "repeat_string") == 0);
    assert(repeat_fn->as.function.return_type->kind == TYPE_STRING);

    Stmt *main_fn = module->statements[3];
    assert(strcmp(main_fn->as.function.name.start, "main") == 0);
    assert(main_fn->as.function.return_type->kind == TYPE_VOID);

    Stmt *print_fact = main_fn->as.function.body[3];
    assert(print_fact->type == STMT_EXPR);
    Expr *call = print_fact->as.expression.expression;
    assert(call->type == EXPR_CALL);
    Expr *arg = call->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 5);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_simple_program_parsing()
{
    Arena arena;
    arena_init(&arena, 4096);

    const char *source =
        "fn add(x:int, y:int):int =>\n"
        "  return x + y\n"
        "fn main():void =>\n"
        "  var z:int = add(6, 2)\n"
        "  print($\"The answer is {z}\\n\")\n";

    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    SymbolTable symbol_table;
    symbol_table_init(&arena, &symbol_table);

    Parser parser;
    parser_init(&arena, &parser, &lexer, &symbol_table);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 2);
    assert(strcmp(module->filename, "test.sn") == 0);

    Stmt *add_fn = module->statements[0];
    assert(add_fn != NULL);
    assert(add_fn->type == STMT_FUNCTION);
    assert(strcmp(add_fn->as.function.name.start, "add") == 0);
    assert(add_fn->as.function.param_count == 2);
    assert(strcmp(add_fn->as.function.params[0].name.start, "x") == 0);
    assert(add_fn->as.function.params[0].type->kind == TYPE_INT);
    assert(strcmp(add_fn->as.function.params[1].name.start, "y") == 0);
    assert(add_fn->as.function.params[1].type->kind == TYPE_INT);
    assert(add_fn->as.function.return_type->kind == TYPE_INT);
    assert(add_fn->as.function.body_count == 1);

    Stmt *add_body = add_fn->as.function.body[0];
    assert(add_body->type == STMT_RETURN);
    assert(add_body->as.return_stmt.value->type == EXPR_BINARY);
    assert(add_body->as.return_stmt.value->as.binary.operator == TOKEN_PLUS);
    assert(add_body->as.return_stmt.value->as.binary.left->type == EXPR_VARIABLE);
    assert(strcmp(add_body->as.return_stmt.value->as.binary.left->as.variable.name.start, "x") == 0);
    assert(add_body->as.return_stmt.value->as.binary.right->type == EXPR_VARIABLE);
    assert(strcmp(add_body->as.return_stmt.value->as.binary.right->as.variable.name.start, "y") == 0);

    Stmt *main_fn = module->statements[1];
    assert(main_fn != NULL);
    assert(main_fn->type == STMT_FUNCTION);
    assert(strcmp(main_fn->as.function.name.start, "main") == 0);
    assert(main_fn->as.function.param_count == 0);
    assert(main_fn->as.function.return_type->kind == TYPE_VOID);
    assert(main_fn->as.function.body_count == 2);

    Stmt *var_decl = main_fn->as.function.body[0];
    assert(var_decl->type == STMT_VAR_DECL);
    assert(strcmp(var_decl->as.var_decl.name.start, "z") == 0);
    assert(var_decl->as.var_decl.type->kind == TYPE_INT);
    assert(var_decl->as.var_decl.initializer->type == EXPR_CALL);
    assert(var_decl->as.var_decl.initializer->as.call.callee->type == EXPR_VARIABLE);
    assert(strcmp(var_decl->as.var_decl.initializer->as.call.callee->as.variable.name.start, "add") == 0);
    assert(var_decl->as.var_decl.initializer->as.call.arg_count == 2);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[0]->as.literal.value.int_value == 6);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[1]->type == EXPR_LITERAL);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[1]->as.literal.value.int_value == 2);

    Stmt *print_stmt = main_fn->as.function.body[1];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(print_stmt->as.expression.expression->as.call.callee->type == EXPR_VARIABLE);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_INTERPOLATED);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.part_count == 3);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->as.literal.value.string_value, "The answer is ") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->as.variable.name.start, "z") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->as.literal.value.string_value, "\n") == 0);

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    symbol_table_cleanup(&symbol_table);
    arena_free(&arena);
}

static void test_parser_program_main()
{
    TEST_SECTION("Parser Program Tests");
    TEST_RUN("full_program_parsing", test_full_program_parsing);
    TEST_RUN("simple_program_parsing", test_simple_program_parsing);
}
