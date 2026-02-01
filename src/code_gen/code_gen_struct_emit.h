/**
 * code_gen_struct_emit.h - Struct typedef emission for code generation
 *
 * Contains functions for emitting struct typedefs and native struct
 * forward declarations during code generation.
 */

#ifndef CODE_GEN_STRUCT_EMIT_H
#define CODE_GEN_STRUCT_EMIT_H

#include "code_gen.h"
#include "ast.h"

/**
 * Emit a single struct typedef.
 * Handles regular structs and packed structs.
 */
void code_gen_emit_struct_typedef(CodeGen *gen, StructDeclStmt *struct_decl, int *count_ptr);

/**
 * Recursively emit struct typedefs from imported modules.
 */
void code_gen_emit_imported_struct_typedefs(CodeGen *gen, Stmt **statements, int count, int *struct_count);

/**
 * Tracking structure for emitted native alias forward declarations.
 */
typedef struct {
    const char **names;
    int count;
    int capacity;
} EmittedNativeAliases;

/**
 * Emit native struct forward declaration (with deduplication).
 */
void code_gen_emit_native_alias_fwd(CodeGen *gen, StructDeclStmt *struct_decl,
                                    int *count_ptr, EmittedNativeAliases *emitted);

/**
 * Recursively emit native struct forward declarations from imports.
 */
void code_gen_emit_imported_native_aliases(CodeGen *gen, Stmt **statements, int count,
                                           int *alias_count, EmittedNativeAliases *emitted);

#endif /* CODE_GEN_STRUCT_EMIT_H */
