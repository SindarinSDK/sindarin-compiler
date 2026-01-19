// tests/code_gen_tests_memory.c
// Code generation tests for memory management features (private functions, shared loops)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"
#include "../file.h"
#include "../test_utils.h"

static const char *test_output_path_mem = "test_output_mem.c";
static const char *expected_output_path_mem = "expected_output_mem.c";

static void setup_token_mem(Token *token, SnTokenType type, const char *lexeme)
{
    token_init(token, type, lexeme, (int)strlen(lexeme), 1, "test.sn");
}

static void test_code_gen_private_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path_mem);

    // Create: private fn compute(): int => return 42
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token ret_tok;
    setup_token_mem(&ret_tok, TOKEN_RETURN, "return");
    Token lit_tok;
    setup_token_mem(&lit_tok, TOKEN_INT_LITERAL, "42");
    LiteralValue v = {.int_value = 42};
    Expr *ret_val = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token_mem(&func_name_tok, TOKEN_IDENTIFIER, "compute");
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_PRIVATE;

    ast_module_add_statement(&arena, &module, func_decl);
    code_gen_module(&gen, &module);

    // Expected: private function receives arena and creates/destroys its own local arena
    // Note: forward declaration is emitted first
    const char *expected = get_expected(&arena,
                                        "long long compute(RtArena *);\n\n"
                                        "long long compute(RtArena *__caller_arena__) {\n"
                                        "    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);\n"
                                        "    long long _return_value = 0;\n"
                                        "    _return_value = 42LL;\n"
                                        "    goto compute_return;\n"
                                        "compute_return:\n"
                                        "    rt_arena_destroy(__local_arena__);\n"
                                        "    return _return_value;\n"
                                        "}\n\n"
                                        "int main() {\n"
                                        "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                        "    int _return_value = 0;\n"
                                        "    goto main_return;\n"
                                        "main_return:\n"
                                        "    rt_arena_destroy(__local_arena__);\n"
                                        "    return _return_value;\n"
                                        "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path_mem, expected);
    compare_output_files(test_output_path_mem, expected_output_path_mem);
    remove_test_file(test_output_path_mem);
    remove_test_file(expected_output_path_mem);

    arena_free(&arena);
}

static void test_code_gen_shared_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path_mem);

    // Create: shared fn helper(): int => return 1
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token ret_tok;
    setup_token_mem(&ret_tok, TOKEN_RETURN, "return");
    Token lit_tok;
    setup_token_mem(&lit_tok, TOKEN_INT_LITERAL, "1");
    LiteralValue v = {.int_value = 1};
    Expr *ret_val = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token_mem(&func_name_tok, TOKEN_IDENTIFIER, "helper");
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_SHARED;

    ast_module_add_statement(&arena, &module, func_decl);
    code_gen_module(&gen, &module);

    // Expected: shared function aliases caller's arena (no new arena, no destroy)
    // Note: forward declaration is emitted first
    const char *expected = get_expected(&arena,
                                        "long long helper(RtArena *);\n\n"
                                        "long long helper(RtArena *__caller_arena__) {\n"
                                        "    RtArena *__local_arena__ = __caller_arena__;\n"
                                        "    long long _return_value = 0;\n"
                                        "    _return_value = 1LL;\n"
                                        "    goto helper_return;\n"
                                        "helper_return:\n"
                                        "    return _return_value;\n"
                                        "}\n\n"
                                        "int main() {\n"
                                        "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                        "    int _return_value = 0;\n"
                                        "    goto main_return;\n"
                                        "main_return:\n"
                                        "    rt_arena_destroy(__local_arena__);\n"
                                        "    return _return_value;\n"
                                        "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path_mem, expected);
    compare_output_files(test_output_path_mem, expected_output_path_mem);
    remove_test_file(test_output_path_mem);
    remove_test_file(expected_output_path_mem);

    arena_free(&arena);
}

static void test_code_gen_default_function(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path_mem);

    // Create: fn regular(): int => return 5
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token ret_tok;
    setup_token_mem(&ret_tok, TOKEN_RETURN, "return");
    Token lit_tok;
    setup_token_mem(&lit_tok, TOKEN_INT_LITERAL, "5");
    LiteralValue v = {.int_value = 5};
    Expr *ret_val = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token_mem(&func_name_tok, TOKEN_IDENTIFIER, "regular");
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_DEFAULT;

    ast_module_add_statement(&arena, &module, func_decl);
    code_gen_module(&gen, &module);

    // Expected: default function receives arena and creates/destroys its own local arena
    // Note: forward declaration is emitted first
    const char *expected = get_expected(&arena,
                                        "long long regular(RtArena *);\n\n"
                                        "long long regular(RtArena *__caller_arena__) {\n"
                                        "    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);\n"
                                        "    long long _return_value = 0;\n"
                                        "    _return_value = 5LL;\n"
                                        "    goto regular_return;\n"
                                        "regular_return:\n"
                                        "    rt_arena_destroy(__local_arena__);\n"
                                        "    return _return_value;\n"
                                        "}\n\n"
                                        "int main() {\n"
                                        "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                        "    int _return_value = 0;\n"
                                        "    goto main_return;\n"
                                        "main_return:\n"
                                        "    rt_arena_destroy(__local_arena__);\n"
                                        "    return _return_value;\n"
                                        "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path_mem, expected);
    compare_output_files(test_output_path_mem, expected_output_path_mem);
    remove_test_file(test_output_path_mem);
    remove_test_file(expected_output_path_mem);

    arena_free(&arena);
}

void test_code_gen_memory_main(void)
{
    TEST_SECTION("Code Gen Memory Tests");
    TEST_RUN("code_gen_private_function", test_code_gen_private_function);
    TEST_RUN("code_gen_shared_function", test_code_gen_shared_function);
    TEST_RUN("code_gen_default_function", test_code_gen_default_function);
}
