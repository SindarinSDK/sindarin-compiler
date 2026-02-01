#ifndef CODE_GEN_EXPR_TYPE_H
#define CODE_GEN_EXPR_TYPE_H

#include "code_gen.h"
#include "ast.h"

/* Sizeof expression: sizeof(Type) or sizeof(expr) */
char *code_gen_sizeof_expression(CodeGen *gen, Expr *expr);

/* Typeof expression: typeof(value) or typeof(Type) */
char *code_gen_typeof_expression(CodeGen *gen, Expr *expr);

/* Is expression: expr is Type */
char *code_gen_is_expression(CodeGen *gen, Expr *expr);

/* As type expression: expr as Type */
char *code_gen_as_type_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_TYPE_H */
