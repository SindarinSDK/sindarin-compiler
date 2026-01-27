// tests/unit/standalone/arena_tests_edge_cases.c
// Edge case tests for arena memory management

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../arena.h"
#include "../debug.h"
#include "../test_harness.h"

/* ============================================================================
 * Basic Initialization Edge Cases
 * ============================================================================ */

static void test_arena_init_very_small(void)
{
    Arena arena;
    // Even with size 1, should allocate something usable
    arena_init(&arena, 1);
    assert(arena.first != NULL);
    arena_free(&arena);
}

static void test_arena_init_various_sizes(void)
{
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    for (int i = 0; i < 9; i++) {
        Arena arena;
        arena_init(&arena, sizes[i]);
        assert(arena.first != NULL);
        assert(arena.first->size == sizes[i]);
        arena_free(&arena);
    }
}

static void test_arena_multiple_init_free_cycles(void)
{
    for (int cycle = 0; cycle < 10; cycle++) {
        Arena arena;
        arena_init(&arena, 64);
        for (int j = 0; j < 10; j++) {
            void *p = arena_alloc(&arena, 8);
            assert(p != NULL);
        }
        arena_free(&arena);
    }
}

/* ============================================================================
 * Allocation Edge Cases
 * ============================================================================ */

static void test_arena_edge_alloc_zero(void)
{
    Arena arena;
    arena_init(&arena, 64);

    void *p = arena_alloc(&arena, 0);
    // Zero allocation should still return valid pointer
    assert(p != NULL);

    arena_free(&arena);
}

static void test_arena_alloc_one_byte(void)
{
    Arena arena;
    arena_init(&arena, 64);

    void *p = arena_alloc(&arena, 1);
    assert(p != NULL);

    // Should be aligned to 16
    assert(arena.current_used >= 16);

    arena_free(&arena);
}

static void test_arena_alloc_exact_block_size(void)
{
    Arena arena;
    arena_init(&arena, 64);

    // Allocate exactly block size (but 64 is aligned to 64)
    void *p = arena_alloc(&arena, 64);
    assert(p != NULL);

    // Should need a new block for next allocation
    void *p2 = arena_alloc(&arena, 1);
    assert(p2 != NULL);
    assert(arena.current != arena.first);

    arena_free(&arena);
}

static void test_arena_alloc_boundary_sizes(void)
{
    Arena arena;
    arena_init(&arena, 256);

    // Test allocations at alignment boundaries
    size_t sizes[] = {1, 15, 16, 17, 31, 32, 33, 63, 64, 65};
    for (int i = 0; i < 10; i++) {
        void *p = arena_alloc(&arena, sizes[i]);
        assert(p != NULL);
        // Verify alignment
        assert(((uintptr_t)p % 16) == 0);
    }

    arena_free(&arena);
}

static void test_arena_alloc_many_small(void)
{
    Arena arena;
    arena_init(&arena, 64);

    // Many small allocations
    for (int i = 0; i < 100; i++) {
        void *p = arena_alloc(&arena, 4);
        assert(p != NULL);
    }

    arena_free(&arena);
}

static void test_arena_alloc_alternating_sizes(void)
{
    Arena arena;
    arena_init(&arena, 256);

    // Alternate between small and larger allocations
    for (int i = 0; i < 50; i++) {
        void *p1 = arena_alloc(&arena, 4);
        void *p2 = arena_alloc(&arena, 64);
        assert(p1 != NULL);
        assert(p2 != NULL);
    }

    arena_free(&arena);
}

/* ============================================================================
 * String Duplication Edge Cases
 * ============================================================================ */

static void test_arena_strdup_empty(void)
{
    Arena arena;
    arena_init(&arena, 64);

    char *s = arena_strdup(&arena, "");
    assert(s != NULL);
    assert(strcmp(s, "") == 0);
    assert(strlen(s) == 0);

    arena_free(&arena);
}

static void test_arena_strdup_single_char(void)
{
    Arena arena;
    arena_init(&arena, 64);

    char *s = arena_strdup(&arena, "x");
    assert(s != NULL);
    assert(strcmp(s, "x") == 0);

    arena_free(&arena);
}

static void test_arena_strdup_long_string(void)
{
    Arena arena;
    arena_init(&arena, 64);

    // Create a 500 character string
    char long_str[501];
    memset(long_str, 'a', 500);
    long_str[500] = '\0';

    char *s = arena_strdup(&arena, long_str);
    assert(s != NULL);
    assert(strlen(s) == 500);
    assert(strcmp(s, long_str) == 0);

    arena_free(&arena);
}

