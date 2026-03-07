#ifndef GEN_MODEL_H
#define GEN_MODEL_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include "code_gen.h"
#include <json-c/json.h>

/* Build a language-agnostic JSON model from a typed/optimized AST module.
 * The returned json_object must be freed by the caller with json_object_put(). */
json_object *gen_model_build(Arena *arena, Module *module, SymbolTable *symbol_table,
                             ArithmeticMode arithmetic_mode);

/* Serialize the model to a file. Returns 0 on success, non-zero on error. */
int gen_model_write(json_object *model, const char *output_path);

/* --- Internal functions (used across gen_model_*.c files) --- */

/* Type handling */
json_object *gen_model_type(Arena *arena, Type *type);
const char *gen_model_type_kind_str(TypeKind kind);

/* Statement emission */
json_object *gen_model_stmt(Arena *arena, Stmt *stmt, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode);

/* Expression emission */
json_object *gen_model_expr(Arena *arena, Expr *expr, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode);

/* Function emission */
json_object *gen_model_function(Arena *arena, FunctionStmt *func, SymbolTable *symbol_table,
                                ArithmeticMode arithmetic_mode);

/* Struct emission */
json_object *gen_model_struct(Arena *arena, StructDeclStmt *decl, SymbolTable *symbol_table,
                              ArithmeticMode arithmetic_mode);

#endif
