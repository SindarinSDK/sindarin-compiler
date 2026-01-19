#ifndef CODE_GEN_EXPR_BINARY_H
#define CODE_GEN_EXPR_BINARY_H

#include "code_gen.h"
#include "ast.h"

/* Binary expression code generation */
char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr);

/* Unary expression code generation */
char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr);

#endif /* CODE_GEN_EXPR_BINARY_H */
