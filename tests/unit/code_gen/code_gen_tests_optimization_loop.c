/**
 * code_gen_tests_optimization_loop.c - Loop counter tracking tests
 *
 * Tests for loop counter stack operations.
 */

/* Test basic push/pop/check for loop counter stack */
static void test_loop_counter_push_pop(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Initially empty - nothing tracked */
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == false);
    assert(is_tracked_loop_counter(&gen, "i") == false);
    assert(gen.loop_counter_count == 0);

    /* Push first counter */
    push_loop_counter(&gen, "__idx_0__");
    assert(gen.loop_counter_count == 1);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == false);

    /* Push second counter */
    push_loop_counter(&gen, "__idx_1__");
    assert(gen.loop_counter_count == 2);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == true);

    /* Pop second counter */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 1);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == false);

    /* Pop first counter */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 0);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == false);

    /* Pop on empty stack should be safe */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test loop counter stack grows when capacity is exceeded */
static void test_loop_counter_stack_growth(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Initial capacity should be 0 */
    assert(gen.loop_counter_capacity == 0);
    assert(gen.loop_counter_count == 0);

    /* Push many counters to force stack growth */
    char name[32];
    for (int i = 0; i < 20; i++)
    {
        snprintf(name, sizeof(name), "__idx_%d__", i);
        push_loop_counter(&gen, name);
    }

    assert(gen.loop_counter_count == 20);
    assert(gen.loop_counter_capacity >= 20);  /* Should have grown */

    /* Verify all counters are tracked */
    for (int i = 0; i < 20; i++)
    {
        snprintf(name, sizeof(name), "__idx_%d__", i);
        assert(is_tracked_loop_counter(&gen, name) == true);
    }

    /* Counter not in stack */
    assert(is_tracked_loop_counter(&gen, "__idx_99__") == false);

    /* Pop all and verify */
    for (int i = 0; i < 20; i++)
    {
        pop_loop_counter(&gen);
    }
    assert(gen.loop_counter_count == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_tracked_loop_counter with NULL input */
static void test_loop_counter_null_check(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* NULL should always return false */
    assert(is_tracked_loop_counter(&gen, NULL) == false);

    /* Even with items in stack, NULL returns false */
    push_loop_counter(&gen, "__idx_0__");
    assert(is_tracked_loop_counter(&gen, NULL) == false);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);

    pop_loop_counter(&gen);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

static void test_code_gen_optimization_loop_main(void)
{
    TEST_RUN("loop_counter_push_pop", test_loop_counter_push_pop);
    TEST_RUN("loop_counter_stack_growth", test_loop_counter_stack_growth);
    TEST_RUN("loop_counter_null_check", test_loop_counter_null_check);
}
