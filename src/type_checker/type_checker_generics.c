/* type_checker_generics.c - Generic template registry and instantiation system
 *
 * Implements:
 *   - Global template / instantiation registries (static module-level state)
 *   - Type substitution engine
 *   - Struct monomorphization
 *   - TYPE_GENERIC_INST resolution
 *   - Generic function template support + type inference
 */

#include "type_checker/type_checker_generics.h"
#include "type_checker/stmt/type_checker_stmt_struct.h"
#include "type_checker/stmt/type_checker_stmt_interface.h"
#include "type_checker/util/type_checker_util.h"
#include "ast/ast_type.h"
#include "symbol_table.h"
#include "arena.h"
#include "debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Global registry state
 * ============================================================================ */

static GenericTemplate *g_templates        = NULL;
static int              g_template_count   = 0;
static int              g_template_capacity = 0;

static GenericInstantiation *g_instantiations        = NULL;
static int                   g_instantiation_count   = 0;
static int                   g_instantiation_capacity = 0;

static GenericFunctionTemplate *g_fn_templates        = NULL;
static int                      g_fn_template_count   = 0;
static int                      g_fn_template_capacity = 0;

static GenericFunctionInstantiation *g_fn_instantiations        = NULL;
static int                           g_fn_instantiation_count   = 0;
static int                           g_fn_instantiation_capacity = 0;

/* ============================================================================
 * Struct template registry
 * ============================================================================ */

void generic_registry_register_template(const char *name, StructDeclStmt *decl)
{
    if (name == NULL || decl == NULL)
        return;

    /* Avoid double-registration */
    for (int i = 0; i < g_template_count; i++)
    {
        if (strcmp(g_templates[i].name, name) == 0)
        {
            g_templates[i].decl = decl;
            return;
        }
    }

    if (g_template_count >= g_template_capacity)
    {
        int new_cap = g_template_capacity == 0 ? 8 : g_template_capacity * 2;
        GenericTemplate *new_arr = realloc(g_templates, sizeof(GenericTemplate) * new_cap);
        if (new_arr == NULL)
        {
            DEBUG_ERROR("Out of memory growing template registry");
            return;
        }
        g_templates        = new_arr;
        g_template_capacity = new_cap;
    }

    g_templates[g_template_count].name = name;
    g_templates[g_template_count].decl = decl;
    g_template_count++;

    DEBUG_VERBOSE("Registered generic template '%s' (%d type params)", name, decl->type_param_count);
}

GenericTemplate *generic_registry_find_template(const char *name)
{
    if (name == NULL)
        return NULL;
    for (int i = 0; i < g_template_count; i++)
    {
        if (strcmp(g_templates[i].name, name) == 0)
            return &g_templates[i];
    }
    return NULL;
}

GenericInstantiation *generic_registry_find_instantiation(const char *template_name,
                                                           Type **type_args, int count)
{
    if (template_name == NULL)
        return NULL;
    for (int i = 0; i < g_instantiation_count; i++)
    {
        GenericInstantiation *inst = &g_instantiations[i];
        if (strcmp(inst->template_name, template_name) != 0)
            continue;
        if (inst->type_arg_count != count)
            continue;
        bool match = true;
        for (int j = 0; j < count; j++)
        {
            if (!ast_type_equals(inst->type_args[j], type_args[j]))
            {
                match = false;
                break;
            }
        }
        if (match)
            return inst;
    }
    return NULL;
}

void generic_registry_add_instantiation(const char *template_name,
                                         Type **type_args, int count,
                                         StructDeclStmt *decl, Type *type)
{
    if (g_instantiation_count >= g_instantiation_capacity)
    {
        int new_cap = g_instantiation_capacity == 0 ? 8 : g_instantiation_capacity * 2;
        GenericInstantiation *new_arr = realloc(g_instantiations,
                                                sizeof(GenericInstantiation) * new_cap);
        if (new_arr == NULL)
        {
            DEBUG_ERROR("Out of memory growing instantiation cache");
            return;
        }
        g_instantiations         = new_arr;
        g_instantiation_capacity = new_cap;
    }

    GenericInstantiation *inst = &g_instantiations[g_instantiation_count++];
    inst->template_name      = template_name;
    inst->type_args          = type_args;
    inst->type_arg_count     = count;
    inst->instantiated_decl  = decl;
    inst->instantiated_type  = type;
}

int generic_registry_instantiation_count(void)
{
    return g_instantiation_count;
}

GenericInstantiation *generic_registry_get_instantiation(int index)
{
    if (index < 0 || index >= g_instantiation_count)
        return NULL;
    return &g_instantiations[index];
}

/* ============================================================================
 * Function template registry
 * ============================================================================ */

void generic_registry_register_function(const char *name, FunctionStmt *decl)
{
    if (name == NULL || decl == NULL)
        return;

    for (int i = 0; i < g_fn_template_count; i++)
    {
        if (strcmp(g_fn_templates[i].name, name) == 0)
        {
            g_fn_templates[i].decl = decl;
            return;
        }
    }

    if (g_fn_template_count >= g_fn_template_capacity)
    {
        int new_cap = g_fn_template_capacity == 0 ? 8 : g_fn_template_capacity * 2;
        GenericFunctionTemplate *new_arr = realloc(g_fn_templates,
                                                    sizeof(GenericFunctionTemplate) * new_cap);
        if (new_arr == NULL)
        {
            DEBUG_ERROR("Out of memory growing function template registry");
            return;
        }
        g_fn_templates         = new_arr;
        g_fn_template_capacity = new_cap;
    }

    g_fn_templates[g_fn_template_count].name = name;
    g_fn_templates[g_fn_template_count].decl = decl;
    g_fn_template_count++;

    DEBUG_VERBOSE("Registered generic function template '%s' (%d type params)", name, decl->type_param_count);
}

GenericFunctionTemplate *generic_registry_find_function_template(const char *name)
{
    if (name == NULL)
        return NULL;
    for (int i = 0; i < g_fn_template_count; i++)
    {
        if (strcmp(g_fn_templates[i].name, name) == 0)
            return &g_fn_templates[i];
    }
    return NULL;
}

GenericFunctionInstantiation *generic_registry_find_function_instantiation(
    const char *template_name, Type **type_args, int count)
{
    if (template_name == NULL)
        return NULL;
    for (int i = 0; i < g_fn_instantiation_count; i++)
    {
        GenericFunctionInstantiation *inst = &g_fn_instantiations[i];
        if (strcmp(inst->template_name, template_name) != 0)
            continue;
        if (inst->type_arg_count != count)
            continue;
        bool match = true;
        for (int j = 0; j < count; j++)
        {
            if (!ast_type_equals(inst->type_args[j], type_args[j]))
            {
                match = false;
                break;
            }
        }
        if (match)
            return inst;
    }
    return NULL;
}

