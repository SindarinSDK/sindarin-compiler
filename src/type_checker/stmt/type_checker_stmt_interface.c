#include "type_checker/stmt/type_checker_stmt_interface.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Interface Declaration Type Checking
 * ============================================================================ */

void type_check_interface_decl(Stmt *stmt, SymbolTable *table)
{
    (void)table; /* symbol table not needed for duplicate-name checks */

    InterfaceDeclStmt *decl = &stmt->as.interface_decl;

    DEBUG_VERBOSE("Type checking interface declaration: %.*s with %d methods",
                  decl->name.length, decl->name.start,
                  decl->method_count);

    /* Check for duplicate method names (O(n^2) linear scan) */
    for (int i = 0; i < decl->method_count; i++)
    {
        const char *name_i = decl->methods[i].name;
        if (name_i == NULL)
            continue;

        for (int j = i + 1; j < decl->method_count; j++)
        {
            const char *name_j = decl->methods[j].name;
            if (name_j == NULL)
                continue;

            if (strcmp(name_i, name_j) == 0)
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "interface '%.*s' has duplicate method '%s'",
                         decl->name.length, decl->name.start,
                         name_i);
                type_error(&decl->name, msg);
                /* Report each duplicate only once — continue checking remaining */
            }
        }
    }
}

/* ============================================================================
 * Structural Interface Satisfaction Check
 * ============================================================================ */

/* Helper: return a short human-readable label for a type without arena allocation.
 * For named types (struct, interface, opaque), returns the stored name.
 * For primitives, returns the keyword string. Falls back to "unknown". */
static const char *type_label(Type *type)
{
    if (type == NULL) return "unknown";
    switch (type->kind)
    {
    case TYPE_INT:       return "int";
    case TYPE_INT32:     return "int32";
    case TYPE_UINT:      return "uint";
    case TYPE_UINT32:    return "uint32";
    case TYPE_LONG:      return "long";
    case TYPE_DOUBLE:    return "double";
    case TYPE_FLOAT:     return "float";
    case TYPE_CHAR:      return "char";
    case TYPE_STRING:    return "str";
    case TYPE_BOOL:      return "bool";
    case TYPE_BYTE:      return "byte";
    case TYPE_VOID:      return "void";
    case TYPE_NIL:       return "nil";
    case TYPE_ARRAY:     return "array";
    case TYPE_FUNCTION:  return "function";
    case TYPE_POINTER:   return "pointer";
    case TYPE_STRUCT:
        return (type->as.struct_type.name != NULL) ? type->as.struct_type.name : "struct";
    case TYPE_INTERFACE:
        return (type->as.interface_type.name != NULL) ? type->as.interface_type.name : "interface";
    case TYPE_OPAQUE:
        return (type->as.opaque.name != NULL) ? type->as.opaque.name : "opaque";
    default:             return "unknown";
    }
}

/* Helper: compare two types for interface-satisfaction purposes.
 * When the interface side is TYPE_OPAQUE named "Self", treat it as
 * equivalent to the concrete struct type being checked (concrete_struct). */
static int iface_type_matches(Type *iface_side, Type *concrete_side,
                               Type *concrete_struct)
{
    if (iface_side == NULL && concrete_side == NULL)
        return 1;
    if (iface_side == NULL || concrete_side == NULL)
        return 0;

    /* Self substitution: interface TYPE_OPAQUE "Self" ≡ concrete struct */
    if (iface_side->kind == TYPE_OPAQUE &&
        iface_side->as.opaque.name != NULL &&
        strcmp(iface_side->as.opaque.name, "Self") == 0)
    {
        /* concrete_side must be the same struct type */
        if (concrete_side->kind != TYPE_STRUCT)
            return 0;
        if (concrete_struct == NULL || concrete_struct->kind != TYPE_STRUCT)
            return 0;
        if (concrete_struct->as.struct_type.name == NULL ||
            concrete_side->as.struct_type.name == NULL)
            return 0;
        return strcmp(concrete_struct->as.struct_type.name,
                      concrete_side->as.struct_type.name) == 0;
    }

    /* Fall back to standard type equality */
    return ast_type_equals(iface_side, concrete_side);
}

bool struct_satisfies_interface(Type *struct_type, Type *iface_type,
                                const char **missing_method,
                                const char **mismatch_reason)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
        return false;
    if (iface_type == NULL || iface_type->kind != TYPE_INTERFACE)
        return false;

    int iface_method_count = iface_type->as.interface_type.method_count;
    StructMethod *iface_methods = iface_type->as.interface_type.methods;

    for (int i = 0; i < iface_method_count; i++)
    {
        StructMethod *required = &iface_methods[i];
        if (required->name == NULL)
            continue;

        /* Search struct methods for a method with the same name */
        StructMethod *found = NULL;
        for (int j = 0; j < struct_type->as.struct_type.method_count; j++)
        {
            StructMethod *candidate = &struct_type->as.struct_type.methods[j];
            if (candidate->name != NULL &&
                strcmp(candidate->name, required->name) == 0)
            {
                found = candidate;
                break;
            }
        }

        if (found == NULL)
        {
            if (missing_method != NULL)
                *missing_method = required->name;
            return false;
        }

        /* Compare parameter count */
        if (found->param_count != required->param_count)
        {
            if (mismatch_reason != NULL)
            {
                static char reason_buf[256];
                snprintf(reason_buf, sizeof(reason_buf),
                         "method '%s' has %d parameter(s) but interface requires %d",
                         required->name, found->param_count, required->param_count);
                *mismatch_reason = reason_buf;
            }
            return false;
        }

        /* Compare parameter types */
        for (int k = 0; k < required->param_count; k++)
        {
            Type *iface_param = required->params[k].type;
            Type *struct_param = found->params[k].type;

            if (!iface_type_matches(iface_param, struct_param, struct_type))
            {
                if (mismatch_reason != NULL)
                {
                    static char reason_buf[256];
                    snprintf(reason_buf, sizeof(reason_buf),
                             "method '%s' parameter %d type mismatch (expected '%s', got '%s')",
                             required->name, k + 1,
                             type_label(iface_param), type_label(struct_param));
                    *mismatch_reason = reason_buf;
                }
                return false;
            }
        }

        /* Compare return type */
        if (!iface_type_matches(required->return_type, found->return_type, struct_type))
        {
            if (mismatch_reason != NULL)
            {
                static char reason_buf[256];
                snprintf(reason_buf, sizeof(reason_buf),
                         "method '%s' has wrong return type (expected '%s', got '%s')",
                         required->name,
                         type_label(required->return_type), type_label(found->return_type));
                *mismatch_reason = reason_buf;
            }
            return false;
        }
    }

    return true;
}
