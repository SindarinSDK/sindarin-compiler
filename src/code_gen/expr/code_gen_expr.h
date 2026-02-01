#ifndef CODE_GEN_EXPR_H
#define CODE_GEN_EXPR_H

#include "code_gen.h"
#include "ast.h"
#include <stdbool.h>

/* Check if expression produces a temporary that needs cleanup */
bool expression_produces_temp(Expr *expr);

/* Check if an expression is provably non-negative (for array index optimization) */
bool is_provably_non_negative(CodeGen *gen, Expr *expr);

/* Main expression code generation entry point */
char *code_gen_expression(CodeGen *gen, Expr *expr);

/* Specific expression generators */
char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr);
char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr);
char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr);
char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr);
char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr);
char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr);
char *code_gen_call_expression(CodeGen *gen, Expr *expr);
char *code_gen_array_expression(CodeGen *gen, Expr *e);
char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr);
char *code_gen_increment_expression(CodeGen *gen, Expr *expr);
char *code_gen_decrement_expression(CodeGen *gen, Expr *expr);
char *code_gen_member_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_H */