void generic_registry_add_function_instantiation(const char *template_name,
                                                   Type **type_args, int count,
                                                   FunctionStmt *decl)
{
    if (g_fn_instantiation_count >= g_fn_instantiation_capacity)
    {
        int new_cap = g_fn_instantiation_capacity == 0 ? 8 : g_fn_instantiation_capacity * 2;
        GenericFunctionInstantiation *new_arr = realloc(g_fn_instantiations,
                                                         sizeof(GenericFunctionInstantiation) * new_cap);
        if (new_arr == NULL)
        {
            DEBUG_ERROR("Out of memory growing function instantiation cache");
            return;
        }
        g_fn_instantiations         = new_arr;
        g_fn_instantiation_capacity = new_cap;
    }

    GenericFunctionInstantiation *inst = &g_fn_instantiations[g_fn_instantiation_count++];
    inst->template_name      = template_name;
    inst->type_args          = type_args;
    inst->type_arg_count     = count;
    inst->instantiated_decl  = decl;
}

int generic_registry_function_instantiation_count(void)
{
    return g_fn_instantiation_count;
}

GenericFunctionInstantiation *generic_registry_get_function_instantiation(int index)
{
    if (index < 0 || index >= g_fn_instantiation_count)
        return NULL;
    return &g_fn_instantiations[index];
}

/* ============================================================================
 * Reset
 * ============================================================================ */

void generic_registry_clear(void)
{
    free(g_templates);
    g_templates         = NULL;
    g_template_count    = 0;
    g_template_capacity = 0;

    free(g_instantiations);
    g_instantiations         = NULL;
    g_instantiation_count    = 0;
    g_instantiation_capacity = 0;

    free(g_fn_templates);
    g_fn_templates         = NULL;
    g_fn_template_count    = 0;
    g_fn_template_capacity = 0;

    free(g_fn_instantiations);
    g_fn_instantiations         = NULL;
    g_fn_instantiation_count    = 0;
    g_fn_instantiation_capacity = 0;
}

/* ============================================================================
 * Type substitution
 * ============================================================================ */

Type *substitute_type_params(Arena *arena, Type *type,
                              const char **param_names, Type **type_args, int count)
{
    if (type == NULL)
        return NULL;

    switch (type->kind)
    {
    case TYPE_OPAQUE:
    {
        /* Check if this opaque name is one of the type parameters */
        const char *name = type->as.opaque.name;
        if (name != NULL)
        {
            for (int i = 0; i < count; i++)
            {
                if (param_names[i] != NULL && strcmp(name, param_names[i]) == 0)
                {
                    return type_args[i];
                }
            }
        }
        /* Not a type param — return as-is (could be a forward ref to another struct) */
        return type;
    }

    case TYPE_ARRAY:
    {
        Type *new_elem = substitute_type_params(arena, type->as.array.element_type,
                                                param_names, type_args, count);
        if (new_elem == type->as.array.element_type)
            return type; /* unchanged */
        return ast_create_array_type(arena, new_elem);
    }

    case TYPE_POINTER:
    {
        Type *new_base = substitute_type_params(arena, type->as.pointer.base_type,
                                                param_names, type_args, count);
        if (new_base == type->as.pointer.base_type)
            return type;
        return ast_create_pointer_type(arena, new_base);
    }

    case TYPE_FUNCTION:
    {
        bool changed = false;
        Type *new_ret = substitute_type_params(arena, type->as.function.return_type,
                                               param_names, type_args, count);
        if (new_ret != type->as.function.return_type)
            changed = true;

        Type **new_params = NULL;
        if (type->as.function.param_count > 0)
        {
            new_params = arena_alloc(arena, sizeof(Type *) * type->as.function.param_count);
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                new_params[i] = substitute_type_params(arena, type->as.function.param_types[i],
                                                        param_names, type_args, count);
                if (new_params[i] != type->as.function.param_types[i])
                    changed = true;
            }
        }

        if (!changed)
            return type;

        Type *result = ast_create_function_type(arena, new_ret, new_params,
                                                 type->as.function.param_count);
        result->as.function.is_variadic     = type->as.function.is_variadic;
        result->as.function.is_native       = type->as.function.is_native;
        result->as.function.has_body        = type->as.function.has_body;
        result->as.function.has_arena_param = type->as.function.has_arena_param;
        result->as.function.typedef_name    = type->as.function.typedef_name;
        return result;
    }

    case TYPE_GENERIC_INST:
    {
        /* Substitute within the type arguments */
        bool changed = false;
        int argc = type->as.generic_inst.type_arg_count;
        Type **new_args = NULL;
        if (argc > 0)
        {
            new_args = arena_alloc(arena, sizeof(Type *) * argc);
            for (int i = 0; i < argc; i++)
            {
                new_args[i] = substitute_type_params(arena, type->as.generic_inst.type_args[i],
                                                      param_names, type_args, count);
                if (new_args[i] != type->as.generic_inst.type_args[i])
                    changed = true;
            }
        }
        if (!changed)
            return type;
        /* Return a new TYPE_GENERIC_INST with substituted args — resolution happens later */
        return ast_create_generic_inst_type(arena, type->as.generic_inst.template_name,
                                             new_args, argc);
    }

    case TYPE_STRUCT:
    {
        /* Check if this is a type-parameter placeholder struct (zero-field forward ref).
         * The parser creates TYPE_STRUCT for unresolved names; for type params like T,
         * the struct has the name "T" and no fields. */
        if (type->as.struct_type.field_count == 0 &&
            type->as.struct_type.name != NULL)
        {
            for (int i = 0; i < count; i++)
            {
                if (param_names[i] != NULL &&
                    strcmp(type->as.struct_type.name, param_names[i]) == 0)
                {
                    return type_args[i];
                }
            }
        }
        return type;  /* concrete struct, no substitution */
    }

    /* Concrete / scalar types — no substitution needed */
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
    case TYPE_CHAR:
    case TYPE_STRING:
    case TYPE_BOOL:
    case TYPE_BYTE:
    case TYPE_VOID:
    case TYPE_NIL:
    case TYPE_INTERFACE:
    default:
        return type;
    }
}

/* ============================================================================
 * Monomorphized name building
 * ============================================================================ */

/* Return a mangling-friendly type name used in the monomorphized C identifier.
 * Sindarin `str` is TYPE_STRING but we mangle it as "str" (not "string") so that
 * Stack<str> → Stack_str rather than Stack_string. */
static const char *mangle_type_name(Arena *arena, Type *type)
{
    if (type == NULL)
        return "unknown";
    /* TYPE_STRING maps to "str" in Sindarin source and should appear as "str" in the mangled name */
    if (type->kind == TYPE_STRING)
        return "str";
    /* For TYPE_STRUCT use the struct name directly */
    if (type->kind == TYPE_STRUCT && type->as.struct_type.name != NULL)
        return type->as.struct_type.name;
    /* Fall back to ast_type_to_string for all other types */
    const char *s = ast_type_to_string(arena, type);
    return s != NULL ? s : "unknown";
}

