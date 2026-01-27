// ast_tests_edge_cases.c
// Additional edge case tests for AST creation functions

/* ============================================================================
 * Primitive Type Creation Tests
 * ============================================================================ */

static void test_ast_primitive_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    assert(t != NULL);
    assert(t->kind == TYPE_INT);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_long(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(t != NULL);
    assert(t->kind == TYPE_LONG);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_double(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(t != NULL);
    assert(t->kind == TYPE_DOUBLE);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_bool(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(t != NULL);
    assert(t->kind == TYPE_BOOL);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_char(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(t != NULL);
    assert(t->kind == TYPE_CHAR);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_byte(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(t != NULL);
    assert(t->kind == TYPE_BYTE);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_string(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(t != NULL);
    assert(t->kind == TYPE_STRING);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_void(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(t != NULL);
    assert(t->kind == TYPE_VOID);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Composite Type Creation Tests
 * ============================================================================ */

static void test_ast_array_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type == elem);

    cleanup_arena(&arena);
}

static void test_ast_array_type_string(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type->kind == TYPE_STRING);

    cleanup_arena(&arena);
}

static void test_ast_pointer_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *pointee = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr = ast_create_pointer_type(&arena, pointee);
    assert(ptr != NULL);
    assert(ptr->kind == TYPE_POINTER);
    assert(ptr->as.pointer.pointee == pointee);

    cleanup_arena(&arena);
}

static void test_ast_pointer_type_char(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *pointee = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *ptr = ast_create_pointer_type(&arena, pointee);
    assert(ptr != NULL);
    assert(ptr->kind == TYPE_POINTER);
    assert(ptr->as.pointer.pointee->kind == TYPE_CHAR);

    cleanup_arena(&arena);
}

static void test_ast_opaque_type(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *opaque = ast_create_opaque_type(&arena, "OpaqueHandle");
    assert(opaque != NULL);
    assert(opaque->kind == TYPE_OPAQUE);

    cleanup_arena(&arena);
}

static void test_ast_nested_array_type(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, elem);
    Type *arr2 = ast_create_array_type(&arena, arr1);
    assert(arr2 != NULL);
    assert(arr2->kind == TYPE_ARRAY);
    assert(arr2->as.array.element_type->kind == TYPE_ARRAY);

    cleanup_arena(&arena);
}

static void test_ast_pointer_to_pointer(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr1 = ast_create_pointer_type(&arena, int_type);
    Type *ptr2 = ast_create_pointer_type(&arena, ptr1);
    assert(ptr2 != NULL);
    assert(ptr2->kind == TYPE_POINTER);
    assert(ptr2->as.pointer.pointee->kind == TYPE_POINTER);

    cleanup_arena(&arena);
}

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

/* ============================================================================
 * Unary Expression Tests
 * ============================================================================ */

static void test_ast_unary_negate(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 5};
    Token tok = create_dummy_token(&arena, "-");
    Expr *operand = ast_create_literal_expr(&arena, v, t, false, &tok);

    Expr *e = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_UNARY);
    assert(e->as.unary.operator == TOKEN_MINUS);
    assert(e->as.unary.operand == operand);

    cleanup_arena(&arena);
}

static void test_ast_unary_not(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    LiteralValue v = {.bool_value = true};
    Token tok = create_dummy_token(&arena, "!");
    Expr *operand = ast_create_literal_expr(&arena, v, t, false, &tok);

    Expr *e = ast_create_unary_expr(&arena, TOKEN_BANG, operand, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_UNARY);
    assert(e->as.unary.operator == TOKEN_BANG);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Variable Expression Tests
 * ============================================================================ */

static void test_ast_variable_simple(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "x");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);
    assert(strncmp(e->as.variable.name.start, "x", 1) == 0);

    cleanup_arena(&arena);
}

static void test_ast_variable_underscore(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "_private");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);
    assert(strncmp(e->as.variable.name.start, "_private", 8) == 0);

    cleanup_arena(&arena);
}

