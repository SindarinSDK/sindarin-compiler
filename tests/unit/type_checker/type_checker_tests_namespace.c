// tests/type_checker_tests_namespace.c
// Tests for namespace and import statement type checking

#include "../type_checker/type_checker_util.h"

/* Redefine TOKEN_LITERAL with filename field for namespace tests */
#undef TOKEN_LITERAL
#define TOKEN_LITERAL(str) ((Token){ .start = str, .length = sizeof(str) - 1, .line = 1, .type = TOKEN_IDENTIFIER, .filename = "test.sn" })

/* Test that namespaced import creates namespace correctly */
static void test_namespace_import_creates_namespace(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a namespace */
    Token ns_name = TOKEN_LITERAL("math");
    symbol_table_add_namespace(&table, ns_name);

    /* Verify namespace exists */
    assert(symbol_table_is_namespace(&table, ns_name) == true);

    /* Add a function to the namespace */
    Token func_name = TOKEN_LITERAL("add");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = int_type;
    param_types[1] = int_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 2);

    symbol_table_add_symbol_to_namespace(&table, ns_name, func_name, func_type);

    /* Verify we can look up the symbol in the namespace */
    Symbol *found = symbol_table_lookup_in_namespace(&table, ns_name, func_name);
    assert(found != NULL);
    assert(found->type != NULL);
    assert(found->type->kind == TYPE_FUNCTION);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that namespace identifier validation rejects keywords */
static void test_namespace_rejects_keywords(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Try to use a keyword as namespace - this should be caught by the type checker,
     * not the symbol table itself. Here we verify the symbol table can still
     * create a namespace with any name (the validation is at a higher level). */

    /* The actual keyword checking is done in type_check_import_stmt using
     * is_reserved_keyword(). We verify the infrastructure here. */

    Token valid_ns = TOKEN_LITERAL("mymodule");
    symbol_table_add_namespace(&table, valid_ns);
    assert(symbol_table_is_namespace(&table, valid_ns) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that namespace doesn't conflict with existing variable names */
static void test_namespace_no_variable_conflict(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable named 'x' */
    Token var_name = TOKEN_LITERAL("x");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    symbol_table_add_symbol(&table, var_name, int_type);

    /* Verify 'x' is a variable, not a namespace */
    assert(symbol_table_is_namespace(&table, var_name) == false);
    Symbol *found = symbol_table_lookup_symbol(&table, var_name);
    assert(found != NULL);
    assert(found->is_namespace == false);

    /* Now add a namespace 'math' */
    Token ns_name = TOKEN_LITERAL("math");
    symbol_table_add_namespace(&table, ns_name);
    assert(symbol_table_is_namespace(&table, ns_name) == true);

    /* Variable 'x' should still exist and not be affected */
    Symbol *var_found = symbol_table_lookup_symbol(&table, var_name);
    assert(var_found != NULL);
    assert(var_found->is_namespace == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that using namespace as variable triggers error check */
static void test_namespace_as_variable_detected(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a namespace */
    Token ns_name = TOKEN_LITERAL("math");
    symbol_table_add_namespace(&table, ns_name);

    /* Look up the namespace symbol */
    Symbol *sym = symbol_table_lookup_symbol(&table, ns_name);
    assert(sym != NULL);

    /* Namespace symbols have is_namespace=true and type=NULL */
    assert(sym->is_namespace == true);
    assert(sym->type == NULL);

    /* This is what type_check_variable checks to detect namespace misuse */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test undefined namespace in member access detection */
static void test_undefined_namespace_detected(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Don't add any namespace, just check that lookup fails */
    Token fake_ns = TOKEN_LITERAL("fakenamespace");

    /* symbol_table_is_namespace should return false for undefined namespace */
    assert(symbol_table_is_namespace(&table, fake_ns) == false);

    /* Lookup should also not find it */
    Symbol *sym = symbol_table_lookup_symbol(&table, fake_ns);
    assert(sym == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test same module imported both ways (direct and namespaced) */
static void test_same_module_both_import_styles(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Simulate direct import: add 'add' function to global scope */
    Token func_name = TOKEN_LITERAL("add");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = int_type;
    param_types[1] = int_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 2);
    symbol_table_add_symbol(&table, func_name, func_type);

    /* Simulate namespaced import: create namespace and add same function */
    Token ns_name = TOKEN_LITERAL("math");
    symbol_table_add_namespace(&table, ns_name);
    symbol_table_add_symbol_to_namespace(&table, ns_name, func_name, func_type);

    /* Verify both access patterns work */

    /* Direct access: 'add' should be in global scope */
    Symbol *direct_sym = symbol_table_lookup_symbol(&table, func_name);
    assert(direct_sym != NULL);
    assert(direct_sym->type != NULL);
    assert(direct_sym->type->kind == TYPE_FUNCTION);

    /* Namespaced access: 'math.add' should also work */
    Symbol *ns_sym = symbol_table_lookup_in_namespace(&table, ns_name, func_name);
    assert(ns_sym != NULL);
    assert(ns_sym->type != NULL);
    assert(ns_sym->type->kind == TYPE_FUNCTION);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test namespace symbol lookup fails for non-existent symbol */
static void test_namespace_symbol_not_found(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create namespace with one function */
    Token ns_name = TOKEN_LITERAL("math");
    symbol_table_add_namespace(&table, ns_name);

    Token add_name = TOKEN_LITERAL("add");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = int_type;
    param_types[1] = int_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 2);
    symbol_table_add_symbol_to_namespace(&table, ns_name, add_name, func_type);

    /* Lookup non-existent function should return NULL */
    Token sub_name = TOKEN_LITERAL("subtract");
    Symbol *not_found = symbol_table_lookup_in_namespace(&table, ns_name, sub_name);
    assert(not_found == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test get_module_symbols helper function */
static void test_get_module_symbols_empty(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an empty module */
    Module empty_module;
    empty_module.statements = NULL;
    empty_module.count = 0;
    empty_module.capacity = 0;
    empty_module.filename = "test.sn";

    Token **symbols = NULL;
    Type **types = NULL;
    int count = -1;

    get_module_symbols(&empty_module, &table, &symbols, &types, &count);

    /* Should return 0 count and NULL arrays for empty module */
    assert(count == 0);
    /* Note: symbols and types may be non-NULL but count is 0 */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test get_module_symbols with NULL input */
static void test_get_module_symbols_null(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token **symbols = (Token **)1;  /* Non-NULL to verify it gets set to NULL */
    Type **types = (Type **)1;
    int count = 999;

    get_module_symbols(NULL, &table, &symbols, &types, &count);

    /* Should handle NULL gracefully */
    assert(count == 0);
    assert(symbols == NULL);
    assert(types == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Main entry point for namespace type checker tests */
void test_type_checker_namespace_main(void)
{
    TEST_SECTION("Type Checker Namespaces");

    TEST_RUN("namespace_import_creates_namespace", test_namespace_import_creates_namespace);
    TEST_RUN("namespace_rejects_keywords", test_namespace_rejects_keywords);
    TEST_RUN("namespace_no_variable_conflict", test_namespace_no_variable_conflict);
    TEST_RUN("namespace_as_variable_detected", test_namespace_as_variable_detected);
    TEST_RUN("undefined_namespace_detected", test_undefined_namespace_detected);
    TEST_RUN("same_module_both_import_styles", test_same_module_both_import_styles);
    TEST_RUN("namespace_symbol_not_found", test_namespace_symbol_not_found);
    TEST_RUN("get_module_symbols_empty", test_get_module_symbols_empty);
    TEST_RUN("get_module_symbols_null", test_get_module_symbols_null);
}