const char *generic_build_monomorphized_name(Arena *arena, const char *template_name,
                                              Type **type_args, int type_arg_count)
{
    /* Start with the template name */
    size_t total = strlen(template_name) + 1; /* +1 for NUL */

    /* Add "_<typestr>" for each arg */
    const char **arg_strs = arena_alloc(arena, sizeof(const char *) * (type_arg_count > 0 ? type_arg_count : 1));
    for (int i = 0; i < type_arg_count; i++)
    {
        arg_strs[i] = mangle_type_name(arena, type_args[i]);
        total += 1 + strlen(arg_strs[i]); /* "_" + type string */
    }

    char *name = arena_alloc(arena, total);
    if (name == NULL)
    {
        DEBUG_ERROR("Out of memory building monomorphized name");
        return template_name;
    }

    strcpy(name, template_name);
    for (int i = 0; i < type_arg_count; i++)
    {
        strcat(name, "_");
        strcat(name, arg_strs[i]);
    }

    DEBUG_VERBOSE("Built monomorphized name: '%s'", name);
    return name;
}

/* ============================================================================
 * Struct monomorphization
 * ============================================================================ */

StructDeclStmt *monomorphize_struct(Arena *arena, GenericTemplate *tmpl,
                                    Type **type_args, int type_arg_count)
{
    StructDeclStmt *src = tmpl->decl;
    const char **param_names = src->type_params;
    int          param_count = src->type_param_count;

    /* Build the monomorphized name */
    const char *mono_name = generic_build_monomorphized_name(arena, tmpl->name,
                                                              type_args, type_arg_count);

    /* Allocate the new StructDeclStmt */
    StructDeclStmt *mono = arena_alloc(arena, sizeof(StructDeclStmt));
    if (mono == NULL)
    {
        DEBUG_ERROR("Out of memory allocating monomorphized struct");
        return NULL;
    }
    memset(mono, 0, sizeof(StructDeclStmt));

    /* Name token — point into arena-duped string */
    char *mono_name_dup = arena_strdup(arena, mono_name);
    mono->name.start    = mono_name_dup;
    mono->name.length   = (int)strlen(mono_name_dup);
    mono->name.line     = src->name.line;
    mono->name.filename = src->name.filename;
    mono->name.type     = src->name.type;

    /* Copy structural flags */
    mono->is_native        = src->is_native;
    mono->is_packed        = src->is_packed;
    mono->pass_self_by_ref = src->pass_self_by_ref;
    mono->is_serializable  = src->is_serializable;
    mono->c_alias          = NULL; /* monomorphized structs don't have C aliases */

    /* Concrete — no type parameters */
    mono->type_params      = NULL;
    mono->type_param_count = 0;

    /* Deep-copy and substitute fields */
    if (src->field_count > 0)
    {
        mono->fields = arena_alloc(arena, sizeof(StructField) * src->field_count);
        if (mono->fields == NULL)
        {
            DEBUG_ERROR("Out of memory allocating monomorphized struct fields");
            return NULL;
        }
        for (int i = 0; i < src->field_count; i++)
        {
            StructField *sf  = &src->fields[i];
            StructField *mf  = &mono->fields[i];

            mf->name          = sf->name; /* string literal pointer — OK */
            mf->type          = substitute_type_params(arena, sf->type,
                                                        param_names, type_args, param_count);
            mf->offset        = 0;        /* will be recalculated after type-checking */
            mf->default_value = sf->default_value; /* shallow — no type substitution in exprs */
            mf->c_alias       = sf->c_alias;
        }
        mono->field_count = src->field_count;
    }

    /* Deep-copy and substitute method signatures (NOT bodies — those get re-checked) */
    if (src->method_count > 0)
    {
        mono->methods = arena_alloc(arena, sizeof(StructMethod) * src->method_count);
        if (mono->methods == NULL)
        {
            DEBUG_ERROR("Out of memory allocating monomorphized struct methods");
            return NULL;
        }
        for (int i = 0; i < src->method_count; i++)
        {
            StructMethod *sm = &src->methods[i];
            StructMethod *mm = &mono->methods[i];

            mm->name          = sm->name;
            mm->return_type   = substitute_type_params(arena, sm->return_type,
                                                        param_names, type_args, param_count);
            mm->return_mem_qualifier = sm->return_mem_qualifier;
            mm->body          = sm->body;       /* shared — same body AST nodes */
            mm->body_count    = sm->body_count;
            mm->modifier      = sm->modifier;
            mm->is_static     = sm->is_static;
            mm->is_native     = sm->is_native;
            mm->has_arena_param = sm->has_arena_param;
            mm->name_token    = sm->name_token;
            mm->c_alias       = sm->c_alias;
            mm->is_operator   = sm->is_operator;
            mm->operator_token = sm->operator_token;

            /* Deep-copy and substitute parameters */
            mm->param_count = sm->param_count;
            if (sm->param_count > 0 && sm->params != NULL)
            {
                mm->params = arena_alloc(arena, sizeof(Parameter) * sm->param_count);
                if (mm->params == NULL)
                {
                    DEBUG_ERROR("Out of memory allocating monomorphized method params");
                    return NULL;
                }
                for (int j = 0; j < sm->param_count; j++)
                {
                    mm->params[j] = sm->params[j]; /* copy name token + qualifiers */
                    mm->params[j].type = substitute_type_params(arena, sm->params[j].type,
                                                                  param_names, type_args, param_count);
                }
            }
            else
            {
                mm->params = NULL;
            }
        }
        mono->method_count = src->method_count;
    }

    DEBUG_VERBOSE("Monomorphized struct '%s' from template '%s'", mono_name, tmpl->name);
    return mono;
}

/* ============================================================================
 * Constraint checking — primitives auto-satisfy standard interfaces
 * ============================================================================ */

/* Check whether a primitive type auto-satisfies a built-in interface.
 * Primitives don't have method declarations, but the compiler treats them
 * as satisfying Comparable, Hashable, and Stringable. */
