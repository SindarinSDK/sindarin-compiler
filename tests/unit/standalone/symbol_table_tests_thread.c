// tests/unit/standalone/symbol_table_tests_thread.c
// Thread state and frozen state tracking tests for symbol table

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../symbol_table.h"
#include "../ast.h"
#include "../test_harness.h"

// Helper macros and constants
#define TEST_ARENA_SIZE 4096
#define TOKEN_LITERAL(str) ((Token){ .start = str, .length = sizeof(str) - 1, .line = 1, .type = TOKEN_IDENTIFIER })

// Helper to create a simple int type
static Type *create_int_type_thread(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

// =====================================================
// Thread State Tracking Tests
// =====================================================

// Test that symbols start in THREAD_STATE_NORMAL
static void test_thread_state_initial_normal(void) {
    DEBUG_INFO("Starting test_thread_state_initial_normal");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("thread_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->thread_state == THREAD_STATE_NORMAL);
    assert(symbol_table_is_pending(sym) == false);
    assert(symbol_table_is_synchronized(sym) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_initial_normal");
}

// Test mark_pending transitions to PENDING state
static void test_thread_state_mark_pending(void) {
    DEBUG_INFO("Starting test_thread_state_mark_pending");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("pending_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    // Transition NORMAL -> PENDING
    bool result = symbol_table_mark_pending(sym);
    assert(result == true);
    assert(sym->thread_state == THREAD_STATE_PENDING);
    assert(symbol_table_is_pending(sym) == true);
    assert(symbol_table_is_synchronized(sym) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_mark_pending");
}

// Test mark_synchronized transitions to SYNCHRONIZED state
static void test_thread_state_mark_synchronized(void) {
    DEBUG_INFO("Starting test_thread_state_mark_synchronized");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("sync_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // First mark as pending
    bool result = symbol_table_mark_pending(sym);
    assert(result == true);
    assert(sym->thread_state == THREAD_STATE_PENDING);

    // Then mark as synchronized (PENDING -> SYNCHRONIZED)
    result = symbol_table_mark_synchronized(sym);
    assert(result == true);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(symbol_table_is_pending(sym) == false);
    assert(symbol_table_is_synchronized(sym) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_mark_synchronized");
}

// Test is_pending query returns correct values
static void test_thread_state_is_pending_query(void) {
    DEBUG_INFO("Starting test_thread_state_is_pending_query");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("query_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Normal state - not pending
    assert(symbol_table_is_pending(sym) == false);

    // Pending state - is pending
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_pending(sym) == true);

    // Synchronized state - not pending
    symbol_table_mark_synchronized(sym);
    assert(symbol_table_is_pending(sym) == false);

    // NULL symbol - returns false
    assert(symbol_table_is_pending(NULL) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_is_pending_query");
}

// Test invalid state transitions are prevented
static void test_thread_state_invalid_transitions(void) {
    DEBUG_INFO("Starting test_thread_state_invalid_transitions");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("invalid_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Test: Cannot mark synchronized from NORMAL (must be PENDING first)
    bool result = symbol_table_mark_synchronized(sym);
    assert(result == false);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    // Test: Mark pending from NORMAL is valid
    result = symbol_table_mark_pending(sym);
    assert(result == true);
    assert(sym->thread_state == THREAD_STATE_PENDING);

    // Test: Cannot mark pending again (already PENDING)
    result = symbol_table_mark_pending(sym);
    assert(result == false);
    assert(sym->thread_state == THREAD_STATE_PENDING);

    // Test: Mark synchronized from PENDING is valid
    result = symbol_table_mark_synchronized(sym);
    assert(result == true);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    // Test: Cannot mark pending from SYNCHRONIZED
    result = symbol_table_mark_pending(sym);
    assert(result == false);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    // Test: Cannot mark synchronized again (already SYNCHRONIZED)
    result = symbol_table_mark_synchronized(sym);
    assert(result == false);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    // Test: NULL symbol returns false
    assert(symbol_table_mark_pending(NULL) == false);
    assert(symbol_table_mark_synchronized(NULL) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_invalid_transitions");
}

// Test is_synchronized query
static void test_thread_state_is_synchronized_query(void) {
    DEBUG_INFO("Starting test_thread_state_is_synchronized_query");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("sync_query_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Normal state - not synchronized
    assert(symbol_table_is_synchronized(sym) == false);

    // Pending state - not synchronized
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_synchronized(sym) == false);

    // Synchronized state - is synchronized
    symbol_table_mark_synchronized(sym);
    assert(symbol_table_is_synchronized(sym) == true);

    // NULL symbol - returns false
    assert(symbol_table_is_synchronized(NULL) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_thread_state_is_synchronized_query");
}

// =====================================================
// Frozen State Tracking Tests
// =====================================================

// Test freeze_symbol increments freeze_count
static void test_frozen_state_freeze_increments_count(void) {
    DEBUG_INFO("Starting test_frozen_state_freeze_increments_count");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("freeze_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Initial state: not frozen, count = 0
    assert(sym->frozen_state.frozen == false);
    assert(sym->frozen_state.freeze_count == 0);
    assert(symbol_table_is_frozen(sym) == false);
    assert(symbol_table_get_freeze_count(sym) == 0);

    // First freeze: count = 1, frozen = true
    bool result = symbol_table_freeze_symbol(sym);
    assert(result == true);
    assert(sym->frozen_state.freeze_count == 1);
    assert(sym->frozen_state.frozen == true);
    assert(symbol_table_is_frozen(sym) == true);
    assert(symbol_table_get_freeze_count(sym) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_freeze_increments_count");
}

// Test multiple freezes accumulate count correctly
static void test_frozen_state_multiple_freezes(void) {
    DEBUG_INFO("Starting test_frozen_state_multiple_freezes");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("multi_freeze_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Freeze multiple times
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 1);
    assert(symbol_table_is_frozen(sym) == true);

    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 2);
    assert(symbol_table_is_frozen(sym) == true);

    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 3);
    assert(symbol_table_is_frozen(sym) == true);

    // Still frozen with count = 3
    assert(sym->frozen_state.frozen == true);
    assert(sym->frozen_state.freeze_count == 3);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_multiple_freezes");
}

// Test unfreeze_symbol decrements count
static void test_frozen_state_unfreeze_decrements_count(void) {
    DEBUG_INFO("Starting test_frozen_state_unfreeze_decrements_count");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("unfreeze_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Freeze twice
    symbol_table_freeze_symbol(sym);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 2);

    // Unfreeze once: count = 1, still frozen
    bool result = symbol_table_unfreeze_symbol(sym);
    assert(result == true);
    assert(symbol_table_get_freeze_count(sym) == 1);
    assert(symbol_table_is_frozen(sym) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_unfreeze_decrements_count");
}

// Test frozen=false when count reaches 0
static void test_frozen_state_unfreezes_at_zero(void) {
    DEBUG_INFO("Starting test_frozen_state_unfreezes_at_zero");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("zero_count_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Freeze twice
    symbol_table_freeze_symbol(sym);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 2);
    assert(symbol_table_is_frozen(sym) == true);

    // Unfreeze once: count = 1, still frozen
    symbol_table_unfreeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 1);
    assert(symbol_table_is_frozen(sym) == true);

    // Unfreeze again: count = 0, now unfrozen
    symbol_table_unfreeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 0);
    assert(symbol_table_is_frozen(sym) == false);
    assert(sym->frozen_state.frozen == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_unfreezes_at_zero");
}

// Test is_frozen query returns correct values
static void test_frozen_state_is_frozen_query(void) {
    DEBUG_INFO("Starting test_frozen_state_is_frozen_query");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("is_frozen_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Initially not frozen
    assert(symbol_table_is_frozen(sym) == false);

    // After freeze: is frozen
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym) == true);

    // After unfreeze: not frozen
    symbol_table_unfreeze_symbol(sym);
    assert(symbol_table_is_frozen(sym) == false);

    // NULL symbol returns false
    assert(symbol_table_is_frozen(NULL) == false);

    // get_freeze_count with NULL returns 0
    assert(symbol_table_get_freeze_count(NULL) == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_is_frozen_query");
}

// Test unfreeze prevents negative counts
static void test_frozen_state_prevents_negative_count(void) {
    DEBUG_INFO("Starting test_frozen_state_prevents_negative_count");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_thread(&arena);
    Token name = TOKEN_LITERAL("negative_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    // Try to unfreeze without freezing first
    bool result = symbol_table_unfreeze_symbol(sym);
    assert(result == false);  // Should fail
    assert(symbol_table_get_freeze_count(sym) == 0);  // Count stays at 0

    // Freeze once, unfreeze once
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 1);

    symbol_table_unfreeze_symbol(sym);
    assert(symbol_table_get_freeze_count(sym) == 0);

    // Try to unfreeze again - should fail
    result = symbol_table_unfreeze_symbol(sym);
    assert(result == false);
    assert(symbol_table_get_freeze_count(sym) == 0);  // Stays at 0, not negative

    // NULL unfreeze returns false
    assert(symbol_table_unfreeze_symbol(NULL) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_frozen_state_prevents_negative_count");
}

void test_symbol_table_thread_main(void)
{
    TEST_SECTION("Symbol Table Thread State");

    TEST_RUN("thread_state_initial_normal", test_thread_state_initial_normal);
    TEST_RUN("thread_state_mark_pending", test_thread_state_mark_pending);
    TEST_RUN("thread_state_mark_synchronized", test_thread_state_mark_synchronized);
    TEST_RUN("thread_state_is_pending_query", test_thread_state_is_pending_query);
    TEST_RUN("thread_state_invalid_transitions", test_thread_state_invalid_transitions);
    TEST_RUN("thread_state_is_synchronized_query", test_thread_state_is_synchronized_query);

    TEST_SECTION("Symbol Table Frozen State");

    TEST_RUN("frozen_state_freeze_increments_count", test_frozen_state_freeze_increments_count);
    TEST_RUN("frozen_state_multiple_freezes", test_frozen_state_multiple_freezes);
    TEST_RUN("frozen_state_unfreeze_decrements_count", test_frozen_state_unfreeze_decrements_count);
    TEST_RUN("frozen_state_unfreezes_at_zero", test_frozen_state_unfreezes_at_zero);
    TEST_RUN("frozen_state_is_frozen_query", test_frozen_state_is_frozen_query);
    TEST_RUN("frozen_state_prevents_negative_count", test_frozen_state_prevents_negative_count);
}
