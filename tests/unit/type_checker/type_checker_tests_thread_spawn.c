// tests/type_checker_tests_thread_spawn.c
// Tests for thread spawn validation and freeze mechanics

#include <assert.h>
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

/* Test array argument is frozen after spawn */
static void test_array_arg_frozen_after_spawn(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create an array variable that will be passed to the function */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    assert(!symbol_table_is_frozen(arr_sym));

    /* Create a function that takes an array parameter and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *));
    param_types[0] = array_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 1);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "processData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with the array as argument */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    args[0] = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 1, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &processData(myData) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* The array argument should now be frozen */
    assert(symbol_table_is_frozen(arr_sym));
    assert(symbol_table_get_freeze_count(arr_sym) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test frozen args stored in pending variable symbol after spawn */
static void test_frozen_args_stored_in_pending_symbol(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create two array variables that will be passed to the function */
    Token arr1_tok, arr2_tok;
    setup_token(&arr1_tok, TOKEN_IDENTIFIER, "data1", 1, "test.sn", &arena);
    setup_token(&arr2_tok, TOKEN_IDENTIFIER, "data2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr1_tok, array_type);
    symbol_table_add_symbol(&table, arr2_tok, array_type);

    Symbol *arr1_sym = symbol_table_lookup_symbol(&table, arr1_tok);
    Symbol *arr2_sym = symbol_table_lookup_symbol(&table, arr2_tok);

    /* Create a function that takes two array parameters and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = array_type;
    param_types[1] = array_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 2);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "combine", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with both arrays as arguments */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    args[0] = ast_create_variable_expr(&arena, arr1_tok, &arr1_tok);
    args[1] = ast_create_variable_expr(&arena, arr2_tok, &arr2_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 2, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &combine(data1, data2) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* Look up the result variable */
    Symbol *result_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(result_sym != NULL);
    assert(symbol_table_is_pending(result_sym));

    /* Verify frozen_args are stored in the pending symbol */
    assert(result_sym->frozen_args != NULL);
    assert(result_sym->frozen_args_count == 2);

    /* Verify both arrays are in the frozen_args */
    bool found_arr1 = false, found_arr2 = false;
    for (int i = 0; i < result_sym->frozen_args_count; i++)
    {
        if (result_sym->frozen_args[i] == arr1_sym) found_arr1 = true;
        if (result_sym->frozen_args[i] == arr2_sym) found_arr2 = true;
    }
    assert(found_arr1);
    assert(found_arr2);

    /* Both arrays should be frozen */
    assert(symbol_table_is_frozen(arr1_sym));
    assert(symbol_table_is_frozen(arr2_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test 'as ref' primitive is frozen after spawn */
static void test_as_ref_primitive_frozen_after_spawn(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create an int variable that will be passed as 'as ref' */
    Token counter_tok;
    setup_token(&counter_tok, TOKEN_IDENTIFIER, "counter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, counter_tok, int_type);

    Symbol *counter_sym = symbol_table_lookup_symbol(&table, counter_tok);
    assert(!symbol_table_is_frozen(counter_sym));

    /* Create a function that takes an int 'as ref' parameter and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *));
    param_types[0] = int_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 1);

    /* Set param_mem_quals to indicate 'as ref' for the first parameter */
    MemoryQualifier *quals = (MemoryQualifier *)arena_alloc(&arena, sizeof(MemoryQualifier));
    quals[0] = MEM_AS_REF;
    func_type->as.function.param_mem_quals = quals;

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "incrementCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with the counter as argument */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    args[0] = ast_create_variable_expr(&arena, counter_tok, &counter_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 1, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &incrementCounter(counter) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* The 'as ref' primitive argument should now be frozen */
    assert(symbol_table_is_frozen(counter_sym));
    assert(symbol_table_get_freeze_count(counter_sym) == 1);

    /* Look up the result variable and verify frozen_args contains the primitive */
    Symbol *result_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(result_sym != NULL);
    assert(result_sym->frozen_args != NULL);
    assert(result_sym->frozen_args_count == 1);
    assert(result_sym->frozen_args[0] == counter_sym);

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
    TEST_RUN("array_arg_frozen_after_spawn", test_array_arg_frozen_after_spawn);
    TEST_RUN("frozen_args_stored_in_pending_symbol", test_frozen_args_stored_in_pending_symbol);
    TEST_RUN("as_ref_primitive_frozen_after_spawn", test_as_ref_primitive_frozen_after_spawn);
    TEST_RUN("spawn_type_mismatch_error", test_spawn_type_mismatch_error);
}