static bool primitive_satisfies_interface(Type *type, Type *iface)
{
    if (type == NULL || iface == NULL || iface->kind != TYPE_INTERFACE)
        return false;
    if (iface->as.interface_type.name == NULL)
        return false;

    const char *iface_name = iface->as.interface_type.name;
    TypeKind k = type->kind;

    /* Numeric types: int, int32, uint, uint32, long, double, float, char, byte */
    bool is_numeric = (k == TYPE_INT || k == TYPE_INT32 || k == TYPE_UINT ||
                       k == TYPE_UINT32 || k == TYPE_LONG || k == TYPE_DOUBLE ||
                       k == TYPE_FLOAT || k == TYPE_CHAR || k == TYPE_BYTE);

    if (is_numeric)
    {
        if (strcmp(iface_name, "Comparable") == 0) return true;
        if (strcmp(iface_name, "Hashable") == 0)   return true;
        if (strcmp(iface_name, "Stringable") == 0) return true;
    }

    /* str: Comparable, Hashable, Stringable */
    if (k == TYPE_STRING)
    {
        if (strcmp(iface_name, "Comparable") == 0) return true;
        if (strcmp(iface_name, "Hashable") == 0)   return true;
        if (strcmp(iface_name, "Stringable") == 0) return true;
    }

    /* bool: Hashable, Stringable */
    if (k == TYPE_BOOL)
    {
        if (strcmp(iface_name, "Hashable") == 0)   return true;
        if (strcmp(iface_name, "Stringable") == 0) return true;
    }

    return false;
}

/* Check whether a concrete type satisfies an interface constraint.
 * For struct types, uses the structural satisfaction check.
 * For primitive types, checks the auto-derivable set. */
static bool type_satisfies_constraint(Type *concrete_type, Type *constraint,
                                       const char **missing_method,
                                       const char **mismatch_reason)
{
    if (concrete_type == NULL || constraint == NULL)
        return false;

    /* Primitives auto-satisfy built-in interfaces */
    if (concrete_type->kind != TYPE_STRUCT && concrete_type->kind != TYPE_INTERFACE)
        return primitive_satisfies_interface(concrete_type, constraint);

    /* Struct: structural satisfaction check */
    if (concrete_type->kind == TYPE_STRUCT)
        return struct_satisfies_interface(concrete_type, constraint, missing_method, mismatch_reason);

    return false;
}

/* Validate that all type arguments satisfy their declared constraints.
 * Returns true if all constraints are satisfied, false otherwise (with error reported).
 * template_name: used for error messages.
 * type_params: parameter names array.
 * type_param_constraints: constraint arrays per type param (may be NULL).
 * type_param_constraint_counts: count array (may be NULL).
 * type_args: concrete type arguments.
 * type_arg_count: number of type arguments. */
bool check_type_param_constraints(const char *template_name,
                                          const char **type_params,
                                          Type ***type_param_constraints,
                                          int *type_param_constraint_counts,
                                          Type **type_args, int type_arg_count)
{
    if (type_param_constraints == NULL || type_param_constraint_counts == NULL)
        return true; /* no constraints declared */

    for (int i = 0; i < type_arg_count; i++)
    {
        int con_count = type_param_constraint_counts[i];
        if (con_count == 0 || type_param_constraints[i] == NULL)
            continue;

        for (int c = 0; c < con_count; c++)
        {
            Type *constraint = type_param_constraints[i][c];
            if (constraint == NULL) continue;

            const char *missing_method = NULL;
            const char *mismatch_reason = NULL;

            if (!type_satisfies_constraint(type_args[i], constraint,
                                            &missing_method, &mismatch_reason))
            {
                Token err_tok;
                err_tok.start    = template_name;
                err_tok.length   = (int)strlen(template_name);
                err_tok.type     = TOKEN_IDENTIFIER;
                err_tok.line     = 0;
                err_tok.filename = NULL;

                const char *type_name = "unknown";
                if (type_args[i] != NULL)
                {
                    switch (type_args[i]->kind)
                    {
                    case TYPE_STRUCT:  type_name = type_args[i]->as.struct_type.name ? type_args[i]->as.struct_type.name : "struct"; break;
                    case TYPE_INT:     type_name = "int"; break;
                    case TYPE_INT32:   type_name = "int32"; break;
                    case TYPE_UINT:    type_name = "uint"; break;
                    case TYPE_UINT32:  type_name = "uint32"; break;
                    case TYPE_LONG:    type_name = "long"; break;
                    case TYPE_DOUBLE:  type_name = "double"; break;
                    case TYPE_FLOAT:   type_name = "float"; break;
                    case TYPE_CHAR:    type_name = "char"; break;
                    case TYPE_STRING:  type_name = "str"; break;
                    case TYPE_BOOL:    type_name = "bool"; break;
                    case TYPE_BYTE:    type_name = "byte"; break;
                    default:           break;
                    }
                }

                const char *iface_name = constraint->as.interface_type.name
                    ? constraint->as.interface_type.name : "interface";

                char msg[512];
                if (missing_method != NULL)
                {
                    snprintf(msg, sizeof(msg),
                             "type '%s' does not satisfy interface '%s' required by type parameter '%s' of '%s': missing method '%s'",
                             type_name, iface_name, type_params[i], template_name, missing_method);
                }
                else if (mismatch_reason != NULL)
                {
                    snprintf(msg, sizeof(msg),
                             "type '%s' does not satisfy interface '%s' required by type parameter '%s' of '%s': %s",
                             type_name, iface_name, type_params[i], template_name, mismatch_reason);
                }
                else
                {
                    snprintf(msg, sizeof(msg),
                             "type '%s' does not satisfy interface '%s' required by type parameter '%s' of '%s'",
                             type_name, iface_name, type_params[i], template_name);
                }
                type_error(&err_tok, msg);
                return false;
            }
        }
    }

    return true;
}

/* ============================================================================
 * TYPE_GENERIC_INST resolution
 * ============================================================================ */

