#ifndef CODE_GEN_EXPR_CORE_H
#define CODE_GEN_EXPR_CORE_H

#include "code_gen.h"
#include "ast.h"

/* Literal expression code generation */
char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr);

/* Variable/identifier expression code generation */
char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr);

/* Assignment expression code generation */
char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr);

/* Index assignment expression code generation */
char *code_gen_index_assign_expression(CodeGen *gen, IndexAssignExpr *expr);

#endif /* CODE_GEN_EXPR_CORE_H */