static void test_ast_variable_long_name(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "very_long_variable_name_123");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Statement Tests
 * ============================================================================ */

static void test_ast_expr_stmt(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token tok = create_dummy_token(&arena, "42");
    Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);

    Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);

    assert(s != NULL);
    assert(s->type == STMT_EXPR);
    assert(s->as.expression.expression == expr);

    cleanup_arena(&arena);
}

static void test_ast_return_stmt_with_value(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 0};
    Token tok = create_dummy_token(&arena, "return");
    Expr *val = ast_create_literal_expr(&arena, v, t, false, &tok);

    Stmt *s = ast_create_return_stmt(&arena, tok, val, &tok);

    assert(s != NULL);
    assert(s->type == STMT_RETURN);
    assert(s->as.return_stmt.value == val);

    cleanup_arena(&arena);
}

static void test_ast_return_stmt_void(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "return");
    Stmt *s = ast_create_return_stmt(&arena, tok, NULL, &tok);

    assert(s != NULL);
    assert(s->type == STMT_RETURN);
    assert(s->as.return_stmt.value == NULL);

    cleanup_arena(&arena);
}

static void test_ast_block_stmt_empty(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "{");
    Stmt *s = ast_create_block_stmt(&arena, NULL, 0, &tok);

    assert(s != NULL);
    assert(s->type == STMT_BLOCK);
    assert(s->as.block.count == 0);

    cleanup_arena(&arena);
}

static void test_ast_block_stmt_with_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "{");
    Token name = create_dummy_token(&arena, "x");

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt *));
    stmts[0] = ast_create_var_decl_stmt(&arena, name, t, NULL, &name);

    Stmt *s = ast_create_block_stmt(&arena, stmts, 1, &tok);

    assert(s != NULL);
    assert(s->type == STMT_BLOCK);
    assert(s->as.block.count == 1);

    cleanup_arena(&arena);
}

static void test_ast_var_decl_no_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token name = create_dummy_token(&arena, "x");
    Stmt *s = ast_create_var_decl_stmt(&arena, name, t, NULL, &name);

    assert(s != NULL);
    assert(s->type == STMT_VAR_DECL);
    assert(s->as.var_decl.type == t);
    assert(s->as.var_decl.initializer == NULL);

    cleanup_arena(&arena);
}

static void test_ast_var_decl_with_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token name = create_dummy_token(&arena, "x");
    Expr *init = ast_create_literal_expr(&arena, v, t, false, &name);

    Stmt *s = ast_create_var_decl_stmt(&arena, name, t, init, &name);

    assert(s != NULL);
    assert(s->type == STMT_VAR_DECL);
    assert(s->as.var_decl.initializer == init);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Module Tests
 * ============================================================================ */

static void test_ast_module_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    assert(mod.filename != NULL);
    assert(strcmp(mod.filename, "test.sn") == 0);
    assert(mod.statement_count == 0);

    cleanup_arena(&arena);
}

static void test_ast_module_add_single_stmt(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token tok = create_dummy_token(&arena, "42");
    Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);
    Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);

    ast_module_add_statement(&arena, &mod, s);

    assert(mod.statement_count == 1);
    assert(mod.statements[0] == s);

    cleanup_arena(&arena);
}