Type *resolve_generic_instantiation(Arena *arena, Type *generic_inst, SymbolTable *table)
{
    if (generic_inst == NULL || generic_inst->kind != TYPE_GENERIC_INST)
        return generic_inst;

    /* Already resolved? */
    if (generic_inst->as.generic_inst.resolved != NULL)
        return generic_inst->as.generic_inst.resolved;

    const char *template_name  = generic_inst->as.generic_inst.template_name;
    Type      **type_args      = generic_inst->as.generic_inst.type_args;
    int         type_arg_count = generic_inst->as.generic_inst.type_arg_count;


    /* Resolve type arguments into a LOCAL copy to avoid mutating shared AST nodes.
     * Handles TYPE_GENERIC_INST (recursive), TYPE_OPAQUE (symbol table aliases),
     * and TYPE_STRUCT forward references (parser creates T as TYPE_STRUCT{name="T"}). */
    Type **resolved_args = type_args;
    for (int i = 0; i < type_arg_count; i++)
    {
        Type *resolved = type_args[i];
        if (resolved == NULL) continue;

        if (resolved->kind == TYPE_GENERIC_INST)
        {
            resolved = resolve_generic_instantiation(arena, resolved, table);
            if (resolved == NULL)
                return NULL; /* error already reported */
        }
        else if (resolved->kind == TYPE_OPAQUE && resolved->as.opaque.name != NULL)
        {
            Token tok = { .start = resolved->as.opaque.name,
                          .length = (int)strlen(resolved->as.opaque.name),
                          .type = TOKEN_IDENTIFIER, .line = 0, .filename = NULL };
            Symbol *alias = symbol_table_lookup_type(table, tok);
            if (alias != NULL && alias->type != NULL && alias->type->kind != TYPE_OPAQUE)
                resolved = alias->type;
        }
        else if (resolved->kind == TYPE_STRUCT &&
                 resolved->as.struct_type.field_count == 0 &&
                 resolved->as.struct_type.fields == NULL &&
                 resolved->as.struct_type.name != NULL)
        {
            Token tok = { .start = resolved->as.struct_type.name,
                          .length = (int)strlen(resolved->as.struct_type.name),
                          .type = TOKEN_IDENTIFIER, .line = 0, .filename = NULL };
            Symbol *alias = symbol_table_lookup_type(table, tok);
            if (alias != NULL && alias->type != NULL && alias->type != resolved)
                resolved = alias->type;
        }

        if (resolved != type_args[i])
        {
            if (resolved_args == type_args)
            {
                /* First modification — copy the array */
                resolved_args = arena_alloc(arena, sizeof(Type *) * type_arg_count);
                for (int j = 0; j < type_arg_count; j++)
                    resolved_args[j] = type_args[j];
            }
            resolved_args[i] = resolved;
        }
    }
    type_args = resolved_args;

    /* Check instantiation cache */
    GenericInstantiation *cached = generic_registry_find_instantiation(template_name,
                                                                         type_args, type_arg_count);
    if (cached != NULL)
    {
        generic_inst->as.generic_inst.resolved = cached->instantiated_type;
        return cached->instantiated_type;
    }

    /* Find the template */
    GenericTemplate *tmpl = generic_registry_find_template(template_name);
    if (tmpl == NULL)
    {
        /* Maybe it's already a concrete struct (no type params)? Look in symbol table. */
        Token name_tok;
        name_tok.start    = template_name;
        name_tok.length   = (int)strlen(template_name);
        name_tok.type     = TOKEN_IDENTIFIER;
        name_tok.line     = 0;
        name_tok.filename = NULL;

        Symbol *sym = symbol_table_lookup_type(table, name_tok);
        if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "'%s' is not a generic template", template_name);
            /* Use a dummy token for the error; callers should ideally pass a token */
            type_error(&name_tok, msg);
            return NULL;
        }
        else
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "unknown generic template '%s'", template_name);
            type_error(&name_tok, msg);
            return NULL;
        }
    }

    /* Check type argument count matches */
    if (tmpl->decl->type_param_count != type_arg_count)
    {
        Token name_tok;
        name_tok.start    = template_name;
        name_tok.length   = (int)strlen(template_name);
        name_tok.type     = TOKEN_IDENTIFIER;
        name_tok.line     = 0;
        name_tok.filename = NULL;

        char msg[256];
        snprintf(msg, sizeof(msg),
                 "generic template '%s' expects %d type arguments, got %d",
                 template_name, tmpl->decl->type_param_count, type_arg_count);
        type_error(&name_tok, msg);
        return NULL;
    }

    /* Check type parameter constraints */
    if (!check_type_param_constraints(template_name,
                                       (const char **)tmpl->decl->type_params,
                                       tmpl->decl->type_param_constraints,
                                       tmpl->decl->type_param_constraint_counts,
                                       type_args, type_arg_count))
    {
        return NULL; /* error already reported */
    }

    /* Monomorphize */
    Type *result = monomorphize_struct_type(arena, tmpl, type_args, type_arg_count, table);
    if (result != NULL)
    {
        generic_inst->as.generic_inst.resolved = result;
    }
    return result;
}

/* ============================================================================
 * Forward declarations for static helpers
 * ============================================================================ */

static void register_type_param_aliases(Arena *arena, GenericTemplate *tmpl,
                                         Type **type_args, int type_arg_count,
                                         SymbolTable *table);

/* ============================================================================
 * High-level monomorphization entry (struct + type registration + caching)
 * ============================================================================ */

Type *monomorphize_struct_type(Arena *arena, GenericTemplate *tmpl,
                                Type **type_args, int type_arg_count,
                                SymbolTable *table)
{
    const char *template_name = tmpl->name;

    /* Check cache first */
    GenericInstantiation *cached = generic_registry_find_instantiation(template_name,
                                                                         type_args, type_arg_count);
    if (cached != NULL)
        return cached->instantiated_type;

    /* Monomorphize the struct */
    StructDeclStmt *mono_decl = monomorphize_struct(arena, tmpl, type_args, type_arg_count);
    if (mono_decl == NULL)
        return NULL;

    const char *mono_name = mono_decl->name.start;

    /* Create the TYPE_STRUCT for the monomorphized struct */
    Type *mono_type = ast_create_struct_type(arena, mono_name,
                                              mono_decl->fields, mono_decl->field_count,
                                              mono_decl->methods, mono_decl->method_count,
                                              mono_decl->is_native, mono_decl->is_packed,
                                              mono_decl->pass_self_by_ref, mono_decl->c_alias);
    mono_type->as.struct_type.is_serializable = mono_decl->is_serializable;

    /* Register in the symbol table under the monomorphized name */
    Token mono_token;
    mono_token.start    = mono_name;
    mono_token.length   = (int)strlen(mono_name);
    mono_token.type     = TOKEN_IDENTIFIER;
    mono_token.line     = tmpl->decl->name.line;
    mono_token.filename = tmpl->decl->name.filename;

    symbol_table_add_type(table, mono_token, mono_type);

    /* Cache the instantiation.
     * We need to keep a heap copy of type_args for the cache because the caller's
     * array may be on the stack.  Use the arena for the copy. */
    Type **args_copy = NULL;
    if (type_arg_count > 0)
    {
        args_copy = arena_alloc(arena, sizeof(Type *) * type_arg_count);
        if (args_copy != NULL)
        {
            for (int i = 0; i < type_arg_count; i++)
                args_copy[i] = type_args[i];
        }
    }

    generic_registry_add_instantiation(template_name, args_copy, type_arg_count,
                                        mono_decl, mono_type);

    /* Register type param → concrete type aliases so method bodies that explicitly
     * reference the type parameter name can resolve it during type-checking. */
    register_type_param_aliases(arena, tmpl, args_copy != NULL ? args_copy : type_args,
                                 type_arg_count, table);

    /* Type-check the monomorphized struct (validates method bodies + field types) */
    type_check_generic_instantiation_decl(arena, mono_decl, table);

    /* Sync resolved method signatures from mono_decl back to mono_type.
     * type_check_struct_decl resolves forward references in method return types
     * and parameter types, but mono_type's methods were copied before type-checking. */
    for (int i = 0; i < mono_type->as.struct_type.method_count && i < mono_decl->method_count; i++)
    {
        mono_type->as.struct_type.methods[i].return_type = mono_decl->methods[i].return_type;
        for (int j = 0; j < mono_decl->methods[i].param_count; j++)
        {
            mono_type->as.struct_type.methods[i].params[j].type =
                mono_decl->methods[i].params[j].type;
        }
    }

    DEBUG_VERBOSE("Completed monomorphization: '%s'", mono_name);
    return mono_type;
}

