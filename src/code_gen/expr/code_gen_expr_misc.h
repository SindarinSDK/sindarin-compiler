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

/* addressOf(expr) - get pointer to value */
char *code_gen_address_of_expression(CodeGen *gen, Expr *expr);

/* valueOf(expr) - dereference pointer or convert *char to str */
char *code_gen_value_of_expression(CodeGen *gen, Expr *expr);

/* copyOf(expr) - deep copy a struct */
char *code_gen_copy_of_expression(CodeGen *gen, Expr *expr);

/* Get the array clone function suffix for a given element type */
const char *get_array_clone_suffix(Type *element_type);

/* sizeof(Type) or sizeof(expr) - compile-time size */
char *code_gen_sizeof_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_MISC_H */
