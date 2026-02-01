// optimizer_tests_string.c
// Tests for string literal merging optimization

/* ============================================================================
 * Test: String Literal Merging
 * ============================================================================ */

static void test_string_literal_merge_adjacent(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Hello " + "World" + "!" */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 3;
    interpol->as.interpol.parts = arena_alloc(&arena, 3 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Hello ");
    interpol->as.interpol.parts[1] = create_string_literal(&arena, "World");
    interpol->as.interpol.parts[2] = create_string_literal(&arena, "!");
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* All three should merge into one */
    assert(interpol->as.interpol.part_count == 1);
    assert(interpol->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[0]->as.literal.value.string_value, "Hello World!") == 0);
    assert(opt.string_literals_merged == 2);  /* 3 merged into 1 = 2 merges */

    arena_free(&arena);
}

static void test_string_literal_merge_with_variable(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Hello " + name + " you are " + "great!" */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 4;
    interpol->as.interpol.parts = arena_alloc(&arena, 4 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Hello ");
    interpol->as.interpol.parts[1] = create_variable_expr(&arena, "name");
    interpol->as.interpol.parts[1]->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);
    interpol->as.interpol.parts[2] = create_string_literal(&arena, " you are ");
    interpol->as.interpol.parts[3] = create_string_literal(&arena, "great!");
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* Should merge parts 2+3, leaving 3 parts: "Hello ", name, " you are great!" */
    assert(interpol->as.interpol.part_count == 3);
    assert(interpol->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[0]->as.literal.value.string_value, "Hello ") == 0);
    assert(interpol->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(interpol->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(interpol->as.interpol.parts[2]->as.literal.value.string_value, " you are great!") == 0);
    assert(opt.string_literals_merged == 1);

    arena_free(&arena);
}

static void test_string_literal_concat_fold(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: "Hello " + "World" as a binary expression */
    Expr *left = create_string_literal(&arena, "Hello ");
    Expr *right = create_string_literal(&arena, "World");
    Expr *binary = create_binary_expr(&arena, left, TOKEN_PLUS, right);
    binary->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    Expr *result = optimize_string_expr(&opt, binary);

    /* Should fold into a single literal */
    assert(result->type == EXPR_LITERAL);
    assert(strcmp(result->as.literal.value.string_value, "Hello World") == 0);
    assert(opt.string_literals_merged == 1);

    arena_free(&arena);
}

static void test_string_no_merge_different_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: $"Count: " + 42 */
    Expr *interpol = arena_alloc(&arena, sizeof(Expr));
    interpol->type = EXPR_INTERPOLATED;
    interpol->as.interpol.part_count = 2;
    interpol->as.interpol.parts = arena_alloc(&arena, 2 * sizeof(Expr *));
    interpol->as.interpol.parts[0] = create_string_literal(&arena, "Count: ");
    interpol->as.interpol.parts[1] = create_int_literal(&arena, 42);
    interpol->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Optimize */
    optimize_string_expr(&opt, interpol);

    /* Should not merge (different types) */
    assert(interpol->as.interpol.part_count == 2);
    assert(opt.string_literals_merged == 0);

    arena_free(&arena);
}

