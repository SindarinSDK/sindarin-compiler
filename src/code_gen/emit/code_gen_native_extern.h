/**
 * code_gen_native_extern.h - Native extern declaration emission
 *
 * Contains functions for emitting extern declarations for native
 * functions without bodies during code generation.
 */

#ifndef CODE_GEN_NATIVE_EXTERN_H
#define CODE_GEN_NATIVE_EXTERN_H

#include "code_gen.h"
#include "ast.h"

/**
 * Check if a function name is a C standard library function.
 * Used to avoid emitting conflicting extern declarations.
 */
bool is_c_stdlib_function(const char *name);

/**
 * Generate extern declaration for native function without body.
 */
void code_gen_native_extern_declaration(CodeGen *gen, FunctionStmt *fn);

/**
 * Emit native extern declarations from statements (including imported modules).
 */
void code_gen_emit_imported_native_externs(CodeGen *gen, Stmt **statements, int count, int *extern_count);

#endif /* CODE_GEN_NATIVE_EXTERN_H */