static void test_ast_module_add_multiple_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "42");
    for (int i = 0; i < 5; i++) {
        LiteralValue v = {.int_value = i};
        Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);
        Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);
        ast_module_add_statement(&arena, &mod, s);
    }

    assert(mod.statement_count == 5);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_ast_edge_cases_main(void)
{
    TEST_SECTION("AST - Primitive Types");
    TEST_RUN("primitive_type_int", test_ast_primitive_type_int);
    TEST_RUN("primitive_type_long", test_ast_primitive_type_long);
    TEST_RUN("primitive_type_double", test_ast_primitive_type_double);
    TEST_RUN("primitive_type_bool", test_ast_primitive_type_bool);
    TEST_RUN("primitive_type_char", test_ast_primitive_type_char);
    TEST_RUN("primitive_type_byte", test_ast_primitive_type_byte);
    TEST_RUN("primitive_type_string", test_ast_primitive_type_string);
    TEST_RUN("primitive_type_void", test_ast_primitive_type_void);

    TEST_SECTION("AST - Composite Types");
    TEST_RUN("array_type_int", test_ast_array_type_int);
    TEST_RUN("array_type_string", test_ast_array_type_string);
    TEST_RUN("pointer_type_int", test_ast_pointer_type_int);
    TEST_RUN("pointer_type_char", test_ast_pointer_type_char);
    TEST_RUN("opaque_type", test_ast_opaque_type);
    TEST_RUN("nested_array_type", test_ast_nested_array_type);
    TEST_RUN("pointer_to_pointer", test_ast_pointer_to_pointer);

    TEST_SECTION("AST - Literal Expressions");
    TEST_RUN("literal_int_zero", test_ast_literal_int_zero);
    TEST_RUN("literal_int_negative", test_ast_literal_int_negative);
    TEST_RUN("literal_double_zero", test_ast_literal_double_zero);
    TEST_RUN("literal_double_pi", test_ast_literal_double_pi);
    TEST_RUN("literal_bool_true", test_ast_literal_bool_true);
    TEST_RUN("literal_bool_false", test_ast_literal_bool_false);
    TEST_RUN("literal_string_empty", test_ast_literal_string_empty);
    TEST_RUN("literal_string_hello", test_ast_literal_string_hello);
    TEST_RUN("literal_char_a", test_ast_literal_char_a);
    TEST_RUN("literal_char_newline", test_ast_literal_char_newline);
    TEST_RUN("literal_byte_zero", test_ast_literal_byte_zero);
    TEST_RUN("literal_byte_max", test_ast_literal_byte_max);

    TEST_SECTION("AST - Binary Expressions");
    TEST_RUN("binary_add", test_ast_binary_add);
    TEST_RUN("binary_sub", test_ast_binary_sub);
    TEST_RUN("binary_mul", test_ast_binary_mul);
    TEST_RUN("binary_div", test_ast_binary_div);
    TEST_RUN("binary_mod", test_ast_binary_mod);
    TEST_RUN("binary_eq", test_ast_binary_eq);
    TEST_RUN("binary_neq", test_ast_binary_neq);
    TEST_RUN("binary_less", test_ast_binary_less);
    TEST_RUN("binary_less_eq", test_ast_binary_less_eq);
    TEST_RUN("binary_greater", test_ast_binary_greater);
    TEST_RUN("binary_greater_eq", test_ast_binary_greater_eq);

    TEST_SECTION("AST - Unary Expressions");
    TEST_RUN("unary_negate", test_ast_unary_negate);
    TEST_RUN("unary_not", test_ast_unary_not);

    TEST_SECTION("AST - Variable Expressions");
    TEST_RUN("variable_simple", test_ast_variable_simple);
    TEST_RUN("variable_underscore", test_ast_variable_underscore);
    TEST_RUN("variable_long_name", test_ast_variable_long_name);

    TEST_SECTION("AST - Statements");
    TEST_RUN("expr_stmt", test_ast_expr_stmt);
    TEST_RUN("return_stmt_with_value", test_ast_return_stmt_with_value);
    TEST_RUN("return_stmt_void", test_ast_return_stmt_void);
    TEST_RUN("block_stmt_empty", test_ast_block_stmt_empty);
    TEST_RUN("block_stmt_with_stmts", test_ast_block_stmt_with_stmts);
    TEST_RUN("var_decl_no_init", test_ast_var_decl_no_init);
    TEST_RUN("var_decl_with_init", test_ast_var_decl_with_init);

    TEST_SECTION("AST - Module");
    TEST_RUN("module_init", test_ast_module_init);
    TEST_RUN("module_add_single_stmt", test_ast_module_add_single_stmt);
    TEST_RUN("module_add_multiple_stmts", test_ast_module_add_multiple_stmts);
}
