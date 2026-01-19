#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../test_harness.h"
#include "../optimizer.h"
#include "../arena.h"
#include "../ast.h"
#include "../debug.h"

/* Note: setup_basic_token is defined in code_gen_tests_util.c */

/* Helper to create an int literal expression */
static Expr *create_int_literal(Arena *arena, int64_t value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.value.int_value = value;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_INT);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

/* Helper to create a variable expression */
static Expr *create_variable_expr(Arena *arena, const char *name)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    setup_basic_token(&expr->as.variable.name, TOKEN_IDENTIFIER, name);
    expr->expr_type = ast_create_primitive_type(arena, TYPE_INT);
    return expr;
}

/* Helper to create a binary expression */
static Expr *create_binary_expr(Arena *arena, Expr *left, SnTokenType op, Expr *right)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = op;
    expr->expr_type = left->expr_type;  /* Simplified */
    return expr;
}

/* Helper to create a unary expression */
static Expr *create_unary_expr(Arena *arena, SnTokenType op, Expr *operand)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = op;
    expr->as.unary.operand = operand;
    expr->expr_type = operand->expr_type;
    return expr;
}

/* Helper to create a return statement */
static Stmt *create_return_stmt(Arena *arena, Expr *value)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_RETURN;
    Token tok;
    setup_basic_token(&tok, TOKEN_RETURN, "return");
    stmt->as.return_stmt.keyword = tok;
    stmt->as.return_stmt.value = value;
    return stmt;
}

/* Helper to create an expression statement */
static Stmt *create_expr_stmt(Arena *arena, Expr *expr)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->as.expression.expression = expr;
    return stmt;
}

/* Helper to create a variable declaration */
static Stmt *create_var_decl(Arena *arena, const char *name, Expr *init)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_VAR_DECL;
    setup_basic_token(&stmt->as.var_decl.name, TOKEN_IDENTIFIER, name);
    stmt->as.var_decl.type = ast_create_primitive_type(arena, TYPE_INT);
    stmt->as.var_decl.initializer = init;
    stmt->as.var_decl.mem_qualifier = MEM_DEFAULT;
    return stmt;
}

/* ============================================================================
 * Test: stmt_is_terminator
 * ============================================================================ */

static void test_stmt_is_terminator_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *ret_stmt = create_return_stmt(&arena, create_int_literal(&arena, 0));
    assert(stmt_is_terminator(ret_stmt) == true);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_return");
}

static void test_stmt_is_terminator_break_continue(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *break_stmt = arena_alloc(&arena, sizeof(Stmt));
    break_stmt->type = STMT_BREAK;
    assert(stmt_is_terminator(break_stmt) == true);

    Stmt *continue_stmt = arena_alloc(&arena, sizeof(Stmt));
    continue_stmt->type = STMT_CONTINUE;
    assert(stmt_is_terminator(continue_stmt) == true);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_break_continue");
}

static void test_stmt_is_terminator_non_terminator(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *expr_stmt = create_expr_stmt(&arena, create_int_literal(&arena, 42));
    assert(stmt_is_terminator(expr_stmt) == false);

    Stmt *var_decl = create_var_decl(&arena, "x", create_int_literal(&arena, 5));
    assert(stmt_is_terminator(var_decl) == false);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_non_terminator");
}

/* ============================================================================
 * Test: expr_is_noop
 * ============================================================================ */

static void test_expr_is_noop_add_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, zero);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test 0 + x */
    Expr *add2 = create_binary_expr(&arena, zero, TOKEN_PLUS, x);
    is_noop = expr_is_noop(add2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_add_zero");
}

static void test_expr_is_noop_sub_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *sub = create_binary_expr(&arena, x, TOKEN_MINUS, zero);

    Expr *simplified;
    bool is_noop = expr_is_noop(sub, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_sub_zero");
}

static void test_expr_is_noop_mul_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *one = create_int_literal(&arena, 1);
    Expr *mul = create_binary_expr(&arena, x, TOKEN_STAR, one);

    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test 1 * x */
    Expr *mul2 = create_binary_expr(&arena, one, TOKEN_STAR, x);
    is_noop = expr_is_noop(mul2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_mul_one");
}

