/*
 * code_gen_boundary.h - Arena boundary protocol
 *
 * Every function with its own __local_arena__ is an ownership domain.
 * Handles entering must be cloned (callee owns copies).
 * Handles exiting must be promoted/cloned (caller owns result).
 *
 * This module formalises the arena boundary protocol as a first-class
 * codegen concern, ensuring consistent ownership transfer at every
 * function and method boundary.
 */

#ifndef CODE_GEN_BOUNDARY_H
#define CODE_GEN_BOUNDARY_H

#include "code_gen.h"
#include "ast.h"

/* Clone incoming handle-type parameters into __local_arena__.
 * Called immediately after __local_arena__ creation in any function or
 * instance method that has its own arena. Ensures the callee owns all
 * handle data it works with, so the caller can safely free its temps.
 *
 * Clones:
 *   - String parameters: param = rt_arena_v2_clone(__local_arena__, param)
 *   - Struct parameters with string fields: each string field cloned
 * Skips:
 *   - 'as ref' struct parameters (caller retains ownership)
 *   - Array parameters (not currently cloned)
 *   - Native struct parameters */
void code_gen_boundary_enter(CodeGen *gen, Parameter *params, int param_count, int indent);

/* Promote or clone outgoing return value to __caller_arena__.
 * Called at the return label before arena condemnation.
 *
 * If the return value is on __local_arena__, promotes (moves) it.
 * If the return value is NOT on __local_arena__ (e.g. borrowed from
 * self in an instance method), clones (copies) it so the caller
 * receives a handle it exclusively owns.
 *
 * target_arena: "__caller_arena__" for regular functions,
 *               "__sn__self->__arena__" for struct-returning instance methods */
void code_gen_boundary_return(CodeGen *gen, Type *return_type, bool is_main,
                              const char *target_arena, int indent);

/* Promote self handle fields from __local_arena__ to self->__arena__
 * before the method's local arena is condemned. Only promotes fields
 * whose arena matches __local_arena__ (modified during THIS method call). */
void code_gen_boundary_self_promote(CodeGen *gen, StructDeclStmt *struct_decl, int indent);

#endif /* CODE_GEN_BOUNDARY_H */
