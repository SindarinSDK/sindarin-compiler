#ifndef CODE_GEN_EXPR_ACCESS_H
#define CODE_GEN_EXPR_ACCESS_H

#include "code_gen.h"
#include "ast.h"

/* Member access expression: struct.field */
char *code_gen_member_access_expression(CodeGen *gen, Expr *expr);

/* Compound assignment expression: x += 5 */
char *code_gen_compound_assign_expression(CodeGen *gen, Expr *expr);

/* Member assign expression: struct.field = value */
char *code_gen_member_assign_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_ACCESS_H */