static void test_expr_is_noop_div_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *one = create_int_literal(&arena, 1);
    Expr *div = create_binary_expr(&arena, x, TOKEN_SLASH, one);

    Expr *simplified;
    bool is_noop = expr_is_noop(div, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_div_one");
}

static void test_expr_is_noop_double_negation(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *neg1 = create_unary_expr(&arena, TOKEN_BANG, x);
    Expr *neg2 = create_unary_expr(&arena, TOKEN_BANG, neg1);

    Expr *simplified;
    bool is_noop = expr_is_noop(neg2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test -(-x) */
    Expr *y = create_variable_expr(&arena, "y");
    Expr *minus1 = create_unary_expr(&arena, TOKEN_MINUS, y);
    Expr *minus2 = create_unary_expr(&arena, TOKEN_MINUS, minus1);

    is_noop = expr_is_noop(minus2, &simplified);
    assert(is_noop == true);
    assert(simplified == y);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_double_negation");
}

static void test_expr_is_noop_not_noop(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == false);
    assert(simplified == add);  /* Not simplified */

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_not_noop");
}

/* ============================================================================
 * Test: remove_unreachable_statements
 * ============================================================================ */

static void test_remove_unreachable_after_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: return 0; x = 5; (x = 5 should be removed) */
    Stmt **stmts = arena_alloc(&arena, 3 * sizeof(Stmt *));
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    stmts[2] = create_expr_stmt(&arena, create_variable_expr(&arena, "y"));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 2);
    assert(count == 1);
    assert(stmts[0]->type == STMT_RETURN);

    arena_free(&arena);
    DEBUG_INFO("Finished test_remove_unreachable_after_return");
}

static void test_remove_unreachable_after_break(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: break; x = 5; (x = 5 should be removed) */
    Stmt **stmts = arena_alloc(&arena, 2 * sizeof(Stmt *));
    stmts[0] = arena_alloc(&arena, sizeof(Stmt));
    stmts[0]->type = STMT_BREAK;
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    int count = 2;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 1);
    assert(count == 1);
    assert(stmts[0]->type == STMT_BREAK);

    arena_free(&arena);
    DEBUG_INFO("Finished test_remove_unreachable_after_break");
}

static void test_no_unreachable_statements(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: x = 5; y = 10; return 0; (no dead code) */
    Stmt **stmts = arena_alloc(&arena, 3 * sizeof(Stmt *));
    stmts[0] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "y"));
    stmts[2] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 0);
    assert(count == 3);

    arena_free(&arena);
    DEBUG_INFO("Finished test_no_unreachable_statements");
}

/* ============================================================================
 * Test: Variable usage tracking
 * ============================================================================ */

static void test_collect_used_variables(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: x + y */
    Expr *x = create_variable_expr(&arena, "x");
    Expr *y = create_variable_expr(&arena, "y");
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, y);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(add, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 2);
    assert(is_variable_used(used_vars, used_count, x->as.variable.name));
    assert(is_variable_used(used_vars, used_count, y->as.variable.name));

    arena_free(&arena);
    DEBUG_INFO("Finished test_collect_used_variables");
}

static void test_is_variable_used(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token vars[2];
    setup_basic_token(&vars[0], TOKEN_IDENTIFIER, "x");
    setup_basic_token(&vars[1], TOKEN_IDENTIFIER, "y");

    Token x_tok, z_tok;
    setup_basic_token(&x_tok, TOKEN_IDENTIFIER, "x");
    setup_basic_token(&z_tok, TOKEN_IDENTIFIER, "z");

    assert(is_variable_used(vars, 2, x_tok) == true);
    assert(is_variable_used(vars, 2, z_tok) == false);

    arena_free(&arena);
    DEBUG_INFO("Finished test_is_variable_used");
}

/* ============================================================================
 * Test: Full optimization passes
 * ============================================================================ */

