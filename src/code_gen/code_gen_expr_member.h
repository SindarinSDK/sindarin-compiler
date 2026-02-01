#ifndef CODE_GEN_EXPR_MEMBER_H
#define CODE_GEN_EXPR_MEMBER_H

#include "code_gen.h"
#include "ast.h"

/* Member expression: object.member (namespace or struct access) */
char *code_gen_member_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_MEMBER_H */