/* ============================================================================
 * Type-check a monomorphized struct declaration
 * ============================================================================ */

void type_check_generic_instantiation_decl(Arena *arena, StructDeclStmt *decl,
                                            SymbolTable *table)
{
    (void)arena; /* used implicitly through table->arena */

    /* Clear any cached expr_type values from previous instantiations of the same
     * generic template.  Method bodies are shared AST pointers (mm->body = sm->body),
     * so cached types from an earlier instantiation (e.g., Stack<int>) must be
     * wiped before type-checking a new instantiation (e.g., Stack<str>). */
    for (int i = 0; i < decl->method_count; i++)
    {
        StructMethod *m = &decl->methods[i];
        if (m->body != NULL && m->body_count > 0)
            clear_expr_types_in_stmts(m->body, m->body_count);
    }

    /* Build a synthetic Stmt wrapper so we can call the existing type checker */
    Stmt wrapper;
    memset(&wrapper, 0, sizeof(Stmt));
    wrapper.type           = STMT_STRUCT_DECL;
    wrapper.token          = &decl->name;
    wrapper.as.struct_decl = *decl;

    /* The struct type-checker already handles everything including layout calculation.
     * Note: type_param_count is 0 on the monomorphized decl so the early-return guard
     * for generic templates will NOT trigger — normal type-checking proceeds. */
    type_check_struct_decl(&wrapper, table);
}

/* Helper called by monomorphize_struct_type to register type param → concrete type
 * mappings as type aliases before type-checking the instantiated body.
 * This allows method bodies that explicitly reference T to resolve correctly. */
static void register_type_param_aliases(Arena *arena, GenericTemplate *tmpl,
                                         Type **type_args, int type_arg_count,
                                         SymbolTable *table)
{
    (void)arena;
    StructDeclStmt *src = tmpl->decl;
    for (int i = 0; i < type_arg_count && i < src->type_param_count; i++)
    {
        Token tp_tok;
        tp_tok.start    = src->type_params[i];
        tp_tok.length   = (int)strlen(src->type_params[i]);
        tp_tok.type     = TOKEN_IDENTIFIER;
        tp_tok.line     = 0;
        tp_tok.filename = NULL;
        symbol_table_add_type(table, tp_tok, type_args[i]);
    }
}

/* ============================================================================
 * Generic function monomorphization
 * ============================================================================ */

FunctionStmt *monomorphize_function(Arena *arena, GenericFunctionTemplate *tmpl,
                                    Type **type_args, int type_arg_count)
{
    FunctionStmt *src = tmpl->decl;
    const char  **param_names = src->type_params;
    int           param_count = src->type_param_count;

    /* Build monomorphized name */
    const char *mono_name = generic_build_monomorphized_name(arena, tmpl->name,
                                                              type_args, type_arg_count);

    /* Allocate the new FunctionStmt */
    FunctionStmt *mono = arena_alloc(arena, sizeof(FunctionStmt));
    if (mono == NULL)
    {
        DEBUG_ERROR("Out of memory allocating monomorphized function");
        return NULL;
    }
    memset(mono, 0, sizeof(FunctionStmt));

    /* Name token */
    char *mono_name_dup    = arena_strdup(arena, mono_name);
    mono->name.start       = mono_name_dup;
    mono->name.length      = (int)strlen(mono_name_dup);
    mono->name.line        = src->name.line;
    mono->name.filename    = src->name.filename;
    mono->name.type        = src->name.type;

    /* Copy flags */
    mono->is_native         = src->is_native;
    mono->is_variadic       = src->is_variadic;
    mono->has_arena_param   = src->has_arena_param;
    mono->c_alias           = NULL;  /* monomorphized functions don't use alias */
    mono->modifier          = src->modifier;
    mono->type_params       = NULL;
    mono->type_param_count  = 0;
    mono->body              = src->body;       /* shared body AST */
    mono->body_count        = src->body_count;

    /* Substitute return type */
    mono->return_type = substitute_type_params(arena, src->return_type,
                                                param_names, type_args, param_count);
    mono->return_mem_qualifier = src->return_mem_qualifier;

    /* Deep-copy and substitute parameters */
    mono->param_count = src->param_count;
    if (src->param_count > 0 && src->params != NULL)
    {
        mono->params = arena_alloc(arena, sizeof(Parameter) * src->param_count);
        if (mono->params == NULL)
        {
            DEBUG_ERROR("Out of memory allocating monomorphized function params");
            return NULL;
        }
        for (int i = 0; i < src->param_count; i++)
        {
            mono->params[i] = src->params[i];
            mono->params[i].type = substitute_type_params(arena, src->params[i].type,
                                                            param_names, type_args, param_count);
        }
    }

    DEBUG_VERBOSE("Monomorphized function '%s' from template '%s'", mono_name, tmpl->name);
    return mono;
}

/* ============================================================================
 * Generic function type inference
 * ============================================================================ */

