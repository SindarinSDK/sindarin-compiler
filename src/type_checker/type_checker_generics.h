/* type_checker_generics.h - Generic template registry and instantiation system
 *
 * This module implements unconstrained generics for the Sindarin compiler:
 *   - A global template registry (struct/function templates)
 *   - An instantiation cache (keyed by template name + concrete type args)
 *   - Type substitution (replace TYPE_OPAQUE type-params with concrete types)
 *   - Monomorphization (produce concrete StructDeclStmt / FunctionStmt)
 *   - Resolution of TYPE_GENERIC_INST to concrete TYPE_STRUCT
 */

#ifndef TYPE_CHECKER_GENERICS_H
#define TYPE_CHECKER_GENERICS_H

#include "ast.h"
#include "arena.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Template registries
 * ============================================================================ */

typedef struct
{
    const char    *name;   /* "Stack" */
    StructDeclStmt *decl;  /* original parsed StructDeclStmt with type_params */
} GenericTemplate;

typedef struct
{
    const char    *name;   /* "identity" */
    FunctionStmt  *decl;   /* original parsed FunctionStmt with type_params */
} GenericFunctionTemplate;

/* ============================================================================
 * Instantiation cache entries
 * ============================================================================ */

typedef struct
{
    const char    *template_name;       /* "Stack" */
    Type         **type_args;           /* [int_type] */
    int            type_arg_count;
    StructDeclStmt *instantiated_decl;  /* "Stack_int" struct, fully concrete */
    Type          *instantiated_type;   /* the corresponding TYPE_STRUCT */
} GenericInstantiation;

typedef struct
{
    const char   *template_name;        /* "identity" */
    Type        **type_args;
    int           type_arg_count;
    FunctionStmt *instantiated_decl;    /* concrete monomorphized function */
} GenericFunctionInstantiation;

/* ============================================================================
 * Struct template registry API
 * ============================================================================ */

/* Register a generic struct template (called when a struct with type_params is first seen). */
void generic_registry_register_template(const char *name, StructDeclStmt *decl);

/* Look up a struct template by name. Returns NULL if not found. */
GenericTemplate *generic_registry_find_template(const char *name);

/* Look up a cached struct instantiation. Returns NULL on cache miss. */
GenericInstantiation *generic_registry_find_instantiation(const char *template_name,
                                                           Type **type_args, int count);

/* Store a completed struct instantiation in the cache. */
void generic_registry_add_instantiation(const char *template_name,
                                         Type **type_args, int count,
                                         StructDeclStmt *decl, Type *type);

/* Iterate over all instantiated structs (used by codegen). */
int  generic_registry_instantiation_count(void);
GenericInstantiation *generic_registry_get_instantiation(int index);

/* ============================================================================
 * Function template registry API
 * ============================================================================ */

void generic_registry_register_function(const char *name, FunctionStmt *decl);
GenericFunctionTemplate *generic_registry_find_function_template(const char *name);

GenericFunctionInstantiation *generic_registry_find_function_instantiation(
    const char *template_name, Type **type_args, int count);
void generic_registry_add_function_instantiation(const char *template_name,
                                                   Type **type_args, int count,
                                                   FunctionStmt *decl);

int  generic_registry_function_instantiation_count(void);
GenericFunctionInstantiation *generic_registry_get_function_instantiation(int index);

/* ============================================================================
 * Reset all global state (called between compilations)
 * ============================================================================ */

void generic_registry_clear(void);

/* ============================================================================
 * Type substitution
 * ============================================================================ */

/* Deep-copy a type, substituting type parameters with concrete types.
 * param_names[i] → type_args[i] for TYPE_OPAQUE nodes whose name matches.
 * All other nodes are returned as-is (primitives) or recursively substituted. */
Type *substitute_type_params(Arena *arena, Type *type,
                              const char **param_names, Type **type_args, int count);

/* ============================================================================
 * Monomorphization
 * ============================================================================ */

/* Build a concrete, mangled name for an instantiation:
 *   Stack<int>       → "Stack_int"
 *   Pair<int,str>    → "Pair_int_str"
 * Caller must not free the returned string (it lives in the arena). */
const char *generic_build_monomorphized_name(Arena *arena, const char *template_name,
                                              Type **type_args, int type_arg_count);

/* Produce a concrete StructDeclStmt from a template + type args.
 * The result has type_param_count = 0 and all field/method signatures use concrete types.
 * The monomorphized struct is NOT yet registered in the symbol table by this function. */
StructDeclStmt *monomorphize_struct(Arena *arena, GenericTemplate *tmpl,
                                    Type **type_args, int type_arg_count);

/* High-level entry: monomorphize, register in the symbol table, cache, and return the
 * concrete TYPE_STRUCT.  If already cached, returns the cached type immediately. */
Type *monomorphize_struct_type(Arena *arena, GenericTemplate *tmpl,
                                Type **type_args, int type_arg_count,
                                SymbolTable *table);

/* Produce a concrete FunctionStmt from a generic function template + type args.
 * The result has type_param_count = 0 and all parameter/return types substituted. */
FunctionStmt *monomorphize_function(Arena *arena, GenericFunctionTemplate *tmpl,
                                    Type **type_args, int type_arg_count);

/* ============================================================================
 * TYPE_GENERIC_INST resolution
 * ============================================================================ */

/* Resolve a TYPE_GENERIC_INST to a concrete TYPE_STRUCT.
 * Looks up the template, monomorphizes if not cached, caches, and returns
 * the concrete type.  Also sets generic_inst->resolved on the input node.
 * Reports a type error and returns NULL on failure. */
Type *resolve_generic_instantiation(Arena *arena, Type *generic_inst, SymbolTable *table);

/* ============================================================================
 * Type-checking the monomorphized instantiation
 * ============================================================================ */

/* Type-check the body of a freshly monomorphized struct declaration.
 * Wraps the existing struct type-checker while preventing infinite recursion. */
void type_check_generic_instantiation_decl(Arena *arena, StructDeclStmt *decl,
                                            SymbolTable *table);

/* ============================================================================
 * Generic function type inference
 * ============================================================================ */

/* Attempt to infer type parameters from function arguments.
 * tmpl_func: the generic FunctionStmt template.
 * arg_types:  concrete types of the call-site arguments.
 * arg_count:  number of arguments.
 * inferred_args (output): array of size tmpl_func->type_param_count, filled by this function.
 * Returns true on success, false if inference failed (caller emits the error). */
bool infer_type_params_from_args(FunctionStmt *tmpl_func,
                                  Type **arg_types, int arg_count,
                                  Type **inferred_args);

/* ============================================================================
 * Body preparation for re-instantiation
 * ============================================================================ */

/* Clear the cached expr_type on every Expr node in a body statement array.
 * This MUST be called before type-checking a generic function body for each
 * new instantiation, because:
 *  - The body Stmt/Expr nodes are shared across all instantiations
 *  - type_check_expr() caches the result in expr->expr_type
 *  - Without clearing, the second instantiation (e.g., identity<str>) sees
 *    the cached type from the first (e.g., int) and produces a false type error.
 */
void clear_expr_types_in_stmts(Stmt **stmts, int count);

#endif /* TYPE_CHECKER_GENERICS_H */
