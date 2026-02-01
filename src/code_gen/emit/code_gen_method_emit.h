/**
 * code_gen_method_emit.h - Struct method emission for code generation
 *
 * Contains functions for emitting struct method forward declarations
 * and implementations during code generation.
 */

#ifndef CODE_GEN_METHOD_EMIT_H
#define CODE_GEN_METHOD_EMIT_H

#include "code_gen.h"
#include "ast.h"

/**
 * Emit forward declarations for all struct methods in the module.
 * Returns the count of methods emitted.
 */
int code_gen_emit_struct_method_forwards(CodeGen *gen, Stmt **statements, int count);

/**
 * Emit implementations for all struct methods in the module.
 */
void code_gen_emit_struct_method_implementations(CodeGen *gen, Stmt **statements, int count);

#endif /* CODE_GEN_METHOD_EMIT_H */