static void test_optimizer_dead_code_elimination_function(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a function with:
       - var unused: int = 0  (unused variable - should be removed)
       - var x: int = 5       (used in return)
       - return x
       - var unreachable = 0  (unreachable - should be removed)
     */
    Stmt **body = arena_alloc(&arena, 4 * sizeof(Stmt *));
    body[0] = create_var_decl(&arena, "unused", create_int_literal(&arena, 0));
    body[1] = create_var_decl(&arena, "x", create_int_literal(&arena, 5));
    body[2] = create_return_stmt(&arena, create_variable_expr(&arena, "x"));
    body[3] = create_var_decl(&arena, "unreachable", create_int_literal(&arena, 0));

    FunctionStmt fn = {
        .body = body,
        .body_count = 4,
        .param_count = 0,
        .params = NULL,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "test_fn");

    optimizer_eliminate_dead_code_function(&opt, &fn);

    /* Should have removed unreachable code and unused variable */
    int stmts_removed, vars_removed, noops_removed;
    optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);

    assert(stmts_removed >= 1);  /* unreachable statement */
    assert(vars_removed >= 1);   /* unused variable */

    /* Final body should have 2 statements: var x and return x */
    assert(fn.body_count == 2);
    assert(fn.body[0]->type == STMT_VAR_DECL);
    assert(fn.body[1]->type == STMT_RETURN);

    arena_free(&arena);
    DEBUG_INFO("Finished test_optimizer_dead_code_elimination_function");
}

static void test_optimizer_noop_simplification(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a function with:
       - var x: int = y + 0   (should simplify to y)
       - return x
     */
    Expr *y = create_variable_expr(&arena, "y");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *add = create_binary_expr(&arena, y, TOKEN_PLUS, zero);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));
    body[0] = create_var_decl(&arena, "x", add);
    body[1] = create_return_stmt(&arena, create_variable_expr(&arena, "x"));

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 0,
        .params = NULL,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "test_fn");

    optimizer_eliminate_dead_code_function(&opt, &fn);

    int stmts_removed, vars_removed, noops_removed;
    optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);

    assert(noops_removed >= 1);

    /* The initializer should now be simplified to just y */
    Expr *init = fn.body[0]->as.var_decl.initializer;
    assert(init->type == EXPR_VARIABLE);

    arena_free(&arena);
    DEBUG_INFO("Finished test_optimizer_noop_simplification");
}

/* ============================================================================
 * Test: Tail Call Detection
 * ============================================================================ */

/* Helper to create a call expression */
static Expr *create_call_expr(Arena *arena, const char *func_name, Expr **args, int arg_count)
{
    Expr *callee = arena_alloc(arena, sizeof(Expr));
    callee->type = EXPR_VARIABLE;
    setup_basic_token(&callee->as.variable.name, TOKEN_IDENTIFIER, func_name);
    callee->expr_type = ast_create_primitive_type(arena, TYPE_INT);

    Expr *call = arena_alloc(arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = args;
    call->as.call.arg_count = arg_count;
    call->as.call.is_tail_call = false;
    call->expr_type = ast_create_primitive_type(arena, TYPE_INT);
    return call;
}

static void test_tail_call_detection_simple(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: return foo(x) */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_variable_expr(&arena, "x");
    Expr *call = create_call_expr(&arena, "foo", args, 1);
    Stmt *ret = create_return_stmt(&arena, call);

    /* Create a token for function name "foo" */
    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "foo");

    /* Should be a tail call */
    assert(is_tail_recursive_return(ret, func_name) == true);

    /* Not a tail call to a different function */
    Token other_name;
    setup_basic_token(&other_name, TOKEN_IDENTIFIER, "bar");
    assert(is_tail_recursive_return(ret, other_name) == false);

    arena_free(&arena);
}

static void test_tail_call_detection_not_tail(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: return n * foo(x) - NOT a tail call */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_variable_expr(&arena, "x");
    Expr *call = create_call_expr(&arena, "foo", args, 1);
    Expr *n = create_variable_expr(&arena, "n");
    Expr *mul = create_binary_expr(&arena, n, TOKEN_STAR, call);
    Stmt *ret = create_return_stmt(&arena, mul);

    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "foo");

    /* Should NOT be a tail call (wrapped in multiplication) */
    assert(is_tail_recursive_return(ret, func_name) == false);

    arena_free(&arena);
}

