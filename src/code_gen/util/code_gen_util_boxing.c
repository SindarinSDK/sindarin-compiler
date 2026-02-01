#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>

/* ============================================================================
 * Any Type Boxing/Unboxing Helpers
 * ============================================================================ */

/**
 * Generate a consistent type ID for a struct type.
 * Uses a simple hash of the struct name to generate a unique integer.
 * This allows runtime type checking via `a is StructType` syntax.
 */
int get_struct_type_id(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return 0;
    }
    const char *name = struct_type->as.struct_type.name;
    if (name == NULL)
    {
        return 0;
    }
    /* Simple djb2 hash function */
    unsigned long hash = 5381;
    int c;
    while ((c = *name++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    /* Ensure positive int result */
    return (int)(hash & 0x7FFFFFFF);
}

const char *get_boxing_function(Type *type)
{
    DEBUG_VERBOSE("Entering get_boxing_function");
    if (type == NULL) return "rt_box_nil";

    switch (type->kind)
    {
    case TYPE_INT:
        return "rt_box_int";
    case TYPE_LONG:
        return "rt_box_long";
    case TYPE_INT32:
        return "rt_box_int32";
    case TYPE_UINT:
        return "rt_box_uint";
    case TYPE_UINT32:
        return "rt_box_uint32";
    case TYPE_DOUBLE:
        return "rt_box_double";
    case TYPE_FLOAT:
        return "rt_box_float";
    case TYPE_STRING:
        return "rt_box_string";
    case TYPE_CHAR:
        return "rt_box_char";
    case TYPE_BOOL:
        return "rt_box_bool";
    case TYPE_BYTE:
        return "rt_box_byte";
    case TYPE_ARRAY:
        return "rt_box_array";
    case TYPE_FUNCTION:
        return "rt_box_function";
    case TYPE_STRUCT:
        return "rt_box_struct";
    case TYPE_NIL:
    case TYPE_VOID:
        return "rt_box_nil";
    case TYPE_ANY:
        return NULL;  /* Already boxed */
    default:
        return "rt_box_nil";
    }
}

const char *get_unboxing_function(Type *type)
{
    DEBUG_VERBOSE("Entering get_unboxing_function");
    if (type == NULL) return NULL;

    switch (type->kind)
    {
    case TYPE_INT:
        return "rt_unbox_int";
    case TYPE_LONG:
        return "rt_unbox_long";
    case TYPE_INT32:
        return "rt_unbox_int32";
    case TYPE_UINT:
        return "rt_unbox_uint";
    case TYPE_UINT32:
        return "rt_unbox_uint32";
    case TYPE_DOUBLE:
        return "rt_unbox_double";
    case TYPE_FLOAT:
        return "rt_unbox_float";
    case TYPE_STRING:
        return "rt_unbox_string";
    case TYPE_CHAR:
        return "rt_unbox_char";
    case TYPE_BOOL:
        return "rt_unbox_bool";
    case TYPE_BYTE:
        return "rt_unbox_byte";
    case TYPE_ARRAY:
        return "rt_unbox_array";
    case TYPE_FUNCTION:
        return "rt_unbox_function";
    case TYPE_STRUCT:
        return "rt_unbox_struct";
    default:
        return NULL;
    }
}

/* Get the RtAnyTag constant for an element type (for boxing arrays) */
const char *get_element_type_tag(Type *element_type)
{
    if (element_type == NULL) return "RT_ANY_NIL";

    switch (element_type->kind)
    {
    case TYPE_INT:
        return "RT_ANY_INT";
    case TYPE_LONG:
        return "RT_ANY_LONG";
    case TYPE_INT32:
        return "RT_ANY_INT32";
    case TYPE_UINT:
        return "RT_ANY_UINT";
    case TYPE_UINT32:
        return "RT_ANY_UINT32";
    case TYPE_DOUBLE:
        return "RT_ANY_DOUBLE";
    case TYPE_FLOAT:
        return "RT_ANY_FLOAT";
    case TYPE_STRING:
        return "RT_ANY_STRING";
    case TYPE_CHAR:
        return "RT_ANY_CHAR";
    case TYPE_BOOL:
        return "RT_ANY_BOOL";
    case TYPE_BYTE:
        return "RT_ANY_BYTE";
    case TYPE_ARRAY:
        return "RT_ANY_ARRAY";
    case TYPE_STRUCT:
        return "RT_ANY_STRUCT";
    case TYPE_ANY:
        return "RT_ANY_NIL";  /* any[] - element types vary */
    default:
        return "RT_ANY_NIL";
    }
}

char *code_gen_box_value(CodeGen *gen, const char *value_str, Type *value_type)
{
    DEBUG_VERBOSE("Entering code_gen_box_value");

    if (value_type == NULL)
    {
        return arena_sprintf(gen->arena, "rt_box_nil()");
    }

    /* Already an any type - no boxing needed */
    if (value_type->kind == TYPE_ANY)
    {
        return arena_strdup(gen->arena, value_str);
    }

    const char *box_func = get_boxing_function(value_type);
    if (box_func == NULL)
    {
        return arena_strdup(gen->arena, value_str);
    }

    /* Arrays need the element type tag as second argument.
     * In handle mode, value_str is an RtHandle (uint32_t) - cast to void* for storage. */
    if (value_type->kind == TYPE_ARRAY)
    {
        const char *elem_tag = get_element_type_tag(value_type->as.array.element_type);
        if (gen->current_arena_var != NULL)
            return arena_sprintf(gen->arena, "%s((void *)(uintptr_t)%s, %s)", box_func, value_str, elem_tag);
        return arena_sprintf(gen->arena, "%s(%s, %s)", box_func, value_str, elem_tag);
    }

    /* Structs need arena, address, size, and type ID */
    if (value_type->kind == TYPE_STRUCT)
    {
        int type_id = get_struct_type_id(value_type);
        const char *struct_name = get_c_type(gen->arena, value_type);
        return arena_sprintf(gen->arena, "rt_box_struct(%s, &(%s), sizeof(%s), %d)",
                             ARENA_VAR(gen), value_str, struct_name, type_id);
    }

    return arena_sprintf(gen->arena, "%s(%s)", box_func, value_str);
}

char *code_gen_unbox_value(CodeGen *gen, const char *any_str, Type *target_type)
{
    DEBUG_VERBOSE("Entering code_gen_unbox_value");

    if (target_type == NULL)
    {
        return arena_strdup(gen->arena, any_str);
    }

    /* Target is any - no unboxing needed */
    if (target_type->kind == TYPE_ANY)
    {
        return arena_strdup(gen->arena, any_str);
    }

    const char *unbox_func = get_unboxing_function(target_type);
    if (unbox_func == NULL)
    {
        return arena_strdup(gen->arena, any_str);
    }

    /* Arrays need a cast after unboxing.
     * In handle mode, the stored value is an RtHandle (via void*) - cast back. */
    if (target_type->kind == TYPE_ARRAY)
    {
        if (gen->current_arena_var != NULL)
            return arena_sprintf(gen->arena, "(RtHandle)(uintptr_t)%s(%s)", unbox_func, any_str);
        const char *c_type = get_c_type(gen->arena, target_type);
        return arena_sprintf(gen->arena, "(%s)%s(%s)", c_type, unbox_func, any_str);
    }

    /* Structs need a cast and dereference (unbox returns void*) */
    if (target_type->kind == TYPE_STRUCT)
    {
        int type_id = get_struct_type_id(target_type);
        const char *struct_name = get_c_type(gen->arena, target_type);
        return arena_sprintf(gen->arena, "(*((%s *)rt_unbox_struct(%s, %d)))",
                             struct_name, any_str, type_id);
    }

    /* Strings in arena mode with handle context: unbox returns char*, wrap in handle.
     * When expr_as_handle is false (e.g. inside string interpolation), just return
     * the raw char* from rt_unbox_string directly. */
    if (target_type->kind == TYPE_STRING && gen->current_arena_var != NULL
        && gen->expr_as_handle)
    {
        return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s(%s))",
                             ARENA_VAR(gen), unbox_func, any_str);
    }

    return arena_sprintf(gen->arena, "%s(%s)", unbox_func, any_str);
}
