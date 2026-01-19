// tests/type_checker_tests_thread_access.c
// Tests for variable access rules, frozen method tests, and function constraints

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"
#include "../test_harness.h"

/* Test accessing a pending variable reports error */
static void test_pending_variable_access_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it pending (simulating spawn assignment) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "pendingResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_pending(sym));

    /* Create variable expression to access the pending variable */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test accessing a synchronized variable is allowed */
static void test_synchronized_variable_access_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it synchronized */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "syncedResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    symbol_table_mark_synchronized(sym);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Create variable expression to access the synchronized variable */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test accessing a normal (non-thread) variable is allowed */
static void test_normal_variable_access_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not a thread) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NORMAL state (default) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test all array elements become accessible after sync */
static void test_array_sync_all_elements_accessible(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create three pending thread handles */
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

    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);
    symbol_table_mark_pending(h3_sym);

    /* Create array sync */
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

    /* Sync all */
    type_checker_reset_error();
    type_check_expr(sync_expr, &table);
    assert(!type_checker_had_error());

    /* All should be synchronized (accessible) */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify we can access each variable (type check should succeed) */
    type_checker_reset_error();
    Type *r1 = type_check_expr(v1, &table);
    assert(r1 != NULL);
    assert(r1->kind == TYPE_INT);
    assert(!type_checker_had_error());

    type_checker_reset_error();
    Type *r2 = type_check_expr(v2, &table);
    assert(r2 != NULL);
    assert(r2->kind == TYPE_INT);
    assert(!type_checker_had_error());

    type_checker_reset_error();
    Type *r3 = type_check_expr(v3, &table);
    assert(r3 != NULL);
    assert(r3->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a pending variable reports error */
static void test_pending_variable_reassign_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it pending (simulating spawn assignment) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "pendingResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_pending(sym));

    /* Create assignment expression: pendingResult = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a synchronized variable is allowed */
static void test_synchronized_variable_reassign_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it synchronized */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "syncedResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    symbol_table_mark_synchronized(sym);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Create assignment expression: syncedResult = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a normal (non-thread) variable is allowed */
static void test_normal_variable_reassign_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not a thread) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NORMAL state (default) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    /* Create assignment expression: normalVar = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that mutating methods on frozen arrays report error */
static void test_frozen_array_mutating_method_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add an array variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "frozenArr", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create member expression: frozenArr.push */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(member_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that read-only methods on frozen arrays are allowed */
static void test_frozen_array_readonly_method_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add an array variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "frozenArr", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create member expression: frozenArr.length */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token length_tok;
    setup_token(&length_tok, TOKEN_IDENTIFIER, "length", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, length_tok, &length_tok);

    /* Type check should succeed - length is read-only */
    type_checker_reset_error();
    Type *result = type_check_expr(member_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that incrementing a frozen variable reports error */
static void test_frozen_variable_increment_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "frozenCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create increment expression: frozenCounter++ */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token inc_tok;
    setup_token(&inc_tok, TOKEN_PLUS_PLUS, "++", 1, "test.sn", &arena);
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &inc_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(inc_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that decrementing a frozen variable reports error */
static void test_frozen_variable_decrement_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "frozenCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create decrement expression: frozenCounter-- */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token dec_tok;
    setup_token(&dec_tok, TOKEN_MINUS_MINUS, "--", 1, "test.sn", &arena);
    Expr *dec_expr = ast_create_decrement_expr(&arena, var_expr, &dec_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(dec_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that incrementing a normal variable is allowed */
static void test_normal_variable_increment_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not frozen) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_frozen(sym));

    /* Create increment expression: normalCounter++ */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token inc_tok;
    setup_token(&inc_tok, TOKEN_PLUS_PLUS, "++", 1, "test.sn", &arena);
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &inc_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(inc_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that decrementing a normal variable is allowed */
static void test_normal_variable_decrement_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not frozen) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_frozen(sym));

    /* Create decrement expression: normalCounter-- */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token dec_tok;
    setup_token(&dec_tok, TOKEN_MINUS_MINUS, "--", 1, "test.sn", &arena);
    Expr *dec_expr = ast_create_decrement_expr(&arena, var_expr, &dec_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(dec_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning array type reports error */
static void test_private_function_array_return_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning string type reports error */
static void test_private_function_string_return_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning str */
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *func_type = ast_create_function_type(&arena, string_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getString", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning primitive int is allowed */
static void test_private_function_int_return_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getInt", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning void is allowed */
static void test_private_function_void_return_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning void */
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "doWork", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test default (non-private) function returning array is allowed */
static void test_default_function_array_return_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a default function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_DEFAULT modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_DEFAULT;

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed - default modifier allows any return type */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test shared function returning array is allowed */
static void test_shared_function_array_return_allowed(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a shared function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_SHARED modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_SHARED;

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed - shared modifier allows any return type */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_thread_access_main(void)
{
    TEST_SECTION("Thread Access Type Checker");

    TEST_RUN("pending_variable_access_error", test_pending_variable_access_error);
    TEST_RUN("synchronized_variable_access_allowed", test_synchronized_variable_access_allowed);
    TEST_RUN("normal_variable_access_allowed", test_normal_variable_access_allowed);
    TEST_RUN("array_sync_all_elements_accessible", test_array_sync_all_elements_accessible);
    TEST_RUN("pending_variable_reassign_error", test_pending_variable_reassign_error);
    TEST_RUN("synchronized_variable_reassign_allowed", test_synchronized_variable_reassign_allowed);
    TEST_RUN("normal_variable_reassign_allowed", test_normal_variable_reassign_allowed);
    TEST_RUN("frozen_array_mutating_method_error", test_frozen_array_mutating_method_error);
    TEST_RUN("frozen_array_readonly_method_allowed", test_frozen_array_readonly_method_allowed);
    TEST_RUN("frozen_variable_increment_error", test_frozen_variable_increment_error);
    TEST_RUN("frozen_variable_decrement_error", test_frozen_variable_decrement_error);
    TEST_RUN("normal_variable_increment_allowed", test_normal_variable_increment_allowed);
    TEST_RUN("normal_variable_decrement_allowed", test_normal_variable_decrement_allowed);
    TEST_RUN("private_function_array_return_error", test_private_function_array_return_error);
    TEST_RUN("private_function_string_return_error", test_private_function_string_return_error);
    TEST_RUN("private_function_int_return_allowed", test_private_function_int_return_allowed);
    TEST_RUN("private_function_void_return_allowed", test_private_function_void_return_allowed);
    TEST_RUN("default_function_array_return_allowed", test_default_function_array_return_allowed);
    TEST_RUN("shared_function_array_return_allowed", test_shared_function_array_return_allowed);
}
