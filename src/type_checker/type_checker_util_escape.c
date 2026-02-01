#include "type_checker_util_escape.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

/* Forward declaration for recursive struct field checking */
static bool struct_has_only_primitives(Type *struct_type);

bool can_escape_private(Type *type)
{
    if (type == NULL) return false;

    /* Primitive types can always escape */
    if (is_primitive_type(type))
    {
        return true;
    }

    /* Struct types can escape only if they contain only primitives */
    if (type->kind == TYPE_STRUCT)
    {
        return struct_has_only_primitives(type);
    }

    /* All other types (arrays, strings, pointers, etc.) cannot escape */
    return false;
}

/* Helper function to check if a struct contains only primitive fields.
 * Recursively checks nested struct types.
 * Returns true if all fields are primitives or primitive-only structs.
 * Returns false if any field is a heap type (str, array, pointer, etc.). */
static bool struct_has_only_primitives(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return false;
    }

    /* Native structs may contain pointers and cannot escape private blocks */
    if (struct_type->as.struct_type.is_native)
    {
        DEBUG_VERBOSE("Struct '%s' is native (contains pointers) - cannot escape private",
                      struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
        return false;
    }

    /* Check each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;

        if (field_type == NULL)
        {
            return false;
        }

        /* Primitive fields are OK */
        if (is_primitive_type(field_type))
        {
            continue;
        }

        /* Nested struct fields - recursively check */
        if (field_type->kind == TYPE_STRUCT)
        {
            if (!struct_has_only_primitives(field_type))
            {
                DEBUG_VERBOSE("Struct field '%s' contains non-primitive nested struct - cannot escape private",
                              field->name ? field->name : "unknown");
                return false;
            }
            continue;
        }

        /* String fields - heap allocated, cannot escape */
        if (field_type->kind == TYPE_STRING)
        {
            DEBUG_VERBOSE("Struct field '%s' is str type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Array fields - heap allocated (dynamic) or contain heap data */
        if (field_type->kind == TYPE_ARRAY)
        {
            DEBUG_VERBOSE("Struct field '%s' is array type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Pointer fields - point to heap/external memory */
        if (field_type->kind == TYPE_POINTER)
        {
            DEBUG_VERBOSE("Struct field '%s' is pointer type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Function fields (closures) - contain heap references */
        if (field_type->kind == TYPE_FUNCTION)
        {
            DEBUG_VERBOSE("Struct field '%s' is function type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Opaque types - external C pointers */
        if (field_type->kind == TYPE_OPAQUE)
        {
            DEBUG_VERBOSE("Struct field '%s' is opaque type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Any type - may contain heap references */
        if (field_type->kind == TYPE_ANY)
        {
            DEBUG_VERBOSE("Struct field '%s' is any type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Unknown type kind - conservative default: cannot escape */
        DEBUG_VERBOSE("Struct field '%s' has unknown type kind %d - cannot escape private",
                      field->name ? field->name : "unknown", field_type->kind);
        return false;
    }

    /* All fields are primitives or primitive-only structs */
    DEBUG_VERBOSE("Struct '%s' contains only primitives - can escape private",
                  struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
    return true;
}

/* Helper to find and describe the first non-primitive field in a struct.
 * Returns a static buffer with the description, or NULL if all fields are primitives. */
static const char *find_blocking_struct_field(Type *struct_type)
{
    static char reason_buffer[256];

    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return NULL;
    }

    /* Native structs may contain pointers */
    if (struct_type->as.struct_type.is_native)
    {
        snprintf(reason_buffer, sizeof(reason_buffer),
                 "struct '%s' is a native struct (may contain pointers)",
                 struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
        return reason_buffer;
    }

    /* Check each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;
        const char *field_name = field->name ? field->name : "unknown";

        if (field_type == NULL) continue;

        if (is_primitive_type(field_type)) continue;

        /* Nested struct - recursively check */
        if (field_type->kind == TYPE_STRUCT)
        {
            const char *nested_reason = find_blocking_struct_field(field_type);
            if (nested_reason != NULL)
            {
                /* Copy nested_reason first since it points to the same static buffer
                 * we're about to write to */
                char nested_copy[256];
                strncpy(nested_copy, nested_reason, sizeof(nested_copy) - 1);
                nested_copy[sizeof(nested_copy) - 1] = '\0';

                snprintf(reason_buffer, sizeof(reason_buffer),
                         "field '%s' contains %s", field_name, nested_copy);
                return reason_buffer;
            }
            continue;
        }

        /* Describe the blocking field type */
        const char *type_desc = NULL;
        switch (field_type->kind)
        {
            case TYPE_STRING:
                type_desc = "str (heap-allocated string)";
                break;
            case TYPE_ARRAY:
                type_desc = "array (heap-allocated)";
                break;
            case TYPE_POINTER:
                type_desc = "pointer";
                break;
            case TYPE_FUNCTION:
                type_desc = "function (closure)";
                break;
            case TYPE_OPAQUE:
                type_desc = "opaque type";
                break;
            case TYPE_ANY:
                type_desc = "any type";
                break;
            default:
                type_desc = "non-primitive type";
                break;
        }

        snprintf(reason_buffer, sizeof(reason_buffer),
                 "field '%s' is %s", field_name, type_desc);
        return reason_buffer;
    }

    return NULL;
}

const char *get_private_escape_block_reason(Type *type)
{
    static char reason_buffer[512];

    if (type == NULL) return "unknown type";

    /* Primitive types can escape */
    if (is_primitive_type(type))
    {
        return NULL;
    }

    /* Struct types - check for heap fields */
    if (type->kind == TYPE_STRUCT)
    {
        const char *field_reason = find_blocking_struct_field(type);
        if (field_reason != NULL)
        {
            snprintf(reason_buffer, sizeof(reason_buffer),
                     "struct '%s' contains heap data: %s",
                     type->as.struct_type.name ? type->as.struct_type.name : "anonymous",
                     field_reason);
            return reason_buffer;
        }
        return NULL; /* Struct with only primitives can escape */
    }

    /* Non-struct, non-primitive types */
    switch (type->kind)
    {
        case TYPE_STRING:
            return "str type contains heap-allocated string data";
        case TYPE_ARRAY:
            return "array type is heap-allocated";
        case TYPE_POINTER:
            return "pointer type references external memory";
        case TYPE_FUNCTION:
            return "function type (closure) contains heap references";
        case TYPE_OPAQUE:
            return "opaque type references external C memory";
        case TYPE_ANY:
            return "any type may contain heap references";
        default:
            return "type cannot escape private block";
    }
}
