#ifndef CODE_GEN_EXPR_INCR_H
#define CODE_GEN_EXPR_INCR_H

#include "code_gen.h"
#include "ast.h"

/* Increment expression: x++ */
char *code_gen_increment_expression(CodeGen *gen, Expr *expr);

/* Decrement expression: x-- */
char *code_gen_decrement_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_INCR_H */
