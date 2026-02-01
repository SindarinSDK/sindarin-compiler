/**
 * code_gen_tests_optimization_native.c - Native operator tests
 *
 * Tests for native C operator availability and generation.
 */

/* Test native operator availability */
static void test_can_use_native_operator(void)
{
    /* Operators that can use native C */
    assert(can_use_native_operator(TOKEN_PLUS) == true);
    assert(can_use_native_operator(TOKEN_MINUS) == true);
    assert(can_use_native_operator(TOKEN_STAR) == true);
    assert(can_use_native_operator(TOKEN_EQUAL_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_BANG_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_LESS) == true);
    assert(can_use_native_operator(TOKEN_LESS_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_GREATER) == true);
    assert(can_use_native_operator(TOKEN_GREATER_EQUAL) == true);

    /* Operators that need runtime (division/modulo for zero check) */
    assert(can_use_native_operator(TOKEN_SLASH) == false);
    assert(can_use_native_operator(TOKEN_MODULO) == false);

    /* Unknown operators */
    assert(can_use_native_operator(TOKEN_DOT) == false);
    assert(can_use_native_operator(TOKEN_COMMA) == false);
}

/* Test get_native_c_operator returns correct strings */
static void test_get_native_c_operator(void)
{
    assert(strcmp(get_native_c_operator(TOKEN_PLUS), "+") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MINUS), "-") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_STAR), "*") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_SLASH), "/") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MODULO), "%") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_EQUAL_EQUAL), "==") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_BANG_EQUAL), "!=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS), "<") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS_EQUAL), "<=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER), ">") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER_EQUAL), ">=") == 0);

    /* Unknown operators return NULL */
    assert(get_native_c_operator(TOKEN_DOT) == NULL);
}

/* Test gen_native_arithmetic in unchecked mode */
static void test_gen_native_arithmetic_unchecked(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Test integer addition */
    char *result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_PLUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    /* Test integer subtraction */
    result = gen_native_arithmetic(&gen, "10LL", "4LL", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test integer multiplication */
    result = gen_native_arithmetic(&gen, "7LL", "6LL", TOKEN_STAR, int_type);
    assert(result != NULL);
    assert(strstr(result, "*") != NULL);

    /* Test division should return NULL (needs runtime for zero check) */
    result = gen_native_arithmetic(&gen, "20LL", "4LL", TOKEN_SLASH, int_type);
    assert(result == NULL);

    /* Test double addition */
    result = gen_native_arithmetic(&gen, "3.14", "2.0", TOKEN_PLUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_arithmetic in checked mode returns NULL */
static void test_gen_native_arithmetic_checked(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_CHECKED;  /* Default mode */

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* In checked mode, all operations should return NULL */
    char *result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_PLUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_MINUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_STAR, int_type);
    assert(result == NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_unary */
static void test_gen_native_unary(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Test integer negation */
    char *result = gen_native_unary(&gen, "42LL", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test double negation */
    result = gen_native_unary(&gen, "3.14", TOKEN_MINUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test logical not */
    result = gen_native_unary(&gen, "true", TOKEN_BANG, bool_type);
    assert(result != NULL);
    assert(strstr(result, "!") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

static void test_code_gen_optimization_native_main(void)
{
    TEST_RUN("can_use_native_operator", test_can_use_native_operator);
    TEST_RUN("get_native_c_operator", test_get_native_c_operator);
    TEST_RUN("gen_native_arithmetic_unchecked", test_gen_native_arithmetic_unchecked);
    TEST_RUN("gen_native_arithmetic_checked", test_gen_native_arithmetic_checked);
    TEST_RUN("gen_native_unary", test_gen_native_unary);
}