static void test_arena_strdup_many(void)
{
    Arena arena;
    arena_init(&arena, 128);

    char *strings[100];
    for (int i = 0; i < 100; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "string_%d", i);
        strings[i] = arena_strdup(&arena, buf);
        assert(strings[i] != NULL);
    }

    // Verify all strings are correct
    for (int i = 0; i < 100; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "string_%d", i);
        assert(strcmp(strings[i], buf) == 0);
    }

    arena_free(&arena);
}

static void test_arena_strdup_special_chars(void)
{
    Arena arena;
    arena_init(&arena, 64);

    char *s1 = arena_strdup(&arena, "hello\nworld");
    assert(strcmp(s1, "hello\nworld") == 0);

    char *s2 = arena_strdup(&arena, "tab\there");
    assert(strcmp(s2, "tab\there") == 0);

    char *s3 = arena_strdup(&arena, "null\0hidden");  // Only copies up to null
    assert(strlen(s3) == 4);

    arena_free(&arena);
}

/* ============================================================================
 * Reuse After Free/Init Cycle Edge Cases
 * ============================================================================ */

static void test_arena_reuse_after_free(void)
{
    Arena arena;

    // First use
    arena_init(&arena, 64);
    arena_alloc(&arena, 32);
    arena_free(&arena);

    // Second use - should work fine
    arena_init(&arena, 64);
    void *p = arena_alloc(&arena, 32);
    assert(p != NULL);
    arena_free(&arena);
}

static void test_arena_reinit_different_sizes(void)
{
    Arena arena;

    arena_init(&arena, 64);
    arena_alloc(&arena, 16);
    arena_free(&arena);

    arena_init(&arena, 256);
    void *p = arena_alloc(&arena, 128);
    assert(p != NULL);
    arena_free(&arena);

    arena_init(&arena, 32);
    p = arena_alloc(&arena, 16);
    assert(p != NULL);
    arena_free(&arena);
}

/* ============================================================================
 * Alignment Edge Cases
 * ============================================================================ */

static void test_arena_alignment_16_byte(void)
{
    Arena arena;
    arena_init(&arena, 256);

    for (int i = 0; i < 20; i++) {
        void *p = arena_alloc(&arena, i + 1);
        // All allocations should be 16-byte aligned
        assert(((uintptr_t)p % 16) == 0);
    }

    arena_free(&arena);
}

static void test_arena_strdup_alignment(void)
{
    Arena arena;
    arena_init(&arena, 256);

    // Strings of various lengths should all be aligned
    for (int len = 1; len <= 50; len++) {
        char *buf = malloc(len + 1);
        memset(buf, 'a', len);
        buf[len] = '\0';

        char *s = arena_strdup(&arena, buf);
        assert(((uintptr_t)s % 16) == 0);

        free(buf);
    }

    arena_free(&arena);
}

/* ============================================================================
 * Block Growth Edge Cases
 * ============================================================================ */

static void test_arena_block_doubling(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Fill first block
    arena_alloc(&arena, 16);

    // Next block should be double
    arena_alloc(&arena, 1);
    assert(arena.block_size == 32);

    // Fill that block too
    arena_alloc(&arena, 16);

    // Next should double again
    arena_alloc(&arena, 1);
    assert(arena.block_size == 64);

    arena_free(&arena);
}

static void test_arena_large_allocation_growth(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Request more than 2x current block size
    arena_alloc(&arena, 100);
    // Block should grow to accommodate
    assert(arena.block_size >= 100);

    arena_free(&arena);
}

/* ============================================================================
 * Pointer Arithmetic Edge Cases
 * ============================================================================ */

static void test_arena_sequential_allocations_contiguous(void)
{
    Arena arena;
    arena_init(&arena, 256);

    // In same block, allocations should be contiguous (with alignment)
    void *p1 = arena_alloc(&arena, 16);
    void *p2 = arena_alloc(&arena, 16);
    void *p3 = arena_alloc(&arena, 16);

    assert((char *)p2 - (char *)p1 == 16);
    assert((char *)p3 - (char *)p2 == 16);

    arena_free(&arena);
}

