// tests/type_checker_tests_memory.c
// Type checker tests for memory management features (as val, as ref, shared, private)

static void test_type_check_var_as_ref_primitive()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_REF;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_var_as_ref_array_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, arr_init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_REF;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_var_as_val_array()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Expr *elements[1] = {e1};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{1}", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, arr_init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_VAL;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_private_function_primitive_return()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 42};
    Expr *ret_val = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_PRIVATE;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_private_function_array_return_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *ret_val = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, arr_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_PRIVATE;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_shared_function()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *ret_val = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "helper", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, arr_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_SHARED;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1); // Shared functions can return any type

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_param_as_ref_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);  // 'as ref' on arrays is invalid (they are already references)
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = arr_type;
    params[0].mem_qualifier = MEM_AS_REF;  // Invalid for array parameters - arrays are already references

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_param_as_ref_primitive()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "counter", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_AS_REF;  // Valid for primitive parameters - enables pass-by-reference

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "increment", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - 'as ref' is valid on primitives

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_param_as_val()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = arr_type;
    params[0].mem_qualifier = MEM_AS_VAL;  // Copy semantics for array param

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_null_stmt_handling()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create a function with a NULL statement in the body (edge case)
    // This tests that type_check_stmt handles null properly
    Stmt *body[2];

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    body[0] = ast_create_return_stmt(&arena, ret_tok, NULL, &ret_tok);
    body[1] = NULL;  // Null statement in body

    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_null", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    // Should not crash even with null statement in body
    int no_error = type_check_module(&module, &table);
    // No assertion on result, just checking it doesn't crash
    (void)no_error;

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_function_with_null_param_type()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create a function with a parameter that has NULL type (edge case)
    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = NULL;  // NULL type - edge case
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_null_param", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    // Should not crash even with null param type
    int no_error = type_check_module(&module, &table);
    // No assertion on result, just checking it doesn't crash
    (void)no_error;

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// ============================================================================
// MemoryContext scope_depth tracking tests
// ============================================================================

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

// ============================================================================
// Escape analysis tests for variable assignments
// ============================================================================

