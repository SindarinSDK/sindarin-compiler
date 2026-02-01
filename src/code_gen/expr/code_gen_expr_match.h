#ifndef CODE_GEN_EXPR_MATCH_H
#define CODE_GEN_EXPR_MATCH_H

#include "code_gen.h"
#include "ast.h"

/* Match expression: match subject => pattern => body, ... */
char *code_gen_match_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_MATCH_H */
