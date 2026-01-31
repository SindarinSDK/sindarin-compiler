// tests/type_checker_tests_struct_circular.c
// Circular dependency detection tests for struct type checker

/* ============================================================================
 * Circular Dependency Detection Tests
 * ============================================================================ */

/* Test: direct circular dependency (struct A contains field of type A) - should fail */
static void test_struct_direct_circular_dependency()
{
    DEBUG_INFO("Starting test_struct_direct_circular_dependency");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct Node with field of type Node (direct cycle) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Node", 1, "test.sn", &arena);

    /* Manually create the self-referencing struct type without using ast_create_struct_type
     * which would try to clone field types (causing infinite recursion) */
    Type *node_type = arena_alloc(&arena, sizeof(Type));
    memset(node_type, 0, sizeof(Type));
    node_type->kind = TYPE_STRUCT;
    node_type->as.struct_type.name = arena_strdup(&arena, "Node");
    node_type->as.struct_type.field_count = 2;
    node_type->as.struct_type.is_native = false;
    node_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    /* Field 0: value: int */
    node_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    node_type->as.struct_type.fields[0].type = int_type;
    node_type->as.struct_type.fields[0].offset = 0;
    node_type->as.struct_type.fields[0].default_value = NULL;
    node_type->as.struct_type.fields[0].c_alias = NULL;

    /* Field 1: next: Node (self-reference) */
    node_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    node_type->as.struct_type.fields[1].type = node_type;  /* Direct self-reference! */
    node_type->as.struct_type.fields[1].offset = 0;
    node_type->as.struct_type.fields[1].default_value = NULL;
    node_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type here as it calls ast_clone_type
     * which will infinitely recurse on self-referential types.
     * The type checker only needs the struct_decl fields, not the symbol table entry. */

    /* Create the struct declaration using the same fields pointer */
    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = node_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = false;

    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - direct circular dependency */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_direct_circular_dependency");
}

/* Test: indirect circular dependency (A -> B -> A) - should fail */
static void test_struct_indirect_circular_dependency()
{
    DEBUG_INFO("Starting test_struct_indirect_circular_dependency");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Manually create both struct types with all fields pre-configured.
     * We cannot use symbol_table_add_type because it calls ast_clone_type
     * which would infinitely recurse on circular structures. */

    /* Create struct A */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "StructA", 1, "test.sn", &arena);

    Type *struct_a = arena_alloc(&arena, sizeof(Type));
    memset(struct_a, 0, sizeof(Type));
    struct_a->kind = TYPE_STRUCT;
    struct_a->as.struct_type.name = arena_strdup(&arena, "StructA");
    struct_a->as.struct_type.field_count = 2;
    struct_a->as.struct_type.is_native = false;
    struct_a->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_a->as.struct_type.fields[0].name = arena_strdup(&arena, "value_a");
    struct_a->as.struct_type.fields[0].type = int_type;
    struct_a->as.struct_type.fields[0].offset = 0;
    struct_a->as.struct_type.fields[0].default_value = NULL;
    struct_a->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct B */
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "StructB", 2, "test.sn", &arena);

    Type *struct_b = arena_alloc(&arena, sizeof(Type));
    memset(struct_b, 0, sizeof(Type));
    struct_b->kind = TYPE_STRUCT;
    struct_b->as.struct_type.name = arena_strdup(&arena, "StructB");
    struct_b->as.struct_type.field_count = 2;
    struct_b->as.struct_type.is_native = false;
    struct_b->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_b->as.struct_type.fields[0].name = arena_strdup(&arena, "value_b");
    struct_b->as.struct_type.fields[0].type = int_type;
    struct_b->as.struct_type.fields[0].offset = 0;
    struct_b->as.struct_type.fields[0].default_value = NULL;
    struct_b->as.struct_type.fields[0].c_alias = NULL;
    struct_b->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_a");
    struct_b->as.struct_type.fields[1].type = struct_a;  /* B -> A */
    struct_b->as.struct_type.fields[1].offset = 0;
    struct_b->as.struct_type.fields[1].default_value = NULL;
    struct_b->as.struct_type.fields[1].c_alias = NULL;

    /* Complete the cycle: A -> B */
    struct_a->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_b");
    struct_a->as.struct_type.fields[1].type = struct_b;  /* A -> B */
    struct_a->as.struct_type.fields[1].offset = 0;
    struct_a->as.struct_type.fields[1].default_value = NULL;
    struct_a->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on circular references. */

    /* Create struct declarations */
    Stmt *a_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(a_decl, 0, sizeof(Stmt));
    a_decl->type = STMT_STRUCT_DECL;
    a_decl->as.struct_decl.name = a_tok;
    a_decl->as.struct_decl.fields = struct_a->as.struct_type.fields;
    a_decl->as.struct_decl.field_count = 2;
    a_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, a_decl);

    Stmt *b_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(b_decl, 0, sizeof(Stmt));
    b_decl->type = STMT_STRUCT_DECL;
    b_decl->as.struct_decl.name = b_tok;
    b_decl->as.struct_decl.fields = struct_b->as.struct_type.fields;
    b_decl->as.struct_decl.field_count = 2;
    b_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, b_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - indirect circular dependency A -> B -> A */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_indirect_circular_dependency");
}

