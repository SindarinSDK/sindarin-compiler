// ast_tests_edge_literal.c
// Literal expression tests

/* ============================================================================
 * Literal Expression Tests
 * ============================================================================ */

static void test_ast_literal_int_zero(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue val = {.int_value = 0};
    Token tok = create_dummy_token(&arena, "0");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.int_value == 0);

    cleanup_arena(&arena);
}

static void test_ast_literal_int_negative(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue val = {.int_value = -42};
    Token tok = create_dummy_token(&arena, "-42");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.int_value == -42);

    cleanup_arena(&arena);
}

static void test_ast_literal_double_zero(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    LiteralValue val = {.double_value = 0.0};
    Token tok = create_dummy_token(&arena, "0.0");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.double_value == 0.0);

    cleanup_arena(&arena);
}

static void test_ast_literal_double_pi(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    LiteralValue val = {.double_value = 3.14159};
    Token tok = create_dummy_token(&arena, "3.14159");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.double_value > 3.14 && e->as.literal.value.double_value < 3.15);

    cleanup_arena(&arena);
}

static void test_ast_literal_bool_true(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    LiteralValue val = {.bool_value = true};
    Token tok = create_dummy_token(&arena, "true");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.bool_value == true);

    cleanup_arena(&arena);
}

static void test_ast_literal_bool_false(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    LiteralValue val = {.bool_value = false};
    Token tok = create_dummy_token(&arena, "false");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.bool_value == false);

    cleanup_arena(&arena);
}

static void test_ast_literal_string_empty(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_STRING);
    LiteralValue val = {.string_value = ""};
    Token tok = create_dummy_token(&arena, "\"\"");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(strcmp(e->as.literal.value.string_value, "") == 0);

    cleanup_arena(&arena);
}

static void test_ast_literal_string_hello(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_STRING);
    LiteralValue val = {.string_value = "hello"};
    Token tok = create_dummy_token(&arena, "\"hello\"");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(strcmp(e->as.literal.value.string_value, "hello") == 0);

    cleanup_arena(&arena);
}

static void test_ast_literal_char_a(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_CHAR);
    LiteralValue val = {.char_value = 'a'};
    Token tok = create_dummy_token(&arena, "'a'");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.char_value == 'a');

    cleanup_arena(&arena);
}

static void test_ast_literal_char_newline(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_CHAR);
    LiteralValue val = {.char_value = '\n'};
    Token tok = create_dummy_token(&arena, "'\\n'");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.char_value == '\n');

    cleanup_arena(&arena);
}

static void test_ast_literal_byte_zero(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BYTE);
    LiteralValue val = {.int_value = 0};
    Token tok = create_dummy_token(&arena, "0");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.int_value == 0);

    cleanup_arena(&arena);
}

static void test_ast_literal_byte_max(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BYTE);
    LiteralValue val = {.int_value = 255};
    Token tok = create_dummy_token(&arena, "255");
    Expr *e = ast_create_literal_expr(&arena, val, t, false, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_LITERAL);
    assert(e->as.literal.value.int_value == 255);

    cleanup_arena(&arena);
}
