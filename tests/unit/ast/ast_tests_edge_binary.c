// ast_tests_edge_binary.c
// Binary expression tests

/* ============================================================================
 * Binary Expression Tests
 * ============================================================================ */

static void test_ast_binary_add(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 1};
    LiteralValue rv = {.int_value = 2};
    Token tok = create_dummy_token(&arena, "+");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_PLUS);
    assert(e->as.binary.left == left);
    assert(e->as.binary.right == right);

    cleanup_arena(&arena);
}

static void test_ast_binary_sub(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 5};
    LiteralValue rv = {.int_value = 3};
    Token tok = create_dummy_token(&arena, "-");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_MINUS, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_MINUS);

    cleanup_arena(&arena);
}

static void test_ast_binary_mul(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 3};
    LiteralValue rv = {.int_value = 4};
    Token tok = create_dummy_token(&arena, "*");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_STAR);

    cleanup_arena(&arena);
}

static void test_ast_binary_div(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 10};
    LiteralValue rv = {.int_value = 2};
    Token tok = create_dummy_token(&arena, "/");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_SLASH, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_SLASH);

    cleanup_arena(&arena);
}

static void test_ast_binary_mod(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 10};
    LiteralValue rv = {.int_value = 3};
    Token tok = create_dummy_token(&arena, "%");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_MODULO, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_MODULO);

    cleanup_arena(&arena);
}

static void test_ast_binary_eq(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 5};
    LiteralValue rv = {.int_value = 5};
    Token tok = create_dummy_token(&arena, "==");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_EQUAL_EQUAL, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_EQUAL_EQUAL);

    cleanup_arena(&arena);
}

static void test_ast_binary_neq(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 5};
    LiteralValue rv = {.int_value = 3};
    Token tok = create_dummy_token(&arena, "!=");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_BANG_EQUAL, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_BANG_EQUAL);

    cleanup_arena(&arena);
}

static void test_ast_binary_less(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 3};
    LiteralValue rv = {.int_value = 5};
    Token tok = create_dummy_token(&arena, "<");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_LESS, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_LESS);

    cleanup_arena(&arena);
}

static void test_ast_binary_less_eq(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 3};
    LiteralValue rv = {.int_value = 3};
    Token tok = create_dummy_token(&arena, "<=");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_LESS_EQUAL, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_LESS_EQUAL);

    cleanup_arena(&arena);
}

static void test_ast_binary_greater(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 5};
    LiteralValue rv = {.int_value = 3};
    Token tok = create_dummy_token(&arena, ">");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_GREATER, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_GREATER);

    cleanup_arena(&arena);
}

static void test_ast_binary_greater_eq(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue lv = {.int_value = 5};
    LiteralValue rv = {.int_value = 5};
    Token tok = create_dummy_token(&arena, ">=");
    Expr *left = ast_create_literal_expr(&arena, lv, t, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, rv, t, false, &tok);

    Expr *e = ast_create_binary_expr(&arena, left, TOKEN_GREATER_EQUAL, right, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_BINARY);
    assert(e->as.binary.operator == TOKEN_GREATER_EQUAL);

    cleanup_arena(&arena);
}
