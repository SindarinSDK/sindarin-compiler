// tests/type_checker_tests_thread_sync.c
// Tests for thread sync validation and array sync

#include <stdio.h>
#include <string.h>
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/stmt/type_checker_stmt.h"
#include "../ast/ast_expr.h"
#include "../test_harness.h"

/* Test sync on non-variable expression reports error */
static void test_sync_non_variable_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a literal expression (not a variable) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *literal_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);

    /* Create thread sync with literal (invalid - not a variable or spawn) */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, literal_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync on unknown variable reports error */
static void test_sync_unknown_variable_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create variable expression for unknown variable */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "unknownVar", 1, "test.sn", &arena);
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync with unknown variable */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync on non-pending variable reports error */
static void test_sync_non_pending_variable_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal (non-pending) variable */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NOT pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_pending(sym));

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync on non-pending variable */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test valid sync on pending variable returns correct type */
static void test_valid_sync_returns_correct_type(void)
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

    /* Add a pending variable (simulating result of spawn assignment) */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Mark the variable as pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return int type */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that sync transitions symbol from PENDING to SYNCHRONIZED state */
static void test_sync_state_transition(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable with int type */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "threadResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Mark the variable as pending (simulating spawn assignment) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(sym->thread_state == THREAD_STATE_PENDING);

    /* Create variable expression for sync */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync expression */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check the sync - should transition state */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    /* Verify state transitioned to SYNCHRONIZED */
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify subsequent access to the variable is allowed */
    type_checker_reset_error();
    Type *access_result = type_check_expr(var_expr, &table);
    assert(access_result != NULL);
    assert(access_result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with is_array flag true validates array handle */
static void test_array_sync_validates_array_handle(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create two pending thread handle variables */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Create array of variable expressions */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 2, &arr_tok);

    /* Create array sync expression with is_array = true */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check should succeed and return void */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* Verify both variables are now synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-array expression reports error */
static void test_array_sync_non_array_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create a variable (not an array) */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create array sync expression with is_array = true but handle is not EXPR_ARRAY */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, true, &sync_tok);

    /* Type check should fail */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-variable element reports error */
static void test_array_sync_non_variable_element_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create array with literal element (not variable) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *literal = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);

    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = literal;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 1, &arr_tok);

    /* Create array sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check should fail - element is not a variable */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-pending element reports error */
static void test_array_sync_non_pending_element_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create variable that is NOT pending */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    /* Don't mark as pending */

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = var_expr;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 1, &arr_tok);

    /* Create array sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check should fail - element is not pending */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync returns void type */
static void test_array_sync_returns_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create single pending variable in array */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = var_expr;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 1, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync handles mixed states (some pending, some synchronized) */
static void test_array_sync_mixed_states(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create three thread handles */
    Token h1_tok, h2_tok, h3_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 1, "test.sn", &arena);
    setup_token(&h3_tok, TOKEN_IDENTIFIER, "t3", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);
    symbol_table_add_symbol(&table, h3_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    Symbol *h3_sym = symbol_table_lookup_symbol(&table, h3_tok);

    /* t1 is pending, t2 is already synchronized, t3 is pending */
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);
    symbol_table_mark_synchronized(h2_sym); /* Already done */
    symbol_table_mark_pending(h3_sym);

    assert(h1_sym->thread_state == THREAD_STATE_PENDING);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_PENDING);

    /* Create array sync with all three */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr *v3 = ast_create_variable_expr(&arena, h3_tok, &h3_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 3);
    elements[0] = v1;
    elements[1] = v2;
    elements[2] = v3;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 3, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check should succeed - mixed states handled gracefully */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* All should now be synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_thread_sync_main(void)
{
    TEST_SECTION("Thread Sync Type Checker");

    TEST_RUN("sync_non_variable_error", test_sync_non_variable_error);
    TEST_RUN("sync_unknown_variable_error", test_sync_unknown_variable_error);
    TEST_RUN("sync_non_pending_variable_error", test_sync_non_pending_variable_error);
    TEST_RUN("valid_sync_returns_correct_type", test_valid_sync_returns_correct_type);
    TEST_RUN("sync_state_transition", test_sync_state_transition);
    TEST_RUN("array_sync_validates_array_handle", test_array_sync_validates_array_handle);
    TEST_RUN("array_sync_non_array_error", test_array_sync_non_array_error);
    TEST_RUN("array_sync_non_variable_element_error", test_array_sync_non_variable_element_error);
    TEST_RUN("array_sync_non_pending_element_error", test_array_sync_non_pending_element_error);
    TEST_RUN("array_sync_returns_void", test_array_sync_returns_void);
    TEST_RUN("array_sync_mixed_states", test_array_sync_mixed_states);
}
