/**
 * type_checker_tests_memory_context.c - MemoryContext scope_depth tracking tests
 *
 * Tests for MemoryContext scope tracking functionality.
 */

static void test_memory_context_scope_depth_init()
{
    MemoryContext ctx;
    memory_context_init(&ctx);

    // After init, scope_depth should be 0
    assert(memory_context_get_scope_depth(&ctx) == 0);
}

static void test_memory_context_scope_depth_enter_exit()
{
    MemoryContext ctx;
    memory_context_init(&ctx);

    // Enter scope increments depth
    memory_context_enter_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 1);

    memory_context_enter_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 2);

    memory_context_enter_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 3);

    // Exit scope decrements depth
    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 2);

    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 1);

    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 0);
}

static void test_memory_context_scope_depth_bounds()
{
    MemoryContext ctx;
    memory_context_init(&ctx);

    // Exiting scope at depth 0 should not go negative
    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 0);

    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 0);
}

static void test_memory_context_scope_depth_null()
{
    // NULL context should return 0
    assert(memory_context_get_scope_depth(NULL) == 0);

    // Enter/exit on NULL should not crash
    memory_context_enter_scope(NULL);
    memory_context_exit_scope(NULL);
}

static void test_memory_context_scope_depth_nested_deep()
{
    MemoryContext ctx;
    memory_context_init(&ctx);

    // Push 10 nested scopes
    for (int i = 0; i < 10; i++) {
        memory_context_enter_scope(&ctx);
        assert(memory_context_get_scope_depth(&ctx) == i + 1);
    }

    assert(memory_context_get_scope_depth(&ctx) == 10);

    // Pop all 10 scopes
    for (int i = 10; i > 0; i--) {
        assert(memory_context_get_scope_depth(&ctx) == i);
        memory_context_exit_scope(&ctx);
    }

    assert(memory_context_get_scope_depth(&ctx) == 0);
}

static void test_memory_context_scope_with_private()
{
    // Test that scope_depth is independent of private_depth
    MemoryContext ctx;
    memory_context_init(&ctx);

    // Enter private block and some scopes
    memory_context_enter_private(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 0);  // scope_depth unaffected

    memory_context_enter_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 1);
    assert(memory_context_is_private(&ctx) == true);

    memory_context_exit_scope(&ctx);
    assert(memory_context_get_scope_depth(&ctx) == 0);
    assert(memory_context_is_private(&ctx) == true);

    memory_context_exit_private(&ctx);
    assert(memory_context_is_private(&ctx) == false);
}

static void test_type_checker_memory_context_main()
{
    TEST_RUN("memory_context_scope_depth_init", test_memory_context_scope_depth_init);
    TEST_RUN("memory_context_scope_depth_enter_exit", test_memory_context_scope_depth_enter_exit);
    TEST_RUN("memory_context_scope_depth_bounds", test_memory_context_scope_depth_bounds);
    TEST_RUN("memory_context_scope_depth_null", test_memory_context_scope_depth_null);
    TEST_RUN("memory_context_scope_depth_nested_deep", test_memory_context_scope_depth_nested_deep);
    TEST_RUN("memory_context_scope_with_private", test_memory_context_scope_with_private);
}