static void test_escape_detect_inner_to_outer_assignment()
{
    // Test: outer = inner (inner declared in deeper scope than outer)
    // Expected: inner expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 0     // scope depth 1 (in function)
    //   {
    //     var inner: int = 42  // scope depth 2 (in block)
    //     outer = inner        // inner escapes to outer scope
    //   }

    // Statement 1: var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Statement 2: Block containing inner decl and assignment
    // var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 2, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner (assignment expression)
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 3, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create block with inner_decl and assign_stmt
    Stmt *block_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *block_stmt = ast_create_block_stmt(&arena, block_stmts, 2, &block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, block_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that inner_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_same_scope_no_escape()
{
    // Test: a = b (both declared in same scope)
    // Expected: b expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var a: int = 0
    //   var b: int = 42
    //   a = b  // same scope, no escape

    // Statement 1: var a: int = 0
    Token a_name_tok;
    setup_token(&a_name_tok, TOKEN_IDENTIFIER, "a", 1, "test.sn", &arena);
    Token a_init_tok;
    setup_literal_token(&a_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue a_v = {.int_value = 0};
    Expr *a_init = ast_create_literal_expr(&arena, a_v, int_type, false, &a_init_tok);
    Stmt *a_decl = ast_create_var_decl_stmt(&arena, a_name_tok, int_type, a_init, NULL);

    // Statement 2: var b: int = 42
    Token b_name_tok;
    setup_token(&b_name_tok, TOKEN_IDENTIFIER, "b", 2, "test.sn", &arena);
    Token b_init_tok;
    setup_literal_token(&b_init_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue b_v = {.int_value = 42};
    Expr *b_init = ast_create_literal_expr(&arena, b_v, int_type, false, &b_init_tok);
    Stmt *b_decl = ast_create_var_decl_stmt(&arena, b_name_tok, int_type, b_init, NULL);

    // Statement 3: a = b
    Token assign_a_tok;
    setup_token(&assign_a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    Token b_var_tok;
    setup_token(&b_var_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    Expr *b_var_expr = ast_create_variable_expr(&arena, b_var_tok, &b_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_a_tok, b_var_expr, &assign_a_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_a_tok);

    // Create main function body
    Stmt *main_body[3] = {a_decl, b_decl, assign_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 3, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that b_var_expr does NOT have escapes_scope set
    assert(ast_expr_escapes_scope(b_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_outer_to_inner_no_escape()
{
    // Test: inner = outer (assigning from outer scope to inner)
    // Expected: outer expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 42    // scope depth 1
    //   {
    //     var inner: int = 0   // scope depth 2
    //     inner = outer        // outer does NOT escape (it's already in outer scope)
    //   }

    // Statement 1: var outer: int = 42
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 42};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Block: var inner: int = 0; inner = outer
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 2, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 0};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // inner = outer
    Token assign_inner_tok;
    setup_token(&assign_inner_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token outer_var_tok;
    setup_token(&outer_var_tok, TOKEN_IDENTIFIER, "outer", 3, "test.sn", &arena);
    Expr *outer_var_expr = ast_create_variable_expr(&arena, outer_var_tok, &outer_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_inner_tok, outer_var_expr, &assign_inner_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_inner_tok);

    // Create block
    Stmt *block_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *block_stmt = ast_create_block_stmt(&arena, block_stmts, 2, &block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, block_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that outer_var_expr does NOT have escapes_scope set (it's from outer scope)
    assert(ast_expr_escapes_scope(outer_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_detect_nested_blocks()
{
    // Test: outer = deeply_inner (from multiple nesting levels)
    // Expected: deeply_inner expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 0     // scope depth 1
    //   {
    //     {
    //       var deep: int = 42 // scope depth 3
    //       outer = deep       // deep escapes to outer scope (3 > 1)
    //     }
    //   }

    // Statement 1: var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Inner block: var deep: int = 42; outer = deep
    Token deep_name_tok;
    setup_token(&deep_name_tok, TOKEN_IDENTIFIER, "deep", 3, "test.sn", &arena);
    Token deep_init_tok;
    setup_literal_token(&deep_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue deep_v = {.int_value = 42};
    Expr *deep_init = ast_create_literal_expr(&arena, deep_v, int_type, false, &deep_init_tok);
    Stmt *deep_decl = ast_create_var_decl_stmt(&arena, deep_name_tok, int_type, deep_init, NULL);

    // outer = deep
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token deep_var_tok;
    setup_token(&deep_var_tok, TOKEN_IDENTIFIER, "deep", 4, "test.sn", &arena);
    Expr *deep_var_expr = ast_create_variable_expr(&arena, deep_var_tok, &deep_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, deep_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create inner block
    Stmt *inner_block_stmts[2] = {deep_decl, assign_stmt};
    Token inner_block_tok;
    setup_token(&inner_block_tok, TOKEN_LEFT_BRACE, "{", 3, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_block_stmts, 2, &inner_block_tok);

    // Create outer block
    Stmt *outer_block_stmts[1] = {inner_block};
    Token outer_block_tok;
    setup_token(&outer_block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *outer_block = ast_create_block_stmt(&arena, outer_block_stmts, 1, &outer_block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, outer_block};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that deep_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(deep_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// ============================================================================
// Escape analysis tests for return statements
// ============================================================================

static void test_escape_return_local_variable()
{
    // Test: function returns local variable
    // Expected: return expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn getValue(): int =>
    //   var local: int = 42
    //   return local  // local escapes via return

    // var local: int = 42
    Token local_name_tok;
    setup_token(&local_name_tok, TOKEN_IDENTIFIER, "local", 1, "test.sn", &arena);
    Token local_init_tok;
    setup_literal_token(&local_init_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue local_v = {.int_value = 42};
    Expr *local_init = ast_create_literal_expr(&arena, local_v, int_type, false, &local_init_tok);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_name_tok, int_type, local_init, NULL);

    // return local
    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token local_var_tok;
    setup_token(&local_var_tok, TOKEN_IDENTIFIER, "local", 2, "test.sn", &arena);
    Expr *local_var_expr = ast_create_variable_expr(&arena, local_var_tok, &local_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, local_var_expr, &return_tok);

    // Create function body
    Stmt *func_body[2] = {local_decl, return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getValue", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 2, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that local_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(local_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_parameter_no_escape()
{
    // Test: function returns parameter variable
    // Expected: return expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn identity(x: int): int =>
    //   return x  // parameter, no escape

    // Create parameter
    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    // return x
    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token x_var_tok;
    setup_token(&x_var_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Expr *x_var_expr = ast_create_variable_expr(&arena, x_var_tok, &x_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, x_var_expr, &return_tok);

    // Create function body
    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "identity", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, params, 1, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that x_var_expr does NOT have escapes_scope set (it's a parameter)
    assert(ast_expr_escapes_scope(x_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_global_no_escape()
{
    // Test: function returns global variable
    // Expected: return expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create global:
    // var global: int = 100

    Token global_name_tok;
    setup_token(&global_name_tok, TOKEN_IDENTIFIER, "globalVal", 1, "test.sn", &arena);
    Token global_init_tok;
    setup_literal_token(&global_init_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue global_v = {.int_value = 100};
    Expr *global_init = ast_create_literal_expr(&arena, global_v, int_type, false, &global_init_tok);
    Stmt *global_decl = ast_create_var_decl_stmt(&arena, global_name_tok, int_type, global_init, NULL);

    // Create function:
    // fn getGlobal(): int =>
    //   return globalVal  // global, no escape

    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 3, "test.sn", &arena);
    Token global_var_tok;
    setup_token(&global_var_tok, TOKEN_IDENTIFIER, "globalVal", 3, "test.sn", &arena);
    Expr *global_var_expr = ast_create_variable_expr(&arena, global_var_tok, &global_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, global_var_expr, &return_tok);

    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getGlobal", 2, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, global_decl);
    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that global_var_expr does NOT have escapes_scope set (it's a global)
    assert(ast_expr_escapes_scope(global_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// ============================================================================
// Escape analysis tests for struct assignments in control structures
// ============================================================================

static void test_escape_struct_assign_in_if_block()
{
    // Test: struct assignment from inner if-block scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     if true =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Condition: true
    Token cond_tok;
    setup_literal_token(&cond_tok, TOKEN_BOOL_LITERAL, "true", 2, "test.sn", &arena);
    LiteralValue cond_v = {.bool_value = true};
    Expr *cond_expr = ast_create_literal_expr(&arena, cond_v, bool_type, false, &cond_tok);

    // Inside if block: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create then-block
    Stmt *then_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *then_block = ast_create_block_stmt(&arena, then_stmts, 2, &block_tok);

    // Create if statement
    Token if_tok;
    setup_token(&if_tok, TOKEN_IF, "if", 2, "test.sn", &arena);
    Stmt *if_stmt = ast_create_if_stmt(&arena, cond_expr, then_block, NULL, &if_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, if_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_struct_assign_in_while_loop()
{
    // Test: struct assignment from inner while-loop scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     while false =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Condition: false (so loop body isn't executed at runtime, but still type-checked)
    Token cond_tok;
    setup_literal_token(&cond_tok, TOKEN_BOOL_LITERAL, "false", 2, "test.sn", &arena);
    LiteralValue cond_v = {.bool_value = false};
    Expr *cond_expr = ast_create_literal_expr(&arena, cond_v, bool_type, false, &cond_tok);

    // Inside while loop: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create loop body block
    Stmt *body_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *body_block = ast_create_block_stmt(&arena, body_stmts, 2, &block_tok);

    // Create while statement
    Token while_tok;
    setup_token(&while_tok, TOKEN_WHILE, "while", 2, "test.sn", &arena);
    Stmt *while_stmt = ast_create_while_stmt(&arena, cond_expr, body_block, &while_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, while_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_struct_assign_in_for_loop()
{
    // Test: struct assignment from inner for-loop scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     for var i: int = 0; i < 10; i = i + 1 =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // For initializer: var i: int = 0
    Token i_name_tok;
    setup_token(&i_name_tok, TOKEN_IDENTIFIER, "i", 2, "test.sn", &arena);
    Token i_init_tok;
    setup_literal_token(&i_init_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue i_v = {.int_value = 0};
    Expr *i_init = ast_create_literal_expr(&arena, i_v, int_type, false, &i_init_tok);
    Stmt *for_init = ast_create_var_decl_stmt(&arena, i_name_tok, int_type, i_init, NULL);

    // For condition: i < 10
    Token cond_tok;
    setup_token(&cond_tok, TOKEN_LESS, "<", 2, "test.sn", &arena);
    Token i_tok;
    setup_token(&i_tok, TOKEN_IDENTIFIER, "i", 2, "test.sn", &arena);
    Expr *i_var = ast_create_variable_expr(&arena, i_tok, &i_tok);
    Token ten_tok;
    setup_literal_token(&ten_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue ten_v = {.int_value = 10};
    Expr *ten_lit = ast_create_literal_expr(&arena, ten_v, int_type, false, &ten_tok);
    Expr *for_cond = ast_create_binary_expr(&arena, i_var, TOKEN_LESS, ten_lit, &cond_tok);

    // For increment: i = i + 1
    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 2, "test.sn", &arena);
    Expr *i_var2 = ast_create_variable_expr(&arena, i_tok, &i_tok);
    Token one_tok;
    setup_literal_token(&one_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue one_v = {.int_value = 1};
    Expr *one_lit = ast_create_literal_expr(&arena, one_v, int_type, false, &one_tok);
    Expr *i_plus_1 = ast_create_binary_expr(&arena, i_var2, TOKEN_PLUS, one_lit, &plus_tok);
    Expr *for_incr = ast_create_assign_expr(&arena, i_tok, i_plus_1, &i_tok);

    // Inside for loop: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create loop body block
    Stmt *body_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *body_block = ast_create_block_stmt(&arena, body_stmts, 2, &block_tok);

    // Create for statement
    Token for_tok;
    setup_token(&for_tok, TOKEN_FOR, "for", 2, "test.sn", &arena);
    Stmt *for_stmt = ast_create_for_stmt(&arena, for_init, for_cond, for_incr, body_block, &for_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, for_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_literal_no_escape()
{
    // Test: function returns literal expression
    // Expected: return expression should NOT have escapes_scope (not a variable)
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn getConstant(): int =>
    //   return 42  // literal, not a variable

    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue lit_v = {.int_value = 42};
    Expr *lit_expr = ast_create_literal_expr(&arena, lit_v, int_type, false, &lit_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, lit_expr, &return_tok);

    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getConstant", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that lit_expr does NOT have escapes_scope set (it's a literal)
    assert(ast_expr_escapes_scope(lit_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_memory_main()
{
    TEST_SECTION("Type Checker Memory");

    TEST_RUN("var_as_ref_primitive", test_type_check_var_as_ref_primitive);
    TEST_RUN("var_as_ref_array_error", test_type_check_var_as_ref_array_error);
    TEST_RUN("var_as_val_array", test_type_check_var_as_val_array);
    TEST_RUN("private_function_primitive_return", test_type_check_private_function_primitive_return);
    TEST_RUN("private_function_array_return_error", test_type_check_private_function_array_return_error);
    TEST_RUN("shared_function", test_type_check_shared_function);
    TEST_RUN("param_as_ref_error", test_type_check_param_as_ref_error);
    TEST_RUN("param_as_ref_primitive", test_type_check_param_as_ref_primitive);
    TEST_RUN("param_as_val", test_type_check_param_as_val);
    TEST_RUN("null_stmt_handling", test_type_check_null_stmt_handling);
    TEST_RUN("function_with_null_param_type", test_type_check_function_with_null_param_type);
    TEST_RUN("memory_context_scope_depth_init", test_memory_context_scope_depth_init);
    TEST_RUN("memory_context_scope_depth_enter_exit", test_memory_context_scope_depth_enter_exit);
    TEST_RUN("memory_context_scope_depth_bounds", test_memory_context_scope_depth_bounds);
    TEST_RUN("memory_context_scope_depth_null", test_memory_context_scope_depth_null);
    TEST_RUN("memory_context_scope_depth_nested_deep", test_memory_context_scope_depth_nested_deep);
    TEST_RUN("memory_context_scope_with_private", test_memory_context_scope_with_private);

    // Escape analysis tests - variable assignments
    TEST_RUN("escape_detect_inner_to_outer_assignment", test_escape_detect_inner_to_outer_assignment);
    TEST_RUN("escape_same_scope_no_escape", test_escape_same_scope_no_escape);
    TEST_RUN("escape_outer_to_inner_no_escape", test_escape_outer_to_inner_no_escape);
    TEST_RUN("escape_detect_nested_blocks", test_escape_detect_nested_blocks);

    // Escape analysis tests - control structures
    TEST_RUN("escape_struct_assign_in_if_block", test_escape_struct_assign_in_if_block);
    TEST_RUN("escape_struct_assign_in_while_loop", test_escape_struct_assign_in_while_loop);
    TEST_RUN("escape_struct_assign_in_for_loop", test_escape_struct_assign_in_for_loop);

    // Escape analysis tests - return statements
    TEST_RUN("escape_return_local_variable", test_escape_return_local_variable);
    TEST_RUN("escape_return_parameter_no_escape", test_escape_return_parameter_no_escape);
    TEST_RUN("escape_return_global_no_escape", test_escape_return_global_no_escape);
    TEST_RUN("escape_return_literal_no_escape", test_escape_return_literal_no_escape);
}
