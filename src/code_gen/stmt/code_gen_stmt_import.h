/*
 * code_gen_stmt_import.h - Import statement helpers
 */

#ifndef CODE_GEN_STMT_IMPORT_H
#define CODE_GEN_STMT_IMPORT_H

#include "code_gen.h"
#include "ast.h"
#include "symbol_table.h"

/* Emit forward declarations for functions in imported modules */
void emit_import_forward_declarations_recursive(CodeGen *gen, Stmt **stmts, int count, const char *ns_prefix);

/* Add namespace symbols to current scope */
void add_namespace_symbols_to_scope(CodeGen *gen, Symbol *ns_sym);

#endif /* CODE_GEN_STMT_IMPORT_H */