/* Test: multi-level circular chain (A -> B -> C -> A) - should fail */
static void test_struct_multi_level_circular_chain()
{
    DEBUG_INFO("Starting test_struct_multi_level_circular_chain");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Manually create all three struct types with fields pre-configured.
     * We cannot use symbol_table_add_type because it calls ast_clone_type
     * which would infinitely recurse on circular structures. */

    /* Create struct A */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "LevelA", 1, "test.sn", &arena);
    Type *struct_a = arena_alloc(&arena, sizeof(Type));
    memset(struct_a, 0, sizeof(Type));
    struct_a->kind = TYPE_STRUCT;
    struct_a->as.struct_type.name = arena_strdup(&arena, "LevelA");
    struct_a->as.struct_type.field_count = 2;
    struct_a->as.struct_type.is_native = false;
    struct_a->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_a->as.struct_type.fields[0].name = arena_strdup(&arena, "value_a");
    struct_a->as.struct_type.fields[0].type = int_type;
    struct_a->as.struct_type.fields[0].offset = 0;
    struct_a->as.struct_type.fields[0].default_value = NULL;
    struct_a->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct B */
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "LevelB", 2, "test.sn", &arena);
    Type *struct_b = arena_alloc(&arena, sizeof(Type));
    memset(struct_b, 0, sizeof(Type));
    struct_b->kind = TYPE_STRUCT;
    struct_b->as.struct_type.name = arena_strdup(&arena, "LevelB");
    struct_b->as.struct_type.field_count = 2;
    struct_b->as.struct_type.is_native = false;
    struct_b->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_b->as.struct_type.fields[0].name = arena_strdup(&arena, "value_b");
    struct_b->as.struct_type.fields[0].type = int_type;
    struct_b->as.struct_type.fields[0].offset = 0;
    struct_b->as.struct_type.fields[0].default_value = NULL;
    struct_b->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct C */
    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "LevelC", 3, "test.sn", &arena);
    Type *struct_c = arena_alloc(&arena, sizeof(Type));
    memset(struct_c, 0, sizeof(Type));
    struct_c->kind = TYPE_STRUCT;
    struct_c->as.struct_type.name = arena_strdup(&arena, "LevelC");
    struct_c->as.struct_type.field_count = 2;
    struct_c->as.struct_type.is_native = false;
    struct_c->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_c->as.struct_type.fields[0].name = arena_strdup(&arena, "value_c");
    struct_c->as.struct_type.fields[0].type = int_type;
    struct_c->as.struct_type.fields[0].offset = 0;
    struct_c->as.struct_type.fields[0].default_value = NULL;
    struct_c->as.struct_type.fields[0].c_alias = NULL;
    struct_c->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_a");
    struct_c->as.struct_type.fields[1].type = struct_a;  /* C -> A */
    struct_c->as.struct_type.fields[1].offset = 0;
    struct_c->as.struct_type.fields[1].default_value = NULL;
    struct_c->as.struct_type.fields[1].c_alias = NULL;

    /* B references C: B -> C */
    struct_b->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_c");
    struct_b->as.struct_type.fields[1].type = struct_c;
    struct_b->as.struct_type.fields[1].offset = 0;
    struct_b->as.struct_type.fields[1].default_value = NULL;
    struct_b->as.struct_type.fields[1].c_alias = NULL;

    /* A references B: A -> B, completing the cycle A -> B -> C -> A */
    struct_a->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_b");
    struct_a->as.struct_type.fields[1].type = struct_b;
    struct_a->as.struct_type.fields[1].offset = 0;
    struct_a->as.struct_type.fields[1].default_value = NULL;
    struct_a->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on circular references. */

    /* Create struct declarations */
    Stmt *a_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(a_decl, 0, sizeof(Stmt));
    a_decl->type = STMT_STRUCT_DECL;
    a_decl->as.struct_decl.name = a_tok;
    a_decl->as.struct_decl.fields = struct_a->as.struct_type.fields;
    a_decl->as.struct_decl.field_count = 2;
    a_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, a_decl);

    Stmt *b_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(b_decl, 0, sizeof(Stmt));
    b_decl->type = STMT_STRUCT_DECL;
    b_decl->as.struct_decl.name = b_tok;
    b_decl->as.struct_decl.fields = struct_b->as.struct_type.fields;
    b_decl->as.struct_decl.field_count = 2;
    b_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, b_decl);

    Stmt *c_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(c_decl, 0, sizeof(Stmt));
    c_decl->type = STMT_STRUCT_DECL;
    c_decl->as.struct_decl.name = c_tok;
    c_decl->as.struct_decl.fields = struct_c->as.struct_type.fields;
    c_decl->as.struct_decl.field_count = 2;
    c_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, c_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - multi-level circular chain */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_multi_level_circular_chain");
}

