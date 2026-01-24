// tests/arena_tests.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../arena.h"
#include "../debug.h"
#include "../test_harness.h"

static void test_arena_init(void)
{
    Arena arena;
    size_t initial_size = 16;
    arena_init(&arena, initial_size);
    assert(arena.first != NULL);
    assert(arena.first->data != NULL);
    assert(arena.first->size == initial_size);
    assert(arena.first->next == NULL);
    assert(arena.current == arena.first);
    assert(arena.current_used == 0);
    assert(arena.block_size == initial_size);
    arena_free(&arena);
}

static void test_arena_alloc_small(void)
{
    Arena arena;
    arena_init(&arena, 32);

    // Allocate 4 bytes (aligned to 16)
    void *p1 = arena_alloc(&arena, 4);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 16);

    // Allocate another 4 bytes (aligned to 16)
    void *p2 = arena_alloc(&arena, 4);
    assert(p2 == (char *)arena.current->data + 16);
    assert(arena.current_used == 32);

    // Allocate 1 byte, should create new block
    void *p3 = arena_alloc(&arena, 1);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 64); // Doubled from 32
    assert(arena.block_size == 64);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 16);

    arena_free(&arena);
}

static void test_arena_alloc_large(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Allocate something small first
    arena_alloc(&arena, 4); // Uses 16 bytes (aligned to 16), fills block

    // Allocate 20 bytes (aligned to 32), needs new block
    void *p1 = arena_alloc(&arena, 20);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 32);
    assert(arena.block_size == 32);
    assert(arena.current_used == 32);
    assert(p1 == arena.current->data);

    // Allocate 50 bytes (aligned to 64), needs new block
    void *p2 = arena_alloc(&arena, 50);
    assert(arena.current == arena.first->next->next);
    assert(arena.current->size == 64);
    assert(arena.block_size == 64);
    assert(arena.current_used == 64);
    assert(p2 == arena.current->data);

    // Allocate 100 bytes (aligned to 112), needs new block
    void *p3 = arena_alloc(&arena, 100);
    assert(arena.current == arena.first->next->next->next);
    assert(arena.current->size == 128);
    assert(arena.block_size == 128);
    assert(arena.current_used == 112);
    assert(p3 == arena.current->data);

    arena_free(&arena);
}

static void test_arena_alloc_larger_than_double(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Allocate 100 bytes (aligned 112), needs block larger than 2x
    void *p1 = arena_alloc(&arena, 100);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 112);
    assert(arena.block_size == 112);
    assert(arena.current_used == 112);
    assert(p1 == arena.current->data);

    arena_free(&arena);
}

static void test_arena_alloc_zero(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Allocate 0 bytes, should return current ptr but not advance
    void *p1 = arena_alloc(&arena, 0);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 0);

    // Next alloc should be at same place
    void *p2 = arena_alloc(&arena, 0);
    assert(p2 == p1);
    assert(arena.current_used == 0);

    // Now alloc 1, should advance
    void *p3 = arena_alloc(&arena, 1);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 16);

    arena_free(&arena);
}

static void test_arena_strdup(void)
{
    Arena arena;
    arena_init(&arena, 32);

    // NULL input
    char *s1 = arena_strdup(&arena, NULL);
    assert(s1 == NULL);

    // Empty string (1 byte + null = 1, aligned to 16)
    char *s2 = arena_strdup(&arena, "");
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 16);

    // Normal string "hello" (6 bytes, aligned to 16)
    char *s3 = arena_strdup(&arena, "hello");
    assert(strcmp(s3, "hello") == 0);
    assert(arena.current_used == 32);

    // Longer string to force new block (24 bytes, aligned to 32)
    char *s4 = arena_strdup(&arena, "this is a longer string");
    assert(strcmp(s4, "this is a longer string") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 32);

    arena_free(&arena);
}

static void test_arena_strndup(void)
{
    Arena arena;
    arena_init(&arena, 32);

    // NULL input
    char *s1 = arena_strndup(&arena, NULL, 5);
    assert(s1 == NULL);

    // Empty string, n=5 (1 byte, aligned to 16)
    char *s2 = arena_strndup(&arena, "", 5);
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 16);

    // String "hello", n=3 -> "hel" (4 bytes, aligned to 16)
    char *s3 = arena_strndup(&arena, "hello", 3);
    assert(strcmp(s3, "hel") == 0);
    assert(arena.current_used == 32);

    // String "hello", n=10 > len -> "hello" (6 bytes, aligned to 16), new block
    char *s4 = arena_strndup(&arena, "hello", 10);
    assert(strcmp(s4, "hello") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 16);

    // String "abc", n=0 -> "" (1 byte, aligned to 16)
    char *s5 = arena_strndup(&arena, "abc", 0);
    assert(strcmp(s5, "") == 0);
    assert(arena.current_used == 32);

    arena_free(&arena);
}

static void test_arena_free(void)
{
    Arena arena;
    arena_init(&arena, 16);

    // Allocate some stuff to create multiple blocks
    arena_alloc(&arena, 10);
    arena_alloc(&arena, 10);
    arena_strdup(&arena, "test");

    arena_free(&arena);
    assert(arena.first == NULL);
    assert(arena.current == NULL);
    assert(arena.current_used == 0);
    assert(arena.block_size == 0);

    // Should be able to re-init after free
    arena_init(&arena, 32);
    assert(arena.first != NULL);
    assert(arena.first->size == 32);
    arena_free(&arena);
}

void test_arena_main(void)
{
    TEST_SECTION("Arena");

    TEST_RUN("arena_init", test_arena_init);
    TEST_RUN("arena_alloc_small", test_arena_alloc_small);
    TEST_RUN("arena_alloc_large", test_arena_alloc_large);
    TEST_RUN("arena_alloc_larger_than_double", test_arena_alloc_larger_than_double);
    TEST_RUN("arena_alloc_zero", test_arena_alloc_zero);
    TEST_RUN("arena_strdup", test_arena_strdup);
    TEST_RUN("arena_strndup", test_arena_strndup);
    TEST_RUN("arena_free", test_arena_free);
}
