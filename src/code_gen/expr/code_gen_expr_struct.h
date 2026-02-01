#ifndef CODE_GEN_EXPR_STRUCT_H
#define CODE_GEN_EXPR_STRUCT_H

#include "code_gen.h"
#include "ast.h"

/* Struct literal expression: Point { x: 1.0, y: 2.0 } */
char *code_gen_struct_literal_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_STRUCT_H */
