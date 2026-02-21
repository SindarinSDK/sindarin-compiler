/*
 * code_gen_stmt_func_promote.h - Return value promotion helpers
 */

#ifndef CODE_GEN_STMT_FUNC_PROMOTE_H
#define CODE_GEN_STMT_FUNC_PROMOTE_H

#include "code_gen.h"
#include "ast.h"

/* Promote array return values to target arena */
void code_gen_promote_array_return(CodeGen *gen, Type *return_type, const char *target_arena, int indent);

/* Promote struct return values to target arena */
void code_gen_promote_struct_return(CodeGen *gen, Type *return_type, const char *target_arena, int indent);

/* Main entry point for return value promotion.
 * target_arena: "__sn__self->__arena__" for instance methods, "__caller_arena__" for regular functions */
void code_gen_return_promotion(CodeGen *gen, Type *return_type, bool is_main, bool is_shared, const char *target_arena, int indent);

/* Promote self handle fields from __local_arena__ back to self->__arena__ before condemn.
 * Only promotes fields whose arena matches __local_arena__ (modified by THIS method call). */
void code_gen_promote_self_fields(CodeGen *gen, StructDeclStmt *struct_decl, int indent);

#endif /* CODE_GEN_STMT_FUNC_PROMOTE_H */
