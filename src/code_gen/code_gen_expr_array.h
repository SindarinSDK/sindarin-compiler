#ifndef CODE_GEN_EXPR_ARRAY_H
#define CODE_GEN_EXPR_ARRAY_H

#include "code_gen.h"

/* Array expression code generation */
char *code_gen_array_expression(CodeGen *gen, Expr *e);
char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr);
char *code_gen_array_slice_expression(CodeGen *gen, Expr *expr);

/* Helper function for array index optimization */
bool is_provably_non_negative(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_ARRAY_H */