static void test_function_has_tail_recursion(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create a tail-recursive function:
       fn foo(n: int): int =>
           if n <= 0 => return 0
           return foo(n - 1)
     */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_binary_expr(&arena, create_variable_expr(&arena, "n"),
                                 TOKEN_MINUS, create_int_literal(&arena, 1));
    Expr *call = create_call_expr(&arena, "foo", args, 1);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));

    /* if statement */
    Stmt *if_stmt = arena_alloc(&arena, sizeof(Stmt));
    if_stmt->type = STMT_IF;
    if_stmt->as.if_stmt.condition = create_binary_expr(&arena,
        create_variable_expr(&arena, "n"), TOKEN_LESS_EQUAL, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.then_branch = create_return_stmt(&arena, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.else_branch = NULL;
    body[0] = if_stmt;

    /* return foo(n-1) */
    body[1] = create_return_stmt(&arena, call);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    setup_basic_token(&params[0].name, TOKEN_IDENTIFIER, "n");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 1,
        .params = params,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "foo");

    /* Should detect tail recursion */
    assert(function_has_tail_recursion(&fn) == true);

    arena_free(&arena);
}

static void test_tail_call_marking(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a tail-recursive function */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_binary_expr(&arena, create_variable_expr(&arena, "n"),
                                 TOKEN_MINUS, create_int_literal(&arena, 1));
    Expr *call = create_call_expr(&arena, "foo", args, 1);

    /* Verify it's not marked yet */
    assert(call->as.call.is_tail_call == false);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));

    Stmt *if_stmt = arena_alloc(&arena, sizeof(Stmt));
    if_stmt->type = STMT_IF;
    if_stmt->as.if_stmt.condition = create_binary_expr(&arena,
        create_variable_expr(&arena, "n"), TOKEN_LESS_EQUAL, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.then_branch = create_return_stmt(&arena, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.else_branch = NULL;
    body[0] = if_stmt;
    body[1] = create_return_stmt(&arena, call);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    setup_basic_token(&params[0].name, TOKEN_IDENTIFIER, "n");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 1,
        .params = params,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "foo");

    /* Mark tail calls */
    int marked = optimizer_mark_tail_calls(&opt, &fn);

    /* Should have marked 1 tail call */
    assert(marked == 1);
    assert(opt.tail_calls_optimized == 1);

    /* The call expression should now be marked */
    assert(call->as.call.is_tail_call == true);

    arena_free(&arena);
}

/* ============================================================================
 * Test: String Literal Merging
 * ============================================================================ */

/* Helper to create a string literal expression */
static Expr *create_string_literal(Arena *arena, const char *value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_STRING);
    expr->as.literal.value.string_value = arena_strdup(arena, value);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

static void test_string_literal_merge_adjacent(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Hello " + "World" + "!" */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 3;
    interpol->as.interpol.parts = arena_alloc(&arena, 3 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Hello ");
    interpol->as.interpol.parts[1] = create_string_literal(&arena, "World");
    interpol->as.interpol.parts[2] = create_string_literal(&arena, "!");
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* All three should merge into one */
    assert(interpol->as.interpol.part_count == 1);
    assert(interpol->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[0]->as.literal.value.string_value, "Hello World!") == 0);
    assert(opt.string_literals_merged == 2);  /* 3 merged into 1 = 2 merges */

    arena_free(&arena);
}

static void test_string_literal_merge_with_variable(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Hello " + name + " you are " + "great!" */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 4;
    interpol->as.interpol.parts = arena_alloc(&arena, 4 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Hello ");
    interpol->as.interpol.parts[1] = create_variable_expr(&arena, "name");
    interpol->as.interpol.parts[1]->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);
    interpol->as.interpol.parts[2] = create_string_literal(&arena, " you are ");
    interpol->as.interpol.parts[3] = create_string_literal(&arena, "great!");
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* Should merge parts 2+3, leaving 3 parts: "Hello ", name, " you are great!" */
    assert(interpol->as.interpol.part_count == 3);
    assert(interpol->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[0]->as.literal.value.string_value, "Hello ") == 0);
    assert(interpol->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(interpol->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[2]->as.literal.value.string_value, " you are great!") == 0);
    assert(opt.string_literals_merged == 1);

    arena_free(&arena);
}