static void test_arena_no_overlap(void)
{
    Arena arena;
    arena_init(&arena, 128);

    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = arena_alloc(&arena, 8);
    }

    // No allocations should overlap
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            assert(ptrs[i] != ptrs[j]);
            // 8 bytes rounded to 16
            intptr_t diff = (char *)ptrs[j] - (char *)ptrs[i];
            assert(diff >= 16 || diff <= -16);
        }
    }

    arena_free(&arena);
}

/* ============================================================================
 * Usage Tracking Edge Cases
 * ============================================================================ */

static void test_arena_used_tracking(void)
{
    Arena arena;
    arena_init(&arena, 128);

    assert(arena.current_used == 0);

    arena_alloc(&arena, 16);
    assert(arena.current_used == 16);

    arena_alloc(&arena, 16);
    assert(arena.current_used == 32);

    arena_alloc(&arena, 16);
    assert(arena.current_used == 48);

    arena_free(&arena);
}

static void test_arena_used_after_many_allocs(void)
{
    Arena arena;
    arena_init(&arena, 128);

    size_t total = 0;
    for (int i = 0; i < 5; i++) {
        arena_alloc(&arena, 16);
        total += 16;
    }

    assert(arena.current_used >= total || arena.current != arena.first);

    arena_free(&arena);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_arena_many_allocations(void)
{
    Arena arena;
    arena_init(&arena, 64);

    // 1000 allocations of various sizes
    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 64) + 1;
        void *p = arena_alloc(&arena, size);
        assert(p != NULL);
    }

    arena_free(&arena);
}

static void test_arena_mixed_strings_and_data(void)
{
    Arena arena;
    arena_init(&arena, 128);

    for (int i = 0; i < 100; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "str_%d", i);
        char *s = arena_strdup(&arena, buf);
        assert(s != NULL);

        void *data = arena_alloc(&arena, 32);
        assert(data != NULL);
    }

    arena_free(&arena);
}

void test_arena_edge_cases_main(void)
{
    TEST_SECTION("Arena Edge Cases");

    // Initialization
    TEST_RUN("arena_init_very_small", test_arena_init_very_small);
    TEST_RUN("arena_init_various_sizes", test_arena_init_various_sizes);
    TEST_RUN("arena_multiple_init_free_cycles", test_arena_multiple_init_free_cycles);

    // Allocation
    TEST_RUN("arena_edge_alloc_zero", test_arena_edge_alloc_zero);
    TEST_RUN("arena_alloc_one_byte", test_arena_alloc_one_byte);
    TEST_RUN("arena_alloc_exact_block_size", test_arena_alloc_exact_block_size);
    TEST_RUN("arena_alloc_boundary_sizes", test_arena_alloc_boundary_sizes);
    TEST_RUN("arena_alloc_many_small", test_arena_alloc_many_small);
    TEST_RUN("arena_alloc_alternating_sizes", test_arena_alloc_alternating_sizes);

    // String duplication
    TEST_RUN("arena_strdup_empty", test_arena_strdup_empty);
    TEST_RUN("arena_strdup_single_char", test_arena_strdup_single_char);
    TEST_RUN("arena_strdup_long_string", test_arena_strdup_long_string);
    TEST_RUN("arena_strdup_many", test_arena_strdup_many);
    TEST_RUN("arena_strdup_special_chars", test_arena_strdup_special_chars);

    // Reuse
    TEST_RUN("arena_reuse_after_free", test_arena_reuse_after_free);
    TEST_RUN("arena_reinit_different_sizes", test_arena_reinit_different_sizes);

    // Alignment
    TEST_RUN("arena_alignment_16_byte", test_arena_alignment_16_byte);
    TEST_RUN("arena_strdup_alignment", test_arena_strdup_alignment);

    // Block growth
    TEST_RUN("arena_block_doubling", test_arena_block_doubling);
    TEST_RUN("arena_large_allocation_growth", test_arena_large_allocation_growth);

    // Pointer arithmetic
    TEST_RUN("arena_sequential_allocations_contiguous", test_arena_sequential_allocations_contiguous);
    TEST_RUN("arena_no_overlap", test_arena_no_overlap);

    // Usage tracking
    TEST_RUN("arena_used_tracking", test_arena_used_tracking);
    TEST_RUN("arena_used_after_many_allocs", test_arena_used_after_many_allocs);

    // Stress tests
    TEST_RUN("arena_many_allocations", test_arena_many_allocations);
    TEST_RUN("arena_mixed_strings_and_data", test_arena_mixed_strings_and_data);
}
