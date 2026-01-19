// tests/unit/type_checker/type_checker_tests_native_pointer.c
// Tests for native function pointer variable handling, as val, and as ref
// Note: setup_test_token helper is defined in type_checker_tests_native.c

/* Suppress warning for operator name arrays used for readability but not in assertions */
#pragma clang diagnostic ignored "-Wunused-variable"

#include "../test_harness.h"

/* Test that pointer variables are REJECTED in regular (non-native) functions */
static void test_pointer_var_rejected_in_regular_function(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a REGULAR function (not native) */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "regular_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* Regular function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - pointer vars not allowed in regular functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer variables are ACCEPTED in native functions */
static void test_pointer_var_accepted_in_native_function(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a NATIVE function */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "native_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = true;  /* Native function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should PASS - pointer vars allowed in native functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test helper: create a binary expression with pointer and int */
static Stmt *create_pointer_arithmetic_stmt(Arena *arena, Type *ptr_type, Type *int_type, SnTokenType op)
{
    /* Create a pointer variable reference */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", arena);
    Expr *p_ref = ast_create_variable_expr(arena, p_tok, &p_tok);
    p_ref->expr_type = ptr_type;

    /* Create int literal 1 */
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", arena);
    LiteralValue val = {.int_value = 1};
    Expr *lit = ast_create_literal_expr(arena, val, int_type, false, &lit_tok);

    /* Create binary expression: p + 1 */
    Token op_tok;
    setup_test_token(&op_tok, TOKEN_PLUS, "+", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p_ref, op, lit, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer arithmetic is REJECTED for all operators (+, -, *, /, %) */
static void test_pointer_arithmetic_rejected(void)
{
    /* Test each arithmetic operator */
    SnTokenType operators[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO};
    const char *op_names[] = {"+", "-", "*", "/", "%%"};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p: *int = nil */
        Token p_tok;
        setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p + 1 (or p - 1, etc.) */
        Stmt *arith_stmt = create_pointer_arithmetic_stmt(&arena, ptr_int_type, int_type, operators[i]);

        /* Wrap in a native function (to allow pointer var declaration) */
        Stmt *body[2] = {p_decl, arith_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 0);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

/* Test helper: create a comparison expression with two pointers */
static Stmt *create_pointer_comparison_stmt(Arena *arena, Type *ptr_type, SnTokenType op, bool use_nil_as_right)
{
    /* Create a pointer variable reference */
    Token p1_tok;
    setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", arena);
    Expr *p1_ref = ast_create_variable_expr(arena, p1_tok, &p1_tok);
    p1_ref->expr_type = ptr_type;

    Expr *right_operand;
    if (use_nil_as_right)
    {
        /* Create nil literal */
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", arena);
        Type *nil_type = ast_create_primitive_type(arena, TYPE_NIL);
        LiteralValue nil_val = {.int_value = 0};
        right_operand = ast_create_literal_expr(arena, nil_val, nil_type, false, &nil_tok);
    }
    else
    {
        /* Create second pointer variable reference */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", arena);
        right_operand = ast_create_variable_expr(arena, p2_tok, &p2_tok);
        right_operand->expr_type = ptr_type;
    }

    /* Create binary expression: p1 == p2 or p1 == nil */
    Token op_tok;
    setup_test_token(&op_tok, op == TOKEN_EQUAL_EQUAL ? TOKEN_EQUAL_EQUAL : TOKEN_BANG_EQUAL,
                     op == TOKEN_EQUAL_EQUAL ? "==" : "!=", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p1_ref, op, right_operand, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer equality (==, !=) with nil is ALLOWED */
static void test_pointer_nil_comparison_allowed(void)
{
    SnTokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p1 == nil or p1 != nil */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], true);

        /* Wrap in a native function */
        Stmt *body[2] = {p1_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 1);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

/* Test that pointer-to-pointer equality (==, !=) is ALLOWED */
static void test_pointer_pointer_comparison_allowed(void)
{
    SnTokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok1;
        setup_test_token(&nil_tok1, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit1 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok1);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit1, NULL);

        /* Create: var p2: *int = nil */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", &arena);
        Token nil_tok2;
        setup_test_token(&nil_tok2, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        Expr *nil_lit2 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok2);
        Stmt *p2_decl = ast_create_var_decl_stmt(&arena, p2_tok, ptr_int_type, nil_lit2, NULL);

        /* Create: p1 == p2 or p1 != p2 */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], false);

        /* Wrap in a native function */
        Stmt *body[3] = {p1_decl, p2_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 3, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 1);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

/* Test that inline pointer passing (e.g., use_ptr(get_ptr())) is allowed */
static void test_inline_pointer_passing_allowed(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 2, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 2, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create call: get_ptr() */
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);

    /* Create call: use_ptr(get_ptr()) - inline pointer passing */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {get_ptr_call};
    Expr *inline_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, inline_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;  /* Regular function doing inline call */

    /* Add all to module */
    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test inline pointer passing with nil is allowed */
static void test_inline_nil_passing_allowed(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 1, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 1, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create nil literal */
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 5, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    /* Create call: use_ptr(nil) */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {nil_lit};
    Expr *nil_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, nil_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    /* Add all to module */
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *int to int */
static void test_as_val_unwraps_pointer_int(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create: var x: int = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int as val => int */

    /* Verify the expression type is int */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *double to double */
static void test_as_val_unwraps_pointer_double(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_double_type = ast_create_pointer_type(&arena, double_type);

    /* Create: var p: *double = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_double_type, nil_lit, NULL);

    /* Create: var x: double = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, double_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *double as val => double */

    /* Verify the expression type is double */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' rejects non-pointer operand (int as val should error) */
static void test_as_val_rejects_non_pointer(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: var n: int = 42 */
    Token n_tok;
    setup_test_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, lit, NULL);

    /* Create: var x: int = n as val (THIS SHOULD FAIL - n is int, not *int) */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token n_ref_tok;
    setup_test_token(&n_ref_tok, TOKEN_IDENTIFIER, "n", 2, "test.sn", &arena);
    Expr *n_ref = ast_create_variable_expr(&arena, n_ref_tok, &n_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, n_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {n_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: int as val is not allowed */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *float to float */
static void test_as_val_unwraps_pointer_float(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_float_type = ast_create_pointer_type(&arena, float_type);

    /* Create: var p: *float = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_float_type, nil_lit, NULL);

    /* Create: var x: float = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, float_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *float as val => float */

    /* Verify the expression type is float */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_FLOAT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *char as val converts to str (null-terminated string) */
static void test_as_val_char_pointer_to_str(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_char_type = ast_create_pointer_type(&arena, char_type);

    /* Create: var p: *char = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_char_type, nil_lit, NULL);

    /* Create: var s: str = p as val */
    Token s_tok;
    setup_test_token(&s_tok, TOKEN_IDENTIFIER, "s", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Stmt *s_decl = ast_create_var_decl_stmt(&arena, s_tok, str_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, s_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *char as val => str */

    /* Verify the expression type is str */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_STRING);

    /* Verify the metadata flag is set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *int as val does NOT set is_cstr_to_str flag */
static void test_as_val_int_pointer_no_cstr_flag(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create: var x: int = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int as val => int */

    /* Verify the expression type is int */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_INT);

    /* Verify the metadata flag is NOT set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer return from native fn WITHOUT 'as val' fails in regular function */
static void test_pointer_return_without_as_val_fails_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() -- missing 'as val', should fail */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, get_ptr_call, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: pointer return without as val in regular function */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer return from native fn WITH 'as val' succeeds in regular function */
static void test_pointer_return_with_as_val_succeeds_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() as val -- with 'as val', should succeed */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 5, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, get_ptr_call, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: pointer return with as val in regular function */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native functions can store pointer return values without 'as val' */
static void test_native_fn_can_store_pointer_return(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var p: *int = get_ptr() -- allowed in native function */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, get_ptr_call, NULL);

    /* Wrap in native function */
    Stmt *native_body[1] = {p_decl};
    Token native_tok;
    setup_test_token(&native_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Stmt *native_func = ast_create_function_stmt(&arena, native_tok, NULL, 0, void_type, native_body, 1, &native_tok);
    native_func->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, native_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: native function can store pointer returns */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ==========================================================================
 * Native function 'as ref' out-parameter tests
 * ========================================================================== */

/* Test that 'as ref' parameter on primitive types in native functions is valid */
static void test_as_ref_primitive_param_in_native_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_dimensions(width: int as ref, height: int as ref): void */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "get_dimensions", 1, "test.sn", &arena);

    Token width_tok;
    setup_test_token(&width_tok, TOKEN_IDENTIFIER, "width", 1, "test.sn", &arena);

    Token height_tok;
    setup_test_token(&height_tok, TOKEN_IDENTIFIER, "height", 1, "test.sn", &arena);

    /* Create params array with 'as ref' qualifier */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 2);
    params[0].name = width_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_AS_REF;
    params[1].name = height_tok;
    params[1].type = int_type;
    params[1].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 2, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: as ref on int is valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as ref' on array parameter (non-primitive) is rejected */
static void test_as_ref_array_param_rejected(void)
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

    /* Create: native fn process(data: int[] as ref): void -- this should fail */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);

    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 1, "test.sn", &arena);

    /* Create param with 'as ref' on array (invalid) */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = data_tok;
    params[0].type = int_array_type;
    params[0].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: as ref only applies to primitives */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that calling a native function with 'as ref' params works with regular vars */
static void test_as_ref_param_call_with_vars(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn set_value(out: int as ref): void */
    Token set_val_tok;
    setup_test_token(&set_val_tok, TOKEN_IDENTIFIER, "set_value", 1, "test.sn", &arena);

    Token out_tok;
    setup_test_token(&out_tok, TOKEN_IDENTIFIER, "out", 1, "test.sn", &arena);

    Parameter *native_params = arena_alloc(&arena, sizeof(Parameter) * 1);
    native_params[0].name = out_tok;
    native_params[0].type = int_type;
    native_params[0].mem_qualifier = MEM_AS_REF;

    /* Create the param_mem_quals array for the function type */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 1);
    param_types[0] = int_type;
    Type *func_type = ast_create_function_type(&arena, void_type, param_types, 1);
    func_type->as.function.param_mem_quals = arena_alloc(&arena, sizeof(MemoryQualifier) * 1);
    func_type->as.function.param_mem_quals[0] = MEM_AS_REF;

    Stmt *native_decl = ast_create_function_stmt(&arena, set_val_tok, native_params, 1, void_type, NULL, 0, &set_val_tok);
    native_decl->as.function.is_native = true;

    /* Create a regular function that calls set_value(x) */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* var x: int = 0 */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 3, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, zero_lit, NULL);

    /* set_value(x) */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "set_value", 4, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Token x_arg_tok;
    setup_test_token(&x_arg_tok, TOKEN_IDENTIFIER, "x", 4, "test.sn", &arena);
    Expr *x_arg = ast_create_variable_expr(&arena, x_arg_tok, &x_arg_tok);

    Expr **args = arena_alloc(&arena, sizeof(Expr *) * 1);
    args[0] = x_arg;
    Expr *call = ast_create_call_expr(&arena, callee, args, 1, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = x_decl;
    body[1] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 2, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, native_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Pointer-to-struct member access tests
 * ============================================================================ */

/* Test: *struct member access is REJECTED in regular (non-native) functions */
static void test_ptr_struct_member_rejected_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create native struct: native struct Point => x: int, y: int */
    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = int_type;
    fields[1].offset = 4;
    fields[1].default_value = NULL;
    fields[1].c_alias = NULL;

    Token struct_tok;
    setup_test_token(&struct_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_tok, point_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create: *Point type */
    Type *ptr_point_type = ast_create_pointer_type(&arena, point_type);

    /* Create native fn that returns *Point (forward declaration) */
    Token get_point_tok;
    setup_test_token(&get_point_tok, TOKEN_IDENTIFIER, "get_point", 2, "test.sn", &arena);
    Stmt *native_decl = ast_create_function_stmt(&arena, get_point_tok, NULL, 0, ptr_point_type, NULL, 0, &get_point_tok);
    native_decl->as.function.is_native = true;
    ast_module_add_statement(&arena, &module, native_decl);

    /* In regular function: var p: *Point = nil ... p.x - should FAIL */
    /* First create variable declaration for p */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_point_type, nil_lit, &p_tok);

    /* Create variable reference for p */
    Expr *p_ref = ast_create_variable_expr(&arena, p_tok, &p_tok);

    /* Create member access: p.x */
    Token x_field_tok;
    setup_test_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Expr *member_access = ast_create_member_expr(&arena, p_ref, x_field_tok, &x_field_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member_access, &x_field_tok);

    /* Create a REGULAR function (not native) with var decl and member access */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = expr_stmt;
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "regular_func", 4, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;  /* Regular function */

    ast_module_add_statement(&arena, &module, func_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    /* Should FAIL - either due to pointer var in regular fn or pointer member access */
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *struct member access is ACCEPTED in native functions */
static void test_ptr_struct_member_accepted_in_native_fn(void)
{
    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create native struct: native struct Point => x: int, y: int */
    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = int_type;
    fields[1].offset = 4;
    fields[1].default_value = NULL;
    fields[1].c_alias = NULL;

    Token struct_tok;
    setup_test_token(&struct_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_tok, point_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create: *Point type */
    Type *ptr_point_type = ast_create_pointer_type(&arena, point_type);

    /* Create native fn that returns *Point (forward declaration) */
    Token get_point_tok;
    setup_test_token(&get_point_tok, TOKEN_IDENTIFIER, "get_point", 2, "test.sn", &arena);
    Stmt *native_decl = ast_create_function_stmt(&arena, get_point_tok, NULL, 0, ptr_point_type, NULL, 0, &get_point_tok);
    native_decl->as.function.is_native = true;
    ast_module_add_statement(&arena, &module, native_decl);

    /* In NATIVE function: var p: *Point = nil ... p.x - should PASS */
    /* First create variable declaration for p */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_point_type, nil_lit, &p_tok);

    /* Create variable reference for p */
    Expr *p_ref = ast_create_variable_expr(&arena, p_tok, &p_tok);

    /* Create member access: p.x */
    Token x_field_tok;
    setup_test_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Expr *member_access = ast_create_member_expr(&arena, p_ref, x_field_tok, &x_field_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member_access, &x_field_tok);

    /* Create a NATIVE function with var decl and member access */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = expr_stmt;
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "native_func", 4, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;  /* Native function */

    ast_module_add_statement(&arena, &module, func_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should PASS - *struct member access allowed in native fn */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main entry point for pointer tests
 * ============================================================================ */

void test_type_checker_native_pointer_main(void)
{
    TEST_SECTION("Native Pointer");

    TEST_RUN("pointer_var_rejected_in_regular_function", test_pointer_var_rejected_in_regular_function);
    TEST_RUN("pointer_var_accepted_in_native_function", test_pointer_var_accepted_in_native_function);
    TEST_RUN("pointer_arithmetic_rejected", test_pointer_arithmetic_rejected);
    TEST_RUN("pointer_nil_comparison_allowed", test_pointer_nil_comparison_allowed);
    TEST_RUN("pointer_pointer_comparison_allowed", test_pointer_pointer_comparison_allowed);
    TEST_RUN("inline_pointer_passing_allowed", test_inline_pointer_passing_allowed);
    TEST_RUN("inline_nil_passing_allowed", test_inline_nil_passing_allowed);
    TEST_RUN("as_val_unwraps_pointer_int", test_as_val_unwraps_pointer_int);
    TEST_RUN("as_val_unwraps_pointer_double", test_as_val_unwraps_pointer_double);
    TEST_RUN("as_val_unwraps_pointer_float", test_as_val_unwraps_pointer_float);
    TEST_RUN("as_val_rejects_non_pointer", test_as_val_rejects_non_pointer);
    TEST_RUN("as_val_char_pointer_to_str", test_as_val_char_pointer_to_str);
    TEST_RUN("as_val_int_pointer_no_cstr_flag", test_as_val_int_pointer_no_cstr_flag);
    TEST_RUN("pointer_return_without_as_val_fails_in_regular_fn", test_pointer_return_without_as_val_fails_in_regular_fn);
    TEST_RUN("pointer_return_with_as_val_succeeds_in_regular_fn", test_pointer_return_with_as_val_succeeds_in_regular_fn);
    TEST_RUN("native_fn_can_store_pointer_return", test_native_fn_can_store_pointer_return);
    TEST_RUN("as_ref_primitive_param_in_native_fn", test_as_ref_primitive_param_in_native_fn);
    TEST_RUN("as_ref_array_param_rejected", test_as_ref_array_param_rejected);
    TEST_RUN("as_ref_param_call_with_vars", test_as_ref_param_call_with_vars);

    /* Pointer-to-struct member access tests */
    TEST_RUN("ptr_struct_member_rejected_in_regular_fn", test_ptr_struct_member_rejected_in_regular_fn);
    TEST_RUN("ptr_struct_member_accepted_in_native_fn", test_ptr_struct_member_accepted_in_native_fn);
}
