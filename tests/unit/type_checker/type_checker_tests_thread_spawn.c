// tests/type_checker_tests_thread_spawn.c
// Tests for thread spawn validation

#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"
#include "../test_harness.h"

/* Test spawn with non-call expression reports error */
static void test_thread_spawn_non_call_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a thread spawn expression with a literal instead of a call */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);

    /* Create a literal expression (not a call) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *literal_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &spawn_tok);

    /* Create thread spawn with literal (invalid) */
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, literal_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test spawn with non-function callee reports error */
static void test_thread_spawn_non_function_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a non-function variable to symbol table */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Create a call expression to the non-function variable */
    Expr *callee = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &var_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test void spawn assignment reports error */
static void test_void_spawn_assignment_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a void function type */
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "doWork", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression to the void function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* The spawn expression itself should type-check to void */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);

    /* Now create a var declaration trying to assign the void spawn */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "result", 2, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement - should report error */
    type_checker_reset_error();
    type_check_stmt(var_decl, &table, void_type);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test valid non-void spawn returns correct type */
static void test_valid_spawn_returns_correct_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return int */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test pending state is marked on result variable */
static void test_pending_state_marked_on_spawn_assignment(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &compute() */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* Look up the result variable and verify it's pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(sym != NULL);
    assert(symbol_table_is_pending(sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test spawn with wrong return type for variable declaration */
static void test_spawn_type_mismatch_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning string */
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *func_type = ast_create_function_type(&arena, string_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getString", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration with wrong type: var r: int = &getString() */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check should report error */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_thread_spawn_main(void)
{
    TEST_SECTION("Thread Spawn Type Checker");

    TEST_RUN("spawn_non_call_error", test_thread_spawn_non_call_error);
    TEST_RUN("spawn_non_function_error", test_thread_spawn_non_function_error);
    TEST_RUN("void_spawn_assignment_error", test_void_spawn_assignment_error);
    TEST_RUN("valid_spawn_returns_correct_type", test_valid_spawn_returns_correct_type);
    TEST_RUN("pending_state_marked_on_spawn_assignment", test_pending_state_marked_on_spawn_assignment);
    TEST_RUN("spawn_type_mismatch_error", test_spawn_type_mismatch_error);
}