/* Test: pointer to self is allowed (breaks cycle) - should pass */
static void test_struct_pointer_breaks_cycle()
{
    DEBUG_INFO("Starting test_struct_pointer_breaks_cycle");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct Node with *Node pointer field (valid - pointer breaks cycle) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "LinkedNode", 1, "test.sn", &arena);

    /* Manually create struct to avoid clone issues */
    Type *node_type = arena_alloc(&arena, sizeof(Type));
    memset(node_type, 0, sizeof(Type));
    node_type->kind = TYPE_STRUCT;
    node_type->as.struct_type.name = arena_strdup(&arena, "LinkedNode");
    node_type->as.struct_type.field_count = 2;
    node_type->as.struct_type.is_native = true;  /* native struct allows pointers */
    node_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    node_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    node_type->as.struct_type.fields[0].type = int_type;
    node_type->as.struct_type.fields[0].offset = 0;
    node_type->as.struct_type.fields[0].default_value = NULL;
    node_type->as.struct_type.fields[0].c_alias = NULL;

    /* Create pointer to the struct - pointer breaks the cycle */
    Type *ptr_node_type = ast_create_pointer_type(&arena, node_type);
    node_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    node_type->as.struct_type.fields[1].type = ptr_node_type;  /* *LinkedNode - pointer breaks cycle */
    node_type->as.struct_type.fields[1].offset = 0;
    node_type->as.struct_type.fields[1].default_value = NULL;
    node_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses even on pointer-based self-references. */

    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = node_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = true;
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - pointer breaks the cycle */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_pointer_breaks_cycle");
}

/* Test: array of self (struct with field of type Foo[]) - should fail */
static void test_struct_array_of_self_circular()
{
    DEBUG_INFO("Starting test_struct_array_of_self_circular");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct with array of self - should be a circular dependency */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "TreeNode", 1, "test.sn", &arena);

    /* Manually create struct to avoid clone issues */
    Type *tree_type = arena_alloc(&arena, sizeof(Type));
    memset(tree_type, 0, sizeof(Type));
    tree_type->kind = TYPE_STRUCT;
    tree_type->as.struct_type.name = arena_strdup(&arena, "TreeNode");
    tree_type->as.struct_type.field_count = 2;
    tree_type->as.struct_type.is_native = false;
    tree_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    tree_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    tree_type->as.struct_type.fields[0].type = int_type;
    tree_type->as.struct_type.fields[0].offset = 0;
    tree_type->as.struct_type.fields[0].default_value = NULL;
    tree_type->as.struct_type.fields[0].c_alias = NULL;

    /* Create array of struct - this is still a circular dependency */
    Type *tree_array_type = ast_create_array_type(&arena, tree_type);
    tree_type->as.struct_type.fields[1].name = arena_strdup(&arena, "children");
    tree_type->as.struct_type.fields[1].type = tree_array_type;
    tree_type->as.struct_type.fields[1].offset = 0;
    tree_type->as.struct_type.fields[1].default_value = NULL;
    tree_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on self-referencing structures (even through arrays). */

    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = tree_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - array of self is still circular */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_array_of_self_circular");
}

