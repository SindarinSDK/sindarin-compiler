#ifndef CODE_GEN_EXPR_MISC_H
#define CODE_GEN_EXPR_MISC_H

#include "code_gen.h"
#include "ast.h"

/* Range expression: start..end */
char *code_gen_range_expression(CodeGen *gen, Expr *expr);

/* Spread expression: ...array */
char *code_gen_spread_expression(CodeGen *gen, Expr *expr);

/* Sized array allocation: int[size] with default */
char *code_gen_sized_array_alloc_expression(CodeGen *gen, Expr *expr);

/* As ref expression: x as ref */
char *code_gen_as_ref_expression(CodeGen *gen, Expr *expr);

/* As val expression: ptr as val (dereference or deep copy) */
char *code_gen_as_val_expression(CodeGen *gen, Expr *expr);

/* Get the array clone function suffix for a given element type */
const char *get_array_clone_suffix(Type *element_type);

#endif /* CODE_GEN_EXPR_MISC_H */