bool infer_type_params_from_args(FunctionStmt *tmpl_func,
                                  Type **arg_types, int arg_count,
                                  Type **inferred_args)
{
    int tp_count = tmpl_func->type_param_count;

    /* Initialize output to NULL */
    for (int i = 0; i < tp_count; i++)
        inferred_args[i] = NULL;

    /* Walk each parameter; if the parameter type is a TYPE_OPAQUE matching a type param,
     * unify it with the corresponding argument type. */
    int param_count = tmpl_func->param_count;
    if (arg_count < param_count)
        param_count = arg_count; /* don't read past end of arg_types */

    for (int p = 0; p < param_count; p++)
    {
        Type *param_type = tmpl_func->params[p].type;
        Type *arg_type   = arg_types[p];

        if (param_type == NULL || arg_type == NULL)
            continue;

        if (param_type->kind == TYPE_OPAQUE && param_type->as.opaque.name != NULL)
        {
            const char *pname = param_type->as.opaque.name;
            for (int t = 0; t < tp_count; t++)
            {
                if (tmpl_func->type_params[t] != NULL &&
                    strcmp(pname, tmpl_func->type_params[t]) == 0)
                {
                    if (inferred_args[t] == NULL)
                    {
                        inferred_args[t] = arg_type;
                    }
                    else if (!ast_type_equals(inferred_args[t], arg_type))
                    {
                        /* Conflicting inference */
                        return false;
                    }
                    break;
                }
            }
        }

        /* Also handle TYPE_STRUCT forward-ref placeholders (same as TYPE_OPAQUE) */
        if (param_type->kind == TYPE_STRUCT &&
            param_type->as.struct_type.field_count == 0 &&
            param_type->as.struct_type.name != NULL)
        {
            for (int i = 0; i < tp_count; i++)
            {
                if (strcmp(param_type->as.struct_type.name, tmpl_func->type_params[i]) == 0)
                {
                    if (inferred_args[i] == NULL)
                        inferred_args[i] = arg_type;
                    break;
                }
            }
        }

        /* Also handle TYPE_ARRAY parameters: if element type is a placeholder, infer from arg array */
        if (param_type->kind == TYPE_ARRAY &&
            param_type->as.array.element_type != NULL &&
            arg_type != NULL && arg_type->kind == TYPE_ARRAY)
        {
            Type *elem_param = param_type->as.array.element_type;
            Type *elem_arg   = arg_type->as.array.element_type;

            /* Check if element is TYPE_OPAQUE or TYPE_STRUCT placeholder */
            const char *elem_name = NULL;
            if (elem_param->kind == TYPE_OPAQUE)
                elem_name = elem_param->as.opaque.name;
            else if (elem_param->kind == TYPE_STRUCT &&
                     elem_param->as.struct_type.field_count == 0)
                elem_name = elem_param->as.struct_type.name;

            if (elem_name != NULL)
            {
                for (int i = 0; i < tp_count; i++)
                {
                    if (strcmp(elem_name, tmpl_func->type_params[i]) == 0)
                    {
                        if (inferred_args[i] == NULL)
                            inferred_args[i] = elem_arg;
                        break;
                    }
                }
            }
        }

        /* Also handle TYPE_FUNCTION parameter types — when a function parameter is fn(T): U,
         * infer T and U from the corresponding lambda argument */
        if (param_type->kind == TYPE_FUNCTION &&
            arg_type != NULL && arg_type->kind == TYPE_FUNCTION)
        {
            /* Infer each param type in the function type */
            int min_params = param_type->as.function.param_count;
            if (arg_type->as.function.param_count < min_params)
                min_params = arg_type->as.function.param_count;

            for (int q = 0; q < min_params; q++)
            {
                Type *fp = param_type->as.function.param_types[q];
                Type *fa = arg_type->as.function.param_types[q];
                if (fp == NULL || fa == NULL) continue;

                const char *fp_name = NULL;
                if (fp->kind == TYPE_OPAQUE) fp_name = fp->as.opaque.name;
                else if (fp->kind == TYPE_STRUCT && fp->as.struct_type.field_count == 0)
                    fp_name = fp->as.struct_type.name;

                if (fp_name != NULL)
                {
                    for (int i = 0; i < tp_count; i++)
                    {
                        if (strcmp(fp_name, tmpl_func->type_params[i]) == 0)
                        {
                            if (inferred_args[i] == NULL) inferred_args[i] = fa;
                            break;
                        }
                    }
                }
            }

            /* Also infer return type */
            Type *rp = param_type->as.function.return_type;
            Type *ra = arg_type->as.function.return_type;
            if (rp != NULL && ra != NULL)
            {
                const char *rp_name = NULL;
                if (rp->kind == TYPE_OPAQUE) rp_name = rp->as.opaque.name;
                else if (rp->kind == TYPE_STRUCT && rp->as.struct_type.field_count == 0)
                    rp_name = rp->as.struct_type.name;

                if (rp_name != NULL)
                {
                    for (int i = 0; i < tp_count; i++)
                    {
                        if (strcmp(rp_name, tmpl_func->type_params[i]) == 0)
                        {
                            if (inferred_args[i] == NULL) inferred_args[i] = ra;
                            break;
                        }
                    }
                }
            }
        }
    }

    /* Check all type params were inferred */
    for (int i = 0; i < tp_count; i++)
    {
        if (inferred_args[i] == NULL)
            return false;
    }

    return true;
}

/* ============================================================================
 * Clear cached expr_type from all Expr/Stmt nodes in a function body.
 * This is needed because generic function bodies are SHARED between all
 * instantiations of the same template.  type_check_expr() stores its result
 * in expr->expr_type, so a second instantiation (e.g., identity<str>) would
 * see the cached result from the first (identity<int>) and produce spurious
 * type errors.  Clearing before each instantiation forces a fresh type-check.
 * ============================================================================ */

static void clear_expr_types_in_expr(Expr *e);

static void clear_expr_types_in_stmts_impl(Stmt **stmts, int count)
{
    if (stmts == NULL) return;
    for (int i = 0; i < count; i++)
    {
        Stmt *s = stmts[i];
        if (s == NULL) continue;
        switch (s->type)
        {
        case STMT_EXPR:
            clear_expr_types_in_expr(s->as.expression.expression);
            break;
        case STMT_VAR_DECL:
            clear_expr_types_in_expr(s->as.var_decl.initializer);
            /* Reset resolved type for generic bodies (T -> concrete mapping) */
            s->as.var_decl.resolved_type = NULL;
            break;
        case STMT_RETURN:
            clear_expr_types_in_expr(s->as.return_stmt.value);
            break;
        case STMT_BLOCK:
            clear_expr_types_in_stmts_impl(s->as.block.statements, s->as.block.count);
            break;
        case STMT_IF:
            clear_expr_types_in_expr(s->as.if_stmt.condition);
            clear_expr_types_in_stmts_impl(&s->as.if_stmt.then_branch, 1);
            if (s->as.if_stmt.else_branch != NULL)
                clear_expr_types_in_stmts_impl(&s->as.if_stmt.else_branch, 1);
            break;
        case STMT_WHILE:
            clear_expr_types_in_expr(s->as.while_stmt.condition);
            clear_expr_types_in_stmts_impl(&s->as.while_stmt.body, 1);
            break;
        case STMT_FOR:
            if (s->as.for_stmt.initializer != NULL)
                clear_expr_types_in_stmts_impl(&s->as.for_stmt.initializer, 1);
            clear_expr_types_in_expr(s->as.for_stmt.condition);
            clear_expr_types_in_expr(s->as.for_stmt.increment);
            clear_expr_types_in_stmts_impl(&s->as.for_stmt.body, 1);
            break;
        case STMT_FOR_EACH:
            clear_expr_types_in_expr(s->as.for_each_stmt.iterable);
            clear_expr_types_in_stmts_impl(&s->as.for_each_stmt.body, 1);
            break;
        case STMT_FUNCTION:
            /* Nested function — recurse into its body */
            clear_expr_types_in_stmts_impl(s->as.function.body, s->as.function.body_count);
            break;
        case STMT_LOCK:
            clear_expr_types_in_expr(s->as.lock_stmt.lock_expr);
            clear_expr_types_in_stmts_impl(&s->as.lock_stmt.body, 1);
            break;
        case STMT_USING:
            clear_expr_types_in_expr(s->as.using_stmt.initializer);
            clear_expr_types_in_stmts_impl(&s->as.using_stmt.body, 1);
            break;
        default:
            break;
        }
    }
}

