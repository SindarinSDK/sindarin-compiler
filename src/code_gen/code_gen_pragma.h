/**
 * code_gen_pragma.h - Pragma handling for code generation
 *
 * Contains functions for collecting and managing pragma directives
 * (include, link, source) during code generation.
 */

#ifndef CODE_GEN_PRAGMA_H
#define CODE_GEN_PRAGMA_H

#include "code_gen.h"
#include "ast.h"

/**
 * Add a pragma include directive (with deduplication).
 */
void code_gen_add_pragma_include(CodeGen *gen, const char *include);

/**
 * Add a pragma link directive (with deduplication).
 */
void code_gen_add_pragma_link(CodeGen *gen, const char *link);

/**
 * Add a pragma source file with location info (with deduplication).
 */
void code_gen_add_pragma_source(CodeGen *gen, const char *source, const char *source_dir);

/**
 * Collect pragma directives from a list of statements (recursively handles imports).
 */
void code_gen_collect_pragmas(CodeGen *gen, Stmt **statements, int count);

/**
 * Extract directory from a file path.
 */
const char *get_directory_from_path(Arena *arena, const char *filepath);

#endif /* CODE_GEN_PRAGMA_H */
