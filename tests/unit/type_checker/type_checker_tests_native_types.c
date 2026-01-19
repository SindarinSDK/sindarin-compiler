// tests/unit/type_checker/type_checker_tests_native_types.c
// Tests for native opaque types and interop primitive types
// Note: setup_test_token helper is defined in type_checker_tests_native.c

#include "../test_harness.h"

/* ==========================================================================
 * Opaque Type Tests
 * ========================================================================== */

static void test_opaque_type_declaration(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);

    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that opaque pointer type is valid in native function */
static void test_opaque_pointer_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);

    /* Register the type in the symbol table */
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* Create: native fn fclose(f: *FILE): int */
    Token fclose_tok;
    setup_test_token(&fclose_tok, TOKEN_IDENTIFIER, "fclose", 2, "test.sn", &arena);
    Token f_param_tok;
    setup_test_token(&f_param_tok, TOKEN_IDENTIFIER, "f", 2, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    params[0].name = f_param_tok;
    params[0].type = ptr_file_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *fclose_decl = ast_create_function_stmt(&arena, fclose_tok, params, 1, int_type, NULL, 0, &fclose_tok);
    fclose_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, fclose_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that dereferencing opaque pointer is rejected */
static void test_opaque_dereference_rejected(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* In a native function, try to dereference the pointer: var p: *FILE = nil; var x = p as val */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_file_type, nil_lit, NULL);

    /* Create: var x = p as val -- THIS SHOULD FAIL for opaque types */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 4, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 4, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 4, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, opaque_type, as_val_expr, NULL);

    /* Create native function body */
    Stmt *body[2] = {p_decl, x_decl};
    Token native_tok;
    setup_test_token(&native_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    Stmt *native_fn = ast_create_function_stmt(&arena, native_tok, NULL, 0, void_type, body, 2, &native_tok);
    native_fn->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, native_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - cannot dereference opaque pointer */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that opaque type is C-compatible (can be used in native callback) */
static void test_opaque_type_c_compatible(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* Create: type FileCallback = native fn(f: *FILE): void */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "FileCallback", 2, "test.sn", &arena);

    Type **param_types = arena_alloc(&arena, sizeof(Type *));
    param_types[0] = ptr_file_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *callback_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, callback_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - *FILE is C-compatible */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ==========================================================================
 * Interop Primitive Type Tests
 * ========================================================================== */

/* Test int32 type in native function */
static void test_int32_type_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_int32(): int32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_int32", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, int32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test uint type in native function */
static void test_uint_type_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_uint(): uint */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_uint", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, uint_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test uint32 type in native function */
static void test_uint32_type_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_uint32(): uint32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_uint32", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, uint32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test float type in native function */
static void test_float_type_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_float(): float */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_float", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, float_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test interop types are C-compatible in native callback */
static void test_interop_types_c_compatible(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int32, b: uint): float */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = int32_type;
    param_types[1] = uint_type;

    Type *callback_type = ast_create_function_type(&arena, float_type, param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *callback_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, callback_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - int32, uint, float are C-compatible */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test pointer to interop type */
static void test_pointer_to_interop_type(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *ptr_int32_type = ast_create_pointer_type(&arena, int32_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_int32_ptr(): *int32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_int32_ptr", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, ptr_int32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main entry point for native types tests
 * ============================================================================ */

void test_type_checker_native_types_main(void)
{
    TEST_SECTION("Native Types");

    TEST_RUN("opaque_type_declaration", test_opaque_type_declaration);
    TEST_RUN("opaque_pointer_in_native_function", test_opaque_pointer_in_native_function);
    TEST_RUN("opaque_dereference_rejected", test_opaque_dereference_rejected);
    TEST_RUN("opaque_type_c_compatible", test_opaque_type_c_compatible);
    TEST_RUN("int32_type_in_native_function", test_int32_type_in_native_function);
    TEST_RUN("uint_type_in_native_function", test_uint_type_in_native_function);
    TEST_RUN("uint32_type_in_native_function", test_uint32_type_in_native_function);
    TEST_RUN("float_type_in_native_function", test_float_type_in_native_function);
    TEST_RUN("interop_types_c_compatible", test_interop_types_c_compatible);
    TEST_RUN("pointer_to_interop_type", test_pointer_to_interop_type);
}