static void clear_expr_types_in_expr(Expr *e)
{
    if (e == NULL) return;
    e->expr_type = NULL;

    switch (e->type)
    {
    case EXPR_BINARY:
        clear_expr_types_in_expr(e->as.binary.left);
        clear_expr_types_in_expr(e->as.binary.right);
        break;
    case EXPR_UNARY:
        clear_expr_types_in_expr(e->as.unary.operand);
        break;
    case EXPR_ASSIGN:
        clear_expr_types_in_expr(e->as.assign.value);
        break;
    case EXPR_INDEX_ASSIGN:
        clear_expr_types_in_expr(e->as.index_assign.array);
        clear_expr_types_in_expr(e->as.index_assign.index);
        clear_expr_types_in_expr(e->as.index_assign.value);
        break;
    case EXPR_CALL:
        clear_expr_types_in_expr(e->as.call.callee);
        for (int i = 0; i < e->as.call.arg_count; i++)
            clear_expr_types_in_expr(e->as.call.arguments[i]);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < e->as.array.element_count; i++)
            clear_expr_types_in_expr(e->as.array.elements[i]);
        break;
    case EXPR_ARRAY_ACCESS:
        clear_expr_types_in_expr(e->as.array_access.array);
        clear_expr_types_in_expr(e->as.array_access.index);
        break;
    case EXPR_ARRAY_SLICE:
        clear_expr_types_in_expr(e->as.array_slice.array);
        clear_expr_types_in_expr(e->as.array_slice.start);
        clear_expr_types_in_expr(e->as.array_slice.end);
        clear_expr_types_in_expr(e->as.array_slice.step);
        break;
    case EXPR_RANGE:
        clear_expr_types_in_expr(e->as.range.start);
        clear_expr_types_in_expr(e->as.range.end);
        break;
    case EXPR_SPREAD:
        clear_expr_types_in_expr(e->as.spread.array);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < e->as.interpol.part_count; i++)
            clear_expr_types_in_expr(e->as.interpol.parts[i]);
        break;
    case EXPR_MEMBER:
        clear_expr_types_in_expr(e->as.member.object);
        e->as.member.resolved_method    = NULL;
        e->as.member.resolved_struct_type = NULL;
        e->as.member.resolved_namespace = NULL;
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < e->as.static_call.arg_count; i++)
            clear_expr_types_in_expr(e->as.static_call.arguments[i]);
        e->as.static_call.resolved_method      = NULL;
        e->as.static_call.resolved_struct_type = NULL;
        break;
    case EXPR_SIZED_ARRAY_ALLOC:
        clear_expr_types_in_expr(e->as.sized_array_alloc.size_expr);
        clear_expr_types_in_expr(e->as.sized_array_alloc.default_value);
        break;
    case EXPR_THREAD_SPAWN:
        clear_expr_types_in_expr(e->as.thread_spawn.call);
        break;
    case EXPR_THREAD_SYNC:
        clear_expr_types_in_expr(e->as.thread_sync.handle);
        break;
    case EXPR_THREAD_DETACH:
        clear_expr_types_in_expr(e->as.thread_detach.handle);
        break;
    case EXPR_SYNC_LIST:
        for (int i = 0; i < e->as.sync_list.element_count; i++)
            clear_expr_types_in_expr(e->as.sync_list.elements[i]);
        break;
    case EXPR_ADDRESS_OF:
        clear_expr_types_in_expr(e->as.address_of.operand);
        break;
    case EXPR_VALUE_OF:
        clear_expr_types_in_expr(e->as.value_of.operand);
        break;
    case EXPR_COPY_OF:
        clear_expr_types_in_expr(e->as.copy_of.operand);
        break;
    case EXPR_STRUCT_LITERAL:
        for (int i = 0; i < e->as.struct_literal.field_count; i++)
            clear_expr_types_in_expr(e->as.struct_literal.fields[i].value);
        e->as.struct_literal.struct_type = NULL;
        /* Clear resolved generic instantiation so re-resolution uses the current
         * type param aliases (shared bodies are re-checked per instantiation) */
        if (e->as.struct_literal.type_annotation != NULL &&
            e->as.struct_literal.type_annotation->kind == TYPE_GENERIC_INST)
            e->as.struct_literal.type_annotation->as.generic_inst.resolved = NULL;
        break;
    case EXPR_MEMBER_ACCESS:
        clear_expr_types_in_expr(e->as.member_access.object);
        break;
    case EXPR_MEMBER_ASSIGN:
        clear_expr_types_in_expr(e->as.member_assign.object);
        clear_expr_types_in_expr(e->as.member_assign.value);
        break;
    case EXPR_SIZEOF:
        clear_expr_types_in_expr(e->as.sizeof_expr.expr_operand);
        break;
    case EXPR_TYPEOF:
        clear_expr_types_in_expr(e->as.typeof_expr.operand);
        break;
    case EXPR_COMPOUND_ASSIGN:
        clear_expr_types_in_expr(e->as.compound_assign.target);
        clear_expr_types_in_expr(e->as.compound_assign.value);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        clear_expr_types_in_expr(e->as.operand);
        break;
    case EXPR_LAMBDA:
        /* Lambda body is also shared — clear it too */
        clear_expr_types_in_expr(e->as.lambda.body);
        clear_expr_types_in_stmts_impl(e->as.lambda.body_stmts, e->as.lambda.body_stmt_count);
        break;
    case EXPR_METHOD_CALL:
        clear_expr_types_in_expr(e->as.method_call.object);
        for (int i = 0; i < e->as.method_call.arg_count; i++)
            clear_expr_types_in_expr(e->as.method_call.args[i]);
        e->as.method_call.method      = NULL;
        e->as.method_call.struct_type = NULL;
        break;
    case EXPR_MATCH:
        clear_expr_types_in_expr(e->as.match_expr.subject);
        for (int i = 0; i < e->as.match_expr.arm_count; i++)
        {
            MatchArm *arm = &e->as.match_expr.arms[i];
            for (int j = 0; j < arm->pattern_count; j++)
                clear_expr_types_in_expr(arm->patterns[j]);
            if (arm->body != NULL)
                clear_expr_types_in_stmts_impl(&arm->body, 1);
        }
        break;
    case EXPR_LITERAL:
    case EXPR_VARIABLE:
    default:
        break;
    }
}

void clear_expr_types_in_stmts(Stmt **stmts, int count)
{
    clear_expr_types_in_stmts_impl(stmts, count);
}
