/*
 * code_gen_stmt_func_promote.h - Return value promotion helpers
 */

#ifndef CODE_GEN_STMT_FUNC_PROMOTE_H
#define CODE_GEN_STMT_FUNC_PROMOTE_H

#include "code_gen.h"
#include "ast.h"

/* Promote array return values from local to caller arena */
void code_gen_promote_array_return(CodeGen *gen, Type *return_type, int indent);

/* Promote struct return values from local to caller arena */
void code_gen_promote_struct_return(CodeGen *gen, Type *return_type, int indent);

/* Main entry point for return value promotion */
void code_gen_return_promotion(CodeGen *gen, Type *return_type, bool is_main, bool is_shared, int indent);

#endif /* CODE_GEN_STMT_FUNC_PROMOTE_H */