/* Test: detect_struct_circular_dependency function directly */
static void test_circular_dependency_detection_direct()
{
    DEBUG_INFO("Starting test_circular_dependency_detection_direct");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Test 1: No circular dependency - can use ast_create_struct_type since no self-ref */
    StructField simple_fields[2];
    simple_fields[0].name = "x";
    simple_fields[0].type = int_type;
    simple_fields[0].offset = 0;
    simple_fields[0].default_value = NULL;
    simple_fields[0].c_alias = NULL;
    simple_fields[1].name = "y";
    simple_fields[1].type = int_type;
    simple_fields[1].offset = 0;
    simple_fields[1].default_value = NULL;
    simple_fields[1].c_alias = NULL;

    Type *simple_type = ast_create_struct_type(&arena, "Simple", simple_fields, 2, NULL, 0, false, false, false, NULL);

    char chain[512];
    bool has_cycle = detect_struct_circular_dependency(simple_type, NULL, chain, sizeof(chain));
    assert(!has_cycle);  /* No cycle in simple struct with primitives */

    /* Test 2: Direct circular dependency - manually create to avoid clone infinite recursion */
    Type *self_ref_type = arena_alloc(&arena, sizeof(Type));
    memset(self_ref_type, 0, sizeof(Type));
    self_ref_type->kind = TYPE_STRUCT;
    self_ref_type->as.struct_type.name = arena_strdup(&arena, "SelfRef");
    self_ref_type->as.struct_type.field_count = 2;
    self_ref_type->as.struct_type.is_native = false;
    self_ref_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    self_ref_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    self_ref_type->as.struct_type.fields[0].type = int_type;
    self_ref_type->as.struct_type.fields[0].offset = 0;
    self_ref_type->as.struct_type.fields[0].default_value = NULL;
    self_ref_type->as.struct_type.fields[0].c_alias = NULL;

    self_ref_type->as.struct_type.fields[1].name = arena_strdup(&arena, "self");
    self_ref_type->as.struct_type.fields[1].type = self_ref_type;  /* Self-reference */
    self_ref_type->as.struct_type.fields[1].offset = 0;
    self_ref_type->as.struct_type.fields[1].default_value = NULL;
    self_ref_type->as.struct_type.fields[1].c_alias = NULL;

    has_cycle = detect_struct_circular_dependency(self_ref_type, NULL, chain, sizeof(chain));
    assert(has_cycle);  /* Should detect direct cycle */
    assert(strlen(chain) > 0);  /* Chain should be populated */

    /* Test 3: Pointer should break cycle - manually create to avoid clone issues */
    Type *ptr_struct_type = arena_alloc(&arena, sizeof(Type));
    memset(ptr_struct_type, 0, sizeof(Type));
    ptr_struct_type->kind = TYPE_STRUCT;
    ptr_struct_type->as.struct_type.name = arena_strdup(&arena, "PtrNode");
    ptr_struct_type->as.struct_type.field_count = 2;
    ptr_struct_type->as.struct_type.is_native = true;
    ptr_struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    ptr_struct_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    ptr_struct_type->as.struct_type.fields[0].type = int_type;
    ptr_struct_type->as.struct_type.fields[0].offset = 0;
    ptr_struct_type->as.struct_type.fields[0].default_value = NULL;
    ptr_struct_type->as.struct_type.fields[0].c_alias = NULL;

    Type *ptr_to_self = ast_create_pointer_type(&arena, ptr_struct_type);
    ptr_struct_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    ptr_struct_type->as.struct_type.fields[1].type = ptr_to_self;  /* Pointer to self - NOT a cycle */
    ptr_struct_type->as.struct_type.fields[1].offset = 0;
    ptr_struct_type->as.struct_type.fields[1].default_value = NULL;
    ptr_struct_type->as.struct_type.fields[1].c_alias = NULL;

    has_cycle = detect_struct_circular_dependency(ptr_struct_type, NULL, chain, sizeof(chain));
    assert(!has_cycle);  /* Pointer breaks the cycle */

    arena_free(&arena);

    DEBUG_INFO("Finished test_circular_dependency_detection_direct");
}

void test_type_checker_struct_circular_main(void)
{
    TEST_SECTION("Struct Type Checker - Circular Dependencies");

    TEST_RUN("struct_direct_circular_dependency", test_struct_direct_circular_dependency);
    TEST_RUN("struct_indirect_circular_dependency", test_struct_indirect_circular_dependency);
    TEST_RUN("struct_multi_level_circular_chain", test_struct_multi_level_circular_chain);
    TEST_RUN("struct_pointer_breaks_cycle", test_struct_pointer_breaks_cycle);
    TEST_RUN("struct_array_of_self_circular", test_struct_array_of_self_circular);
    TEST_RUN("circular_dependency_detection_direct", test_circular_dependency_detection_direct);
}
