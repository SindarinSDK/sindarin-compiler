// tests/type_checker_tests_thread_sync.c
// Tests for thread sync validation and array sync

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
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

/* Test that sync unfreezes captured arguments */
static void test_sync_unfreezes_arguments(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array type (arrays are frozen when passed to threads) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Add an array variable */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    /* Freeze the array (simulating spawn capturing it) */
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_is_frozen(arr_sym));

    /* Create a pending thread handle with frozen_args tracking */
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);

    /* Set frozen args on the pending symbol */
    Symbol **frozen_args = (Symbol **)arena_alloc(&arena, sizeof(Symbol *) * 1);
    frozen_args[0] = arr_sym;
    symbol_table_set_frozen_args(handle_sym, frozen_args, 1);

    /* Create sync expression for the handle */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    /* Type check the sync - should unfreeze the array */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    /* Verify the array is now unfrozen */
    assert(!symbol_table_is_frozen(arr_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that frozen argument becomes writable after sync */
static void test_frozen_arg_writable_after_sync(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array type (arrays are frozen when passed to threads) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Add an array variable */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    /* Freeze the array (simulating spawn capturing it) */
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_is_frozen(arr_sym));

    /* Verify array cannot be modified while frozen - create push member access */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    /* Type check push on frozen array should fail */
    type_checker_reset_error();
    Type *frozen_result = type_check_expr(member_expr, &table);
    assert(frozen_result == NULL);
    assert(type_checker_had_error());

    /* Now create a pending thread handle with frozen_args tracking */
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);

    /* Set frozen args on the pending symbol */
    Symbol **frozen_args = (Symbol **)arena_alloc(&arena, sizeof(Symbol *) * 1);
    frozen_args[0] = arr_sym;
    symbol_table_set_frozen_args(handle_sym, frozen_args, 1);

    /* Create and type check sync expression */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    type_checker_reset_error();
    Type *sync_result = type_check_expr(sync_expr, &table);
    assert(sync_result != NULL);
    assert(!type_checker_had_error());

    /* Verify array is now unfrozen */
    assert(!symbol_table_is_frozen(arr_sym));

    /* Now verify we can access push on the unfrozen array - should succeed */
    Expr *arr_var2 = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok2;
    setup_token(&push_tok2, TOKEN_IDENTIFIER, "push", 3, "test.sn", &arena);
    Expr *member_expr2 = ast_create_member_expr(&arena, arr_var2, push_tok2, &push_tok2);

    type_checker_reset_error();
    Type *unfrozen_result = type_check_expr(member_expr2, &table);
    assert(unfrozen_result != NULL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync handles case with no frozen arguments */
static void test_sync_handles_no_frozen_args(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a pending thread handle with no frozen args */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);
    /* frozen_args is NULL by default, frozen_args_count is 0 */
    assert(handle_sym->frozen_args == NULL);
    assert(handle_sym->frozen_args_count == 0);

    /* Create sync expression */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    /* Type check should succeed even with no frozen args */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that multiple freezes are decremented correctly */
static void test_sync_multiple_freezes_decremented(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "sharedArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);

    /* Freeze twice (simulating two threads capturing the same array) */
    symbol_table_freeze_symbol(arr_sym);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_get_freeze_count(arr_sym) == 2);
    assert(symbol_table_is_frozen(arr_sym));

    /* First sync unfreezes once */
    Token handle1_tok;
    setup_token(&handle1_tok, TOKEN_IDENTIFIER, "thread1", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle1_tok, int_type);
    Symbol *handle1_sym = symbol_table_lookup_symbol(&table, handle1_tok);
    symbol_table_mark_pending(handle1_sym);
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = arr_sym;
    symbol_table_set_frozen_args(handle1_sym, frozen1, 1);

    Expr *handle1_expr = ast_create_variable_expr(&arena, handle1_tok, &handle1_tok);
    Token sync1_tok;
    setup_token(&sync1_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync1_expr = ast_create_thread_sync_expr(&arena, handle1_expr, false, &sync1_tok);

    type_checker_reset_error();
    type_check_expr(sync1_expr, &table);
    assert(!type_checker_had_error());

    /* After first sync, still frozen (freeze_count = 1) */
    assert(symbol_table_get_freeze_count(arr_sym) == 1);
    assert(symbol_table_is_frozen(arr_sym));

    /* Second sync unfreezes completely */
    Token handle2_tok;
    setup_token(&handle2_tok, TOKEN_IDENTIFIER, "thread2", 3, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle2_tok, int_type);
    Symbol *handle2_sym = symbol_table_lookup_symbol(&table, handle2_tok);
    symbol_table_mark_pending(handle2_sym);
    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = arr_sym;
    symbol_table_set_frozen_args(handle2_sym, frozen2, 1);

    Expr *handle2_expr = ast_create_variable_expr(&arena, handle2_tok, &handle2_tok);
    Token sync2_tok;
    setup_token(&sync2_tok, TOKEN_BANG, "!", 3, "test.sn", &arena);
    Expr *sync2_expr = ast_create_thread_sync_expr(&arena, handle2_expr, false, &sync2_tok);

    type_checker_reset_error();
    type_check_expr(sync2_expr, &table);
    assert(!type_checker_had_error());

    /* After second sync, completely unfrozen */
    assert(symbol_table_get_freeze_count(arr_sym) == 0);
    assert(!symbol_table_is_frozen(arr_sym));

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

/* Test array sync unfreezes arguments for all synced threads */
static void test_array_sync_unfreezes_all_arguments(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create shared arrays that will be frozen */
    Token arr1_tok, arr2_tok;
    setup_token(&arr1_tok, TOKEN_IDENTIFIER, "sharedArr1", 1, "test.sn", &arena);
    setup_token(&arr2_tok, TOKEN_IDENTIFIER, "sharedArr2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr1_tok, array_type);
    symbol_table_add_symbol(&table, arr2_tok, array_type);

    Symbol *arr1_sym = symbol_table_lookup_symbol(&table, arr1_tok);
    Symbol *arr2_sym = symbol_table_lookup_symbol(&table, arr2_tok);

    /* Freeze both arrays */
    symbol_table_freeze_symbol(arr1_sym);
    symbol_table_freeze_symbol(arr2_sym);
    assert(symbol_table_is_frozen(arr1_sym));
    assert(symbol_table_is_frozen(arr2_sym));

    /* Create two pending thread handles with frozen args */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 2, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Set frozen args on thread handles */
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = arr1_sym;
    symbol_table_set_frozen_args(h1_sym, frozen1, 1);

    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = arr2_sym;
    symbol_table_set_frozen_args(h2_sym, frozen2, 1);

    /* Create array sync */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 2, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    /* Both shared arrays should be unfrozen */
    assert(!symbol_table_is_frozen(arr1_sym));
    assert(!symbol_table_is_frozen(arr2_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with same variable frozen by multiple threads */
static void test_array_sync_shared_frozen_variable(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create a shared array that will be frozen by BOTH threads */
    Token shared_arr_tok;
    setup_token(&shared_arr_tok, TOKEN_IDENTIFIER, "sharedData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, shared_arr_tok, array_type);

    Symbol *shared_arr_sym = symbol_table_lookup_symbol(&table, shared_arr_tok);

    /* Freeze the array TWICE (simulating two threads capturing the same array) */
    symbol_table_freeze_symbol(shared_arr_sym);
    symbol_table_freeze_symbol(shared_arr_sym);
    assert(symbol_table_get_freeze_count(shared_arr_sym) == 2);
    assert(symbol_table_is_frozen(shared_arr_sym));

    /* Create two pending thread handles, BOTH referencing the same frozen array */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 2, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Both thread handles reference the SAME frozen array */
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = shared_arr_sym;
    symbol_table_set_frozen_args(h1_sym, frozen1, 1);

    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = shared_arr_sym;
    symbol_table_set_frozen_args(h2_sym, frozen2, 1);

    /* Create array sync [t1, t2]! */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *sync_list_expr = ast_create_sync_list_expr(&arena, elements, 2, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, sync_list_expr, true, &sync_tok);

    /* Type check - should sync both and decrement freeze_count twice */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* After syncing both threads, freeze_count should be 0 and array unfrozen */
    assert(symbol_table_get_freeze_count(shared_arr_sym) == 0);
    assert(!symbol_table_is_frozen(shared_arr_sym));

    /* Both thread handles should be synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify the array is now writable - test push method access */
    Expr *arr_var = ast_create_variable_expr(&arena, shared_arr_tok, &shared_arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 3, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    type_checker_reset_error();
    Type *push_result = type_check_expr(member_expr, &table);
    assert(push_result != NULL);
    assert(!type_checker_had_error());

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
    TEST_RUN("sync_unfreezes_arguments", test_sync_unfreezes_arguments);
    TEST_RUN("frozen_arg_writable_after_sync", test_frozen_arg_writable_after_sync);
    TEST_RUN("sync_handles_no_frozen_args", test_sync_handles_no_frozen_args);
    TEST_RUN("sync_multiple_freezes_decremented", test_sync_multiple_freezes_decremented);
    TEST_RUN("array_sync_validates_array_handle", test_array_sync_validates_array_handle);
    TEST_RUN("array_sync_non_array_error", test_array_sync_non_array_error);
    TEST_RUN("array_sync_non_variable_element_error", test_array_sync_non_variable_element_error);
    TEST_RUN("array_sync_non_pending_element_error", test_array_sync_non_pending_element_error);
    TEST_RUN("array_sync_returns_void", test_array_sync_returns_void);
    TEST_RUN("array_sync_mixed_states", test_array_sync_mixed_states);
    TEST_RUN("array_sync_unfreezes_all_arguments", test_array_sync_unfreezes_all_arguments);
    TEST_RUN("array_sync_shared_frozen_variable", test_array_sync_shared_frozen_variable);
}