static void test_string_literal_concat_fold(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: "Hello " + "World" as a binary expression */
    Expr *left = create_string_literal(&arena, "Hello ");
    Expr *right = create_string_literal(&arena, "World");
    Expr *binary = create_binary_expr(&arena, left, TOKEN_PLUS, right);
    binary->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    Expr *result = optimize_string_expr(&opt, binary);

    /* Should fold into a single literal */
    assert(result->type == EXPR_LITERAL);
    assert(strcmp(result->as.literal.value.string_value, "Hello World") == 0);
    assert(opt.string_literals_merged == 1);

    arena_free(&arena);
}

static void test_string_no_merge_different_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Count: " + 42 */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 2;
    interpol->as.interpol.parts = arena_alloc(&arena, 2 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Count: ");
    interpol->as.interpol.parts[1] = create_int_literal(&arena, 42);
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* Should not merge (different types) */
    assert(interpol->as.interpol.part_count == 2);
    assert(opt.string_literals_merged == 0);

    arena_free(&arena);
}

/* ============================================================================
 * Run all tests
 * ============================================================================ */

void run_optimizer_tests(void)
{
    TEST_SECTION("Optimizer Tests");

    /* Terminator detection tests */
    TEST_RUN("stmt_is_terminator_return", test_stmt_is_terminator_return);
    TEST_RUN("stmt_is_terminator_break_continue", test_stmt_is_terminator_break_continue);
    TEST_RUN("stmt_is_terminator_non_terminator", test_stmt_is_terminator_non_terminator);

    /* No-op detection tests */
    TEST_RUN("expr_is_noop_add_zero", test_expr_is_noop_add_zero);
    TEST_RUN("expr_is_noop_sub_zero", test_expr_is_noop_sub_zero);
    TEST_RUN("expr_is_noop_mul_one", test_expr_is_noop_mul_one);
    TEST_RUN("expr_is_noop_div_one", test_expr_is_noop_div_one);
    TEST_RUN("expr_is_noop_double_negation", test_expr_is_noop_double_negation);
    TEST_RUN("expr_is_noop_not_noop", test_expr_is_noop_not_noop);

    /* Unreachable code removal tests */
    TEST_RUN("remove_unreachable_after_return", test_remove_unreachable_after_return);
    TEST_RUN("remove_unreachable_after_break", test_remove_unreachable_after_break);
    TEST_RUN("no_unreachable_statements", test_no_unreachable_statements);

    /* Variable usage tracking tests */
    TEST_RUN("collect_used_variables", test_collect_used_variables);
    TEST_RUN("is_variable_used", test_is_variable_used);

    /* Full optimization pass tests */
    TEST_RUN("optimizer_dead_code_elimination_function", test_optimizer_dead_code_elimination_function);
    TEST_RUN("optimizer_noop_simplification", test_optimizer_noop_simplification);

    /* Tail call optimization tests */
    TEST_RUN("tail_call_detection_simple", test_tail_call_detection_simple);
    TEST_RUN("tail_call_detection_not_tail", test_tail_call_detection_not_tail);
    TEST_RUN("function_has_tail_recursion", test_function_has_tail_recursion);
    TEST_RUN("tail_call_marking", test_tail_call_marking);

    /* String literal merging tests */
    TEST_RUN("string_literal_merge_adjacent", test_string_literal_merge_adjacent);
    TEST_RUN("string_literal_merge_with_variable", test_string_literal_merge_with_variable);
    TEST_RUN("string_literal_concat_fold", test_string_literal_concat_fold);
    TEST_RUN("string_no_merge_different_types", test_string_no_merge_different_types);
}
