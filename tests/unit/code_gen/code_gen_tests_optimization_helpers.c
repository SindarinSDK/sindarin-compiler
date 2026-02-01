/**
 * code_gen_tests_optimization_helpers.c - Helper functions for optimization tests
 *
 * Contains helper functions used by optimization test modules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include "../arena.h"
#include "../ast.h"
#include "../code_gen.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "../symbol_table.h"

/* Cross-platform null device */
#ifdef _WIN32
#define NULL_DEVICE "NUL"
#else
#define NULL_DEVICE "/dev/null"
#endif

/* Helper to set up a token */
static void init_token(Token *tok, SnTokenType type, const char *lexeme)
{
    tok->type = type;
    tok->start = lexeme;
    tok->length = strlen(lexeme);
    tok->line = 1;
    tok->filename = "test.sn";
}

/* Helper to create an int literal expression */
static Expr *make_int_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_INT_LITERAL, "0");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *int_type = ast_create_primitive_type(arena, TYPE_INT);
    return ast_create_literal_expr(arena, lit_val, int_type, false, &tok);
}

/* Helper to create a long literal expression */
static Expr *make_long_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_LONG_LITERAL, "0LL");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *long_type = ast_create_primitive_type(arena, TYPE_LONG);
    return ast_create_literal_expr(arena, lit_val, long_type, false, &tok);
}

/* Helper to create a double literal expression */
static Expr *make_double_literal(Arena *arena, double value)
{
    Token tok;
    init_token(&tok, TOKEN_DOUBLE_LITERAL, "0.0");

    LiteralValue lit_val;
    lit_val.double_value = value;
    Type *double_type = ast_create_primitive_type(arena, TYPE_DOUBLE);
    return ast_create_literal_expr(arena, lit_val, double_type, false, &tok);
}

/* Helper to create a bool literal expression */
static Expr *make_bool_literal(Arena *arena, bool value)
{
    Token tok;
    init_token(&tok, TOKEN_BOOL_LITERAL, value ? "true" : "false");

    LiteralValue lit_val;
    lit_val.bool_value = value ? 1 : 0;
    Type *bool_type = ast_create_primitive_type(arena, TYPE_BOOL);
    return ast_create_literal_expr(arena, lit_val, bool_type, false, &tok);
}

/* Helper to create a binary expression */
static Expr *make_binary_expr(Arena *arena, Expr *left, SnTokenType op, Expr *right)
{
    Token tok;
    init_token(&tok, op, "+");
    return ast_create_binary_expr(arena, left, op, right, &tok);
}

/* Helper to create a unary expression */
static Expr *make_unary_expr(Arena *arena, SnTokenType op, Expr *operand)
{
    Token tok;
    init_token(&tok, op, "-");
    return ast_create_unary_expr(arena, op, operand, &tok);
}
