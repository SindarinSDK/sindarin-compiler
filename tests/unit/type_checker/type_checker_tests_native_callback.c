// tests/unit/type_checker/type_checker_tests_native_callback.c
// Tests for native callback types, variadic functions, and lambda expressions
// Note: setup_test_token helper is defined in type_checker_tests_native.c

#include "../test_harness.h"

static void test_variadic_function_accepts_extra_args(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create module */
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn printf(format: str, ...): int */
    Token printf_tok;
    setup_test_token(&printf_tok, TOKEN_IDENTIFIER, "printf", 1, "test.sn", &arena);

    Token format_tok;
    setup_test_token(&format_tok, TOKEN_IDENTIFIER, "format", 1, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = format_tok;
    params[0].type = str_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *printf_decl = ast_create_function_stmt(&arena, printf_tok, params, 1, int_type, NULL, 0, &printf_tok);
    printf_decl->as.function.is_native = true;
    printf_decl->as.function.is_variadic = true;

    /* Create main function that calls printf with extra args */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* printf("Hello %d", 42) */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "printf", 3, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Token str_tok;
    setup_test_token(&str_tok, TOKEN_STRING_LITERAL, "Hello %d", 3, "test.sn", &arena);
    LiteralValue str_val = {.string_value = "Hello %d"};
    Expr *format_lit = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);

    Token int_tok;
    setup_test_token(&int_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 42};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    Expr **args = arena_alloc(&arena, sizeof(Expr *) * 2);
    args[0] = format_lit;
    args[1] = int_lit;
    Expr *call = ast_create_call_expr(&arena, callee, args, 2, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    body[0] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 1, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, printf_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - variadic accepts extra args */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_variadic_function_rejects_too_few_args(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create module */
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn printf(format: str, ...): int */
    Token printf_tok;
    setup_test_token(&printf_tok, TOKEN_IDENTIFIER, "printf", 1, "test.sn", &arena);

    Token format_tok;
    setup_test_token(&format_tok, TOKEN_IDENTIFIER, "format", 1, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = format_tok;
    params[0].type = str_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *printf_decl = ast_create_function_stmt(&arena, printf_tok, params, 1, int_type, NULL, 0, &printf_tok);
    printf_decl->as.function.is_native = true;
    printf_decl->as.function.is_variadic = true;

    /* Create main function that calls printf with NO args (missing format) */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* printf() - missing required format param */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "printf", 3, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Expr *call = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    body[0] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 1, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, printf_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - missing required param */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_native_callback_type_alias_c_compatible(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    /* Build parameter types: *void, *void */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = ptr_void_type;
    param_types[1] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, callback_type, &comparator_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should succeed - all types are C-compatible */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type alias with array parameter fails */
static void test_native_callback_type_alias_array_param_fails(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: type BadCallback = native fn(arr: int[]): void */
    Token badcb_tok;
    setup_test_token(&badcb_tok, TOKEN_IDENTIFIER, "BadCallback", 1, "test.sn", &arena);

    /* Build parameter types: int[] (array - NOT C-compatible) */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 1);
    param_types[0] = int_array_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, badcb_tok, callback_type, &badcb_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - array type is not C-compatible */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type alias with array return fails */
static void test_native_callback_type_alias_array_return_fails(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: type BadCallback = native fn(): int[] */
    Token badcb_tok;
    setup_test_token(&badcb_tok, TOKEN_IDENTIFIER, "BadCallback", 1, "test.sn", &arena);

    /* No params, return type is int[] (array - NOT C-compatible) */
    Type *callback_type = ast_create_function_type(&arena, int_array_type, NULL, 0);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, badcb_tok, callback_type, &badcb_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - array return type is not C-compatible */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type can be used as parameter in native function */
static void test_native_callback_as_function_param(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = ptr_void_type;
    cb_param_types[1] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, callback_type, &comparator_tok);

    /* Register the type in symbol table (simulating what parser does) */
    symbol_table_add_type(&table, comparator_tok, callback_type);

    /* Create: native fn qsort(base: *void, count: int, size: int, cmp: Comparator): void */
    Token qsort_tok;
    setup_test_token(&qsort_tok, TOKEN_IDENTIFIER, "qsort", 3, "test.sn", &arena);

    Parameter *qsort_params = arena_alloc(&arena, sizeof(Parameter) * 4);
    Token base_tok; setup_test_token(&base_tok, TOKEN_IDENTIFIER, "base", 3, "test.sn", &arena);
    Token count_tok; setup_test_token(&count_tok, TOKEN_IDENTIFIER, "count", 3, "test.sn", &arena);
    Token size_tok; setup_test_token(&size_tok, TOKEN_IDENTIFIER, "size", 3, "test.sn", &arena);
    Token cmp_tok; setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);

    qsort_params[0].name = base_tok;
    qsort_params[0].type = ptr_void_type;
    qsort_params[0].mem_qualifier = MEM_DEFAULT;
    qsort_params[1].name = count_tok;
    qsort_params[1].type = int_type;
    qsort_params[1].mem_qualifier = MEM_DEFAULT;
    qsort_params[2].name = size_tok;
    qsort_params[2].type = int_type;
    qsort_params[2].mem_qualifier = MEM_DEFAULT;
    qsort_params[3].name = cmp_tok;
    qsort_params[3].type = callback_type;  /* Callback type as parameter! */
    qsort_params[3].mem_qualifier = MEM_DEFAULT;

    Stmt *qsort_decl = ast_create_function_stmt(&arena, qsort_tok, qsort_params, 4, void_type, NULL, 0, &qsort_tok);
    qsort_decl->as.function.is_native = true;

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, qsort_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should succeed - callback type can be used as param */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda capturing a variable from enclosing scope produces an error */
static void test_native_lambda_capture_rejected(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Callback = native fn(data: *void): void */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *));
    cb_param_types[0] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, cb_param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create:
     * native fn setup(): void =>
     *     var counter: int = 0
     *     var handler: Callback = fn(data: *void): void =>
     *         counter = counter + 1  // ERROR: capture
     */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);

    /* Create: var counter: int = 0 */
    Token counter_tok;
    setup_test_token(&counter_tok, TOKEN_IDENTIFIER, "counter", 3, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 3, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *counter_decl = ast_create_var_decl_stmt(&arena, counter_tok, int_type, zero_lit, NULL);

    /* Create the native lambda body: counter = counter + 1
     * This references 'counter' from enclosing scope = capture! */
    Token counter_ref_tok;
    setup_test_token(&counter_ref_tok, TOKEN_IDENTIFIER, "counter", 5, "test.sn", &arena);
    Expr *counter_ref = ast_create_variable_expr(&arena, counter_ref_tok, &counter_ref_tok);
    Token one_tok;
    setup_test_token(&one_tok, TOKEN_INT_LITERAL, "1", 5, "test.sn", &arena);
    LiteralValue one_val = {.int_value = 1};
    Expr *one_lit = ast_create_literal_expr(&arena, one_val, int_type, false, &one_tok);
    Expr *add_expr = ast_create_binary_expr(&arena, counter_ref, TOKEN_PLUS, one_lit, &counter_ref_tok);

    Token assign_tok;
    setup_test_token(&assign_tok, TOKEN_IDENTIFIER, "counter", 5, "test.sn", &arena);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_tok, add_expr, &assign_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_tok);

    /* Create native lambda with statement body */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter));
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 4, "test.sn", &arena);
    lambda_params[0].name = data_tok;
    lambda_params[0].type = ptr_void_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 4, "test.sn", &arena);

    Stmt **lambda_body_stmts = arena_alloc(&arena, sizeof(Stmt *));
    lambda_body_stmts[0] = assign_stmt;

    Expr *native_lambda = ast_create_lambda_stmt_expr(&arena, lambda_params, 1,
                                                       void_type, lambda_body_stmts, 1,
                                                       FUNC_DEFAULT, true, &fn_tok);  /* is_native = true */

    /* Create: var handler: Callback = <lambda> */
    Token handler_tok;
    setup_test_token(&handler_tok, TOKEN_IDENTIFIER, "handler", 4, "test.sn", &arena);
    Stmt *handler_decl = ast_create_var_decl_stmt(&arena, handler_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    setup_body[0] = counter_decl;
    setup_body[1] = handler_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 2, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - native lambda captures 'counter' */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda using only its own parameters succeeds */
static void test_native_lambda_params_only_succeeds(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    Type **cmp_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cmp_param_types[0] = ptr_void_type;
    cmp_param_types[1] = ptr_void_type;

    Type *comparator_type = ast_create_function_type(&arena, int_type, cmp_param_types, 2);
    comparator_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, comparator_type, &comparator_tok);
    symbol_table_add_type(&table, comparator_tok, comparator_type);

    /* Create:
     * native fn setup(): void =>
     *     var cmp: Comparator = fn(a: *void, b: *void): int =>
     *         return 0  // Only uses parameters and literals - OK!
     */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);

    /* Create the native lambda body: return 0 */
    Token return_tok;
    setup_test_token(&return_tok, TOKEN_RETURN, "return", 4, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 4, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, zero_lit, &return_tok);

    /* Create native lambda with parameters a and b */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter) * 2);
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = ptr_void_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;
    Token b_tok;
    setup_test_token(&b_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    lambda_params[1].name = b_tok;
    lambda_params[1].type = ptr_void_type;
    lambda_params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    Stmt **lambda_body_stmts = arena_alloc(&arena, sizeof(Stmt *));
    lambda_body_stmts[0] = return_stmt;

    Expr *native_lambda = ast_create_lambda_stmt_expr(&arena, lambda_params, 2,
                                                       int_type, lambda_body_stmts, 1,
                                                       FUNC_DEFAULT, true, &fn_tok);  /* is_native = true */

    /* Create: var cmp: Comparator = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, comparator_type, native_lambda, NULL);

    /* Create setup function body */
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED - lambda only uses its own parameters */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda with mismatched parameter count produces error */
static void test_native_lambda_param_count_mismatch(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int, b: int): int */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = int_type;
    cb_param_types[1] = int_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create a native lambda with wrong param count: fn(a: int): int => a
     * This has 1 param but callback expects 2 */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter));
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = int_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    /* Body: just return a */
    Expr *a_ref = ast_create_variable_expr(&arena, a_tok, &a_tok);

    Expr *native_lambda = ast_create_lambda_expr(&arena, lambda_params, 1,
                                                  int_type, a_ref, FUNC_DEFAULT,
                                                  false, &fn_tok);  /* is_native will be inferred */

    /* Create: var cmp: Callback = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - parameter count mismatch */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_native_lambda_matching_signature(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int, b: int): int */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = int_type;
    cb_param_types[1] = int_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create a native lambda with matching signature: fn(a: int, b: int): int => a + b */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter) * 2);
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = int_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;
    Token b_tok;
    setup_test_token(&b_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    lambda_params[1].name = b_tok;
    lambda_params[1].type = int_type;
    lambda_params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    /* Body: a + b */
    Expr *a_ref = ast_create_variable_expr(&arena, a_tok, &a_tok);
    Expr *b_ref = ast_create_variable_expr(&arena, b_tok, &b_tok);
    Expr *add_expr = ast_create_binary_expr(&arena, a_ref, TOKEN_PLUS, b_ref, &a_tok);

    Expr *native_lambda = ast_create_lambda_expr(&arena, lambda_params, 2,
                                                  int_type, add_expr, FUNC_DEFAULT,
                                                  false, &fn_tok);  /* is_native will be inferred */

    /* Create: var cmp: Callback = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED - matching signature */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main entry point for callback tests
 * ============================================================================ */

void test_type_checker_native_callback_main(void)
{
    TEST_SECTION("Native Callback");

    TEST_RUN("variadic_function_accepts_extra_args", test_variadic_function_accepts_extra_args);
    TEST_RUN("variadic_function_rejects_too_few_args", test_variadic_function_rejects_too_few_args);
    TEST_RUN("native_callback_type_alias_c_compatible", test_native_callback_type_alias_c_compatible);
    TEST_RUN("native_callback_type_alias_array_param_fails", test_native_callback_type_alias_array_param_fails);
    TEST_RUN("native_callback_type_alias_array_return_fails", test_native_callback_type_alias_array_return_fails);
    TEST_RUN("native_callback_as_function_param", test_native_callback_as_function_param);
    TEST_RUN("native_lambda_capture_rejected", test_native_lambda_capture_rejected);
    TEST_RUN("native_lambda_params_only_succeeds", test_native_lambda_params_only_succeeds);
    TEST_RUN("native_lambda_param_count_mismatch", test_native_lambda_param_count_mismatch);
    TEST_RUN("native_lambda_matching_signature", test_native_lambda_matching_signature);
}
