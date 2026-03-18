#ifndef GEN_MODEL_H
#define GEN_MODEL_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include "compiler.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Add a source_file field to a JSON object, normalizing backslashes to forward slashes
 * so that JSON model output is consistent across platforms. */
static inline void gen_model_add_source_file(json_object *obj, const char *filename)
{
    if (!filename) return;
    char buf[1024];
    size_t len = strlen(filename);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, filename, len);
    buf[len] = '\0';
    for (size_t i = 0; i < len; i++)
        if (buf[i] == '\\') buf[i] = '/';
    json_object_object_add(obj, "source_file", json_object_new_string(buf));
}

/* Build a language-agnostic JSON model from a typed/optimized AST module.
 * The returned json_object must be freed by the caller with json_object_put(). */
json_object *gen_model_build(Arena *arena, Module *module, SymbolTable *symbol_table,
                             ArithmeticMode arithmetic_mode);

/* Serialize the model to a file. Returns 0 on success, non-zero on error. */
int gen_model_write(json_object *model, const char *output_path);

/* Global lambda collection - populated during model building */
extern json_object *g_model_lambdas;

/* Global thread collection - populated during model building */
extern json_object *g_model_threads;
extern int g_model_thread_count;
extern int g_model_lambda_count;

/* Scope depth at the time prescan_function_body runs (before it pushes its own scope).
 * Variables declared at this depth or below are module-level C globals and must
 * never be added to lambda closure captures. */
extern int g_prescan_function_entry_depth;

/* Global captured-variable set - populated by pre-scanning function bodies.
 * Variables in this set need handle-based (promoted) storage in the outer scope
 * so that lambdas can capture them by reference. */
extern char **g_captured_vars;
extern int g_captured_var_count;

/* Global thread-handle-variable set - populated by pre-scanning function bodies.
 * Variables in this set are assigned thread_spawn results outside their var_decl
 * (e.g., conditional spawns) and need a companion SnThread* variable. */
extern char **g_thread_handle_vars;
extern int g_thread_handle_var_count;

/* Suppress auto-cleanup on local array/string vars (set for functions returning structs with heap fields) */
extern bool g_suppress_local_cleanup;

/* Set to true when generating lambda body statements — prevents return from
 * nulling captured variables (they belong to the outer scope, not the lambda). */
extern bool g_in_lambda_body;

/* Set to true when generating main()'s body when main returns void in Sindarin.
 * C requires main to return int, so bare returns must become return 0; */
extern bool g_in_main_void;

/* Set to true when generating the inner call of a thread_spawn expression.
 * Thread args need owned copies, not borrowed pointers, so borrow arg
 * logic must be skipped. */
extern bool g_in_thread_spawn_call;

/* Maps closure variable names to their lambda_ids for return-site cleanup.
 * Set when a var_decl has closure_lambda_id with has_ref_captures. */
#define MAX_CLOSURE_VAR_MAP 64
extern char *g_closure_var_names[MAX_CLOSURE_VAR_MAP];
extern int g_closure_var_lambda_ids[MAX_CLOSURE_VAR_MAP];
extern int g_closure_var_count;

/* Global fn-wrapper collection for function-as-parameter wrapping */
extern json_object *g_model_fn_wrappers;
extern int g_model_fn_wrapper_count;

/* Current namespace prefix for namespaced imports (NULL when not inside a namespace) */
extern const char *g_model_namespace_prefix;

/* Module-level statement access for callee body analysis */
extern Stmt **g_model_module_stmts;
extern int g_model_module_stmt_count;

/* Module-level variable and function names for the current namespace import.
 * When g_model_namespace_prefix is set and a function body references one of
 * these names, the reference gets namespace-prefixed in the model.
 *
 * Static vars (shared across aliases) use g_model_canonical_prefix.
 * Instance vars (per-alias) use g_model_namespace_prefix. */
extern const char **g_model_ns_static_var_names;
extern int g_model_ns_static_var_count;
extern const char **g_model_ns_instance_var_names;
extern int g_model_ns_instance_var_count;
extern const char **g_model_ns_fn_names;
extern const char **g_model_ns_fn_aliases; /* c_alias for each fn (NULL if none) */
extern int g_model_ns_fn_count;

/* Canonical module prefix for static variable sharing.
 * Derived from module filename (e.g., "level_01").
 * All aliases of the same module share static vars under this prefix. */
extern const char *g_model_canonical_prefix;

/* Post-processing pass: flatten method chains into sequential statements */
void gen_model_flatten_chains(json_object *model);

/* --- Internal functions (used across gen_model_*.c files) --- */

/* Enum-to-string conversions (shared by stmt, func, struct) */
const char *gen_model_mem_qual_str(MemoryQualifier mq);
const char *gen_model_sync_mod_str(SyncModifier sm);
const char *gen_model_func_mod_str(FunctionModifier fm);

/* Type category classification — replaces scattered inline type checks */
typedef enum {
    TYPE_CAT_SCALAR,      /* int/int32/uint/uint32/long/double/float/bool/byte/char/void/nil/pointer/opaque */
    TYPE_CAT_OWNED,       /* string/array/function — caller-owned heap allocation */
    TYPE_CAT_REFCOUNTED,  /* struct with pass_self_by_ref — reference counted */
    TYPE_CAT_COMPOSITE,   /* struct without pass_self_by_ref, has heap fields — needs cleanup */
    TYPE_CAT_INERT        /* struct without pass_self_by_ref, no heap fields — plain value */
} TypeCategory;

TypeCategory gen_model_type_category(Type *type);
const char *gen_model_type_category_str(TypeCategory cat);

/* Type utilities */
json_object *gen_model_type(Arena *arena, Type *type);
const char *gen_model_type_kind_str(TypeKind kind);
bool gen_model_type_has_heap_fields(Type *type);
const char *gen_model_var_cleanup_kind(Type *type, bool suppress_local);
void gen_model_emit_param_cleanup(json_object *param_obj, Parameter *param, bool callee_is_native);

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
