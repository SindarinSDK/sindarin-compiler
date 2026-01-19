#include "ast/ast_type.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Type *ast_clone_type(Arena *arena, Type *type)
{
    if (type == NULL)
        return NULL;

    Type *clone = arena_alloc(arena, sizeof(Type));
    if (clone == NULL)
    {
        DEBUG_ERROR("Out of memory when cloning type");
        exit(1);
    }
    memset(clone, 0, sizeof(Type));

    clone->kind = type->kind;

    switch (type->kind)
    {
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
    case TYPE_ANY:
        break;

    case TYPE_OPAQUE:
        if (type->as.opaque.name != NULL)
        {
            clone->as.opaque.name = arena_strdup(arena, type->as.opaque.name);
        }
        else
        {
            clone->as.opaque.name = NULL;
        }
        break;

    case TYPE_STRUCT:
        if (type->as.struct_type.name != NULL)
        {
            clone->as.struct_type.name = arena_strdup(arena, type->as.struct_type.name);
        }
        else
        {
            clone->as.struct_type.name = NULL;
        }
        clone->as.struct_type.field_count = type->as.struct_type.field_count;
        clone->as.struct_type.method_count = type->as.struct_type.method_count;
        clone->as.struct_type.size = type->as.struct_type.size;
        clone->as.struct_type.alignment = type->as.struct_type.alignment;
        clone->as.struct_type.is_native = type->as.struct_type.is_native;
        clone->as.struct_type.is_packed = type->as.struct_type.is_packed;
        clone->as.struct_type.pass_self_by_ref = type->as.struct_type.pass_self_by_ref;
        clone->as.struct_type.c_alias = type->as.struct_type.c_alias
            ? arena_strdup(arena, type->as.struct_type.c_alias) : NULL;
        if (type->as.struct_type.field_count > 0)
        {
            clone->as.struct_type.fields = arena_alloc(arena, sizeof(StructField) * type->as.struct_type.field_count);
            if (clone->as.struct_type.fields == NULL)
            {
                DEBUG_ERROR("Out of memory when cloning struct fields");
                exit(1);
            }
            for (int i = 0; i < type->as.struct_type.field_count; i++)
            {
                clone->as.struct_type.fields[i].name = type->as.struct_type.fields[i].name
                    ? arena_strdup(arena, type->as.struct_type.fields[i].name) : NULL;
                clone->as.struct_type.fields[i].type = ast_clone_type(arena, type->as.struct_type.fields[i].type);
                clone->as.struct_type.fields[i].offset = type->as.struct_type.fields[i].offset;
                clone->as.struct_type.fields[i].default_value = type->as.struct_type.fields[i].default_value;
                clone->as.struct_type.fields[i].c_alias = type->as.struct_type.fields[i].c_alias
                    ? arena_strdup(arena, type->as.struct_type.fields[i].c_alias) : NULL;
            }
        }
        else
        {
            clone->as.struct_type.fields = NULL;
        }
        /* Clone methods if any */
        if (type->as.struct_type.method_count > 0 && type->as.struct_type.methods != NULL)
        {
            clone->as.struct_type.methods = arena_alloc(arena, sizeof(StructMethod) * type->as.struct_type.method_count);
            if (clone->as.struct_type.methods == NULL)
            {
                DEBUG_ERROR("Out of memory when cloning struct methods");
                exit(1);
            }
            for (int i = 0; i < type->as.struct_type.method_count; i++)
            {
                StructMethod *src = &type->as.struct_type.methods[i];
                StructMethod *dst = &clone->as.struct_type.methods[i];
                dst->name = src->name ? arena_strdup(arena, src->name) : NULL;
                dst->param_count = src->param_count;
                dst->return_type = ast_clone_type(arena, src->return_type);
                dst->body = src->body;  /* Shallow copy - statements already in arena */
                dst->body_count = src->body_count;
                dst->modifier = src->modifier;
                dst->is_static = src->is_static;
                dst->is_native = src->is_native;
                dst->name_token = src->name_token;
                dst->c_alias = src->c_alias ? arena_strdup(arena, src->c_alias) : NULL;

                /* Clone parameters */
                if (src->param_count > 0 && src->params != NULL)
                {
                    dst->params = arena_alloc(arena, sizeof(Parameter) * src->param_count);
                    if (dst->params == NULL)
                    {
                        DEBUG_ERROR("Out of memory when cloning method params");
                        exit(1);
                    }
                    for (int j = 0; j < src->param_count; j++)
                    {
                        dst->params[j] = src->params[j];
                        if (src->params[j].name.start != NULL)
                        {
                            dst->params[j].name.start = arena_strndup(arena, src->params[j].name.start, src->params[j].name.length);
                        }
                        dst->params[j].type = ast_clone_type(arena, src->params[j].type);
                    }
                }
                else
                {
                    dst->params = NULL;
                }
            }
        }
        else
        {
            clone->as.struct_type.methods = NULL;
        }
        break;

    case TYPE_ARRAY:
        clone->as.array.element_type = ast_clone_type(arena, type->as.array.element_type);
        break;

    case TYPE_POINTER:
        clone->as.pointer.base_type = ast_clone_type(arena, type->as.pointer.base_type);
        break;

    case TYPE_FUNCTION:
        clone->as.function.return_type = ast_clone_type(arena, type->as.function.return_type);
        clone->as.function.param_count = type->as.function.param_count;
        clone->as.function.is_variadic = type->as.function.is_variadic;
        clone->as.function.is_native = type->as.function.is_native;
        clone->as.function.has_body = type->as.function.has_body;

        if (type->as.function.param_count > 0)
        {
            clone->as.function.param_types = arena_alloc(arena, sizeof(Type *) * type->as.function.param_count);
            if (clone->as.function.param_types == NULL)
            {
                DEBUG_ERROR("Out of memory when cloning function param types");
                exit(1);
            }
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                clone->as.function.param_types[i] = ast_clone_type(arena, type->as.function.param_types[i]);
            }

            /* Clone param_mem_quals if present */
            if (type->as.function.param_mem_quals != NULL)
            {
                clone->as.function.param_mem_quals = arena_alloc(arena, sizeof(MemoryQualifier) * type->as.function.param_count);
                if (clone->as.function.param_mem_quals == NULL)
                {
                    DEBUG_ERROR("Out of memory when cloning function param qualifiers");
                    exit(1);
                }
                for (int i = 0; i < type->as.function.param_count; i++)
                {
                    clone->as.function.param_mem_quals[i] = type->as.function.param_mem_quals[i];
                }
            }
            else
            {
                clone->as.function.param_mem_quals = NULL;
            }
        }
        else
        {
            clone->as.function.param_types = NULL;
            clone->as.function.param_mem_quals = NULL;
        }
        /* Copy typedef_name for native callback types */
        if (type->as.function.typedef_name != NULL)
        {
            clone->as.function.typedef_name = arena_strdup(arena, type->as.function.typedef_name);
        }
        else
        {
            clone->as.function.typedef_name = NULL;
        }
        break;
    }

    return clone;
}

Type *ast_create_primitive_type(Arena *arena, TypeKind kind)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *ast_create_array_type(Arena *arena, Type *element_type)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_ARRAY;
    type->as.array.element_type = element_type;
    return type;
}

Type *ast_create_pointer_type(Arena *arena, Type *base_type)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_POINTER;
    type->as.pointer.base_type = base_type;
    return type;
}

Type *ast_create_opaque_type(Arena *arena, const char *name)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_OPAQUE;
    type->as.opaque.name = name ? arena_strdup(arena, name) : NULL;
    return type;
}

Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_FUNCTION;

    type->as.function.return_type = ast_clone_type(arena, return_type);

    type->as.function.param_count = param_count;
    if (param_count > 0)
    {
        type->as.function.param_types = arena_alloc(arena, sizeof(Type *) * param_count);
        if (type->as.function.param_types == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        if (param_types == NULL && param_count > 0)
        {
            DEBUG_ERROR("Cannot create function type: param_types is NULL but param_count > 0");
            return NULL;
        }
        for (int i = 0; i < param_count; i++)
        {
            type->as.function.param_types[i] = ast_clone_type(arena, param_types[i]);
        }
    }
    else
    {
        type->as.function.param_types = NULL;
    }

    /* Initialize param_mem_quals to NULL - set separately if needed */
    type->as.function.param_mem_quals = NULL;

    return type;
}

int ast_type_equals(Type *a, Type *b)
{
    if (a == NULL && b == NULL)
        return 1;
    if (a == NULL || b == NULL)
        return 0;
    // TYPE_NIL compatibility:
    // - As top-level type (nil literal): only compatible with pointer types
    // - As array element type (empty array literal): compatible with any element type
    // To distinguish: if comparing element types (during recursive array comparison),
    // we want TYPE_NIL to match any type. If comparing top-level types, we restrict nil to pointers.
    // Since this function is recursive, we can't easily distinguish. Instead, handle this:
    // - If comparing primitive vs nil at top level, reject (caught in type_checker_stmt)
    // - For array element type comparison, TYPE_NIL should match any element type
    // Solution: Allow TYPE_NIL to match any type here; the specific check for
    // "nil can only be assigned to pointer types" is done in type_checker_stmt.c
    if (a->kind == TYPE_NIL || b->kind == TYPE_NIL)
        return 1;
    // Allow int literals to be assigned to byte variables (implicit narrowing)
    if ((a->kind == TYPE_BYTE && b->kind == TYPE_INT) ||
        (a->kind == TYPE_INT && b->kind == TYPE_BYTE))
        return 1;
    // Allow int literals to be assigned to interop integer types
    if ((a->kind == TYPE_INT32 && b->kind == TYPE_INT) ||
        (a->kind == TYPE_INT && b->kind == TYPE_INT32))
        return 1;
    if ((a->kind == TYPE_UINT && b->kind == TYPE_INT) ||
        (a->kind == TYPE_INT && b->kind == TYPE_UINT))
        return 1;
    if ((a->kind == TYPE_UINT32 && b->kind == TYPE_INT) ||
        (a->kind == TYPE_INT && b->kind == TYPE_UINT32))
        return 1;
    // Allow double literals to be assigned to float variables (implicit narrowing)
    if ((a->kind == TYPE_FLOAT && b->kind == TYPE_DOUBLE) ||
        (a->kind == TYPE_DOUBLE && b->kind == TYPE_FLOAT))
        return 1;
    if (a->kind != b->kind)
        return 0;

    switch (a->kind)
    {
    case TYPE_ARRAY:
        return ast_type_equals(a->as.array.element_type, b->as.array.element_type);
    case TYPE_POINTER:
        return ast_type_equals(a->as.pointer.base_type, b->as.pointer.base_type);
    case TYPE_FUNCTION:
        if (!ast_type_equals(a->as.function.return_type, b->as.function.return_type))
            return 0;
        if (a->as.function.param_count != b->as.function.param_count)
            return 0;
        for (int i = 0; i < a->as.function.param_count; i++)
        {
            if (!ast_type_equals(a->as.function.param_types[i], b->as.function.param_types[i]))
                return 0;
        }
        return 1;
    case TYPE_OPAQUE:
        /* Opaque types are equal if their names match */
        if (a->as.opaque.name == NULL && b->as.opaque.name == NULL)
            return 1;
        if (a->as.opaque.name == NULL || b->as.opaque.name == NULL)
            return 0;
        return strcmp(a->as.opaque.name, b->as.opaque.name) == 0;
    case TYPE_STRUCT:
        /* Struct types are equal if their names match */
        if (a->as.struct_type.name == NULL && b->as.struct_type.name == NULL)
            return 1;
        if (a->as.struct_type.name == NULL || b->as.struct_type.name == NULL)
            return 0;
        return strcmp(a->as.struct_type.name, b->as.struct_type.name) == 0;
    default:
        return 1;
    }
}

const char *ast_type_to_string(Arena *arena, Type *type)
{
    if (type == NULL)
    {
        return NULL;
    }

    switch (type->kind)
    {
    case TYPE_INT:
        return arena_strdup(arena, "int");
    case TYPE_INT32:
        return arena_strdup(arena, "int32");
    case TYPE_UINT:
        return arena_strdup(arena, "uint");
    case TYPE_UINT32:
        return arena_strdup(arena, "uint32");
    case TYPE_LONG:
        return arena_strdup(arena, "long");
    case TYPE_DOUBLE:
        return arena_strdup(arena, "double");
    case TYPE_FLOAT:
        return arena_strdup(arena, "float");
    case TYPE_CHAR:
        return arena_strdup(arena, "char");
    case TYPE_STRING:
        return arena_strdup(arena, "string");
    case TYPE_BOOL:
        return arena_strdup(arena, "bool");
    case TYPE_BYTE:
        return arena_strdup(arena, "byte");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "nil");
    case TYPE_ANY:
        return arena_strdup(arena, "any");

    case TYPE_ARRAY:
    {
        const char *elem_str = ast_type_to_string(arena, type->as.array.element_type);
        size_t len = strlen("array of ") + strlen(elem_str) + 1;
        char *str = arena_alloc(arena, len);
        if (str == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        snprintf(str, len, "array of %s", elem_str);
        return str;
    }

    case TYPE_POINTER:
    {
        const char *base_str = ast_type_to_string(arena, type->as.pointer.base_type);
        size_t len = 1 + strlen(base_str) + 1;  /* "*" + base_str + '\0' */
        char *str = arena_alloc(arena, len);
        if (str == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        snprintf(str, len, "*%s", base_str);
        return str;
    }

    case TYPE_FUNCTION:
    {
        size_t params_len = 0;
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            const char *param_str = ast_type_to_string(arena, type->as.function.param_types[i]);
            params_len += strlen(param_str);
            if (i < type->as.function.param_count - 1)
            {
                params_len += 2;
            }
        }

        char *params_buf = arena_alloc(arena, params_len + 1);
        if (params_buf == NULL && type->as.function.param_count > 0)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        char *ptr = params_buf;
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            const char *param_str = ast_type_to_string(arena, type->as.function.param_types[i]);
            strcpy(ptr, param_str);
            ptr += strlen(param_str);
            if (i < type->as.function.param_count - 1)
            {
                strcpy(ptr, ", ");
                ptr += 2;
            }
        }
        if (params_buf != NULL)
        {
            *ptr = '\0';
        }
        else
        {
            params_buf = arena_strdup(arena, "");
        }

        const char *ret_str = ast_type_to_string(arena, type->as.function.return_type);
        size_t total_len = strlen("function(") + strlen(params_buf) + strlen(") -> ") + strlen(ret_str) + 1;
        char *str = arena_alloc(arena, total_len);
        if (str == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        snprintf(str, total_len, "function(%s) -> %s", params_buf, ret_str);
        return str;
    }

    case TYPE_OPAQUE:
    {
        if (type->as.opaque.name != NULL)
        {
            return arena_strdup(arena, type->as.opaque.name);
        }
        return arena_strdup(arena, "opaque");
    }

    case TYPE_STRUCT:
    {
        if (type->as.struct_type.name != NULL)
        {
            return arena_strdup(arena, type->as.struct_type.name);
        }
        return arena_strdup(arena, "struct");
    }

    default:
        return arena_strdup(arena, "unknown");
    }
}

int ast_type_is_pointer(Type *type)
{
    return type != NULL && type->kind == TYPE_POINTER;
}

int ast_type_is_opaque(Type *type)
{
    return type != NULL && type->kind == TYPE_OPAQUE;
}

int ast_type_is_struct(Type *type)
{
    return type != NULL && type->kind == TYPE_STRUCT;
}

Type *ast_create_struct_type(Arena *arena, const char *name, StructField *fields, int field_count,
                             StructMethod *methods, int method_count, bool is_native, bool is_packed,
                             bool pass_self_by_ref, const char *c_alias)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_STRUCT;
    type->as.struct_type.name = name ? arena_strdup(arena, name) : NULL;
    type->as.struct_type.field_count = field_count;
    type->as.struct_type.method_count = method_count;
    type->as.struct_type.size = 0;       /* Computed during type checking */
    type->as.struct_type.alignment = 0;  /* Computed during type checking */
    type->as.struct_type.is_native = is_native;
    type->as.struct_type.is_packed = is_packed;
    type->as.struct_type.pass_self_by_ref = pass_self_by_ref;
    type->as.struct_type.c_alias = c_alias ? arena_strdup(arena, c_alias) : NULL;

    if (field_count > 0 && fields != NULL)
    {
        type->as.struct_type.fields = arena_alloc(arena, sizeof(StructField) * field_count);
        if (type->as.struct_type.fields == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < field_count; i++)
        {
            type->as.struct_type.fields[i].name = fields[i].name
                ? arena_strdup(arena, fields[i].name) : NULL;
            type->as.struct_type.fields[i].type = ast_clone_type(arena, fields[i].type);
            type->as.struct_type.fields[i].offset = fields[i].offset;
            type->as.struct_type.fields[i].default_value = fields[i].default_value;
            type->as.struct_type.fields[i].c_alias = fields[i].c_alias
                ? arena_strdup(arena, fields[i].c_alias) : NULL;
        }
    }
    else
    {
        type->as.struct_type.fields = NULL;
    }

    /* Copy methods if provided */
    if (method_count > 0 && methods != NULL)
    {
        type->as.struct_type.methods = arena_alloc(arena, sizeof(StructMethod) * method_count);
        if (type->as.struct_type.methods == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < method_count; i++)
        {
            type->as.struct_type.methods[i].name = methods[i].name
                ? arena_strdup(arena, methods[i].name) : NULL;
            type->as.struct_type.methods[i].param_count = methods[i].param_count;
            type->as.struct_type.methods[i].return_type = ast_clone_type(arena, methods[i].return_type);
            type->as.struct_type.methods[i].body = methods[i].body;  /* Shallow copy - statements already in arena */
            type->as.struct_type.methods[i].body_count = methods[i].body_count;
            type->as.struct_type.methods[i].modifier = methods[i].modifier;
            type->as.struct_type.methods[i].is_static = methods[i].is_static;
            type->as.struct_type.methods[i].is_native = methods[i].is_native;
            type->as.struct_type.methods[i].name_token = methods[i].name_token;
            type->as.struct_type.methods[i].c_alias = methods[i].c_alias
                ? arena_strdup(arena, methods[i].c_alias) : NULL;

            /* Copy parameters if any */
            if (methods[i].param_count > 0 && methods[i].params != NULL)
            {
                type->as.struct_type.methods[i].params = arena_alloc(arena, sizeof(Parameter) * methods[i].param_count);
                if (type->as.struct_type.methods[i].params == NULL)
                {
                    DEBUG_ERROR("Out of memory");
                    exit(1);
                }
                for (int j = 0; j < methods[i].param_count; j++)
                {
                    type->as.struct_type.methods[i].params[j] = methods[i].params[j];
                    if (methods[i].params[j].name.start != NULL)
                    {
                        type->as.struct_type.methods[i].params[j].name.start =
                            arena_strndup(arena, methods[i].params[j].name.start, methods[i].params[j].name.length);
                    }
                    type->as.struct_type.methods[i].params[j].type = ast_clone_type(arena, methods[i].params[j].type);
                }
            }
            else
            {
                type->as.struct_type.methods[i].params = NULL;
            }
        }
    }
    else
    {
        type->as.struct_type.methods = NULL;
    }

    return type;
}

StructField *ast_struct_get_field(Type *struct_type, const char *field_name)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT || field_name == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        if (struct_type->as.struct_type.fields[i].name != NULL &&
            strcmp(struct_type->as.struct_type.fields[i].name, field_name) == 0)
        {
            return &struct_type->as.struct_type.fields[i];
        }
    }

    return NULL;
}

int ast_struct_get_field_index(Type *struct_type, const char *field_name)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT || field_name == NULL)
    {
        return -1;
    }

    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        if (struct_type->as.struct_type.fields[i].name != NULL &&
            strcmp(struct_type->as.struct_type.fields[i].name, field_name) == 0)
        {
            return i;
        }
    }

    return -1;
}

StructMethod *ast_struct_get_method(Type *struct_type, const char *method_name)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT || method_name == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
    {
        if (struct_type->as.struct_type.methods[i].name != NULL &&
            strcmp(struct_type->as.struct_type.methods[i].name, method_name) == 0)
        {
            return &struct_type->as.struct_type.methods[i];
        }
    }

    return NULL;
}

bool ast_struct_literal_field_initialized(Expr *struct_literal_expr, int field_index)
{
    if (struct_literal_expr == NULL)
    {
        return false;
    }

    if (struct_literal_expr->type != EXPR_STRUCT_LITERAL)
    {
        return false;
    }

    if (struct_literal_expr->as.struct_literal.fields_initialized == NULL)
    {
        /* Not yet type-checked */
        return false;
    }

    if (field_index < 0 || field_index >= struct_literal_expr->as.struct_literal.total_field_count)
    {
        /* Invalid field index */
        return false;
    }

    return struct_literal_expr->as.struct_literal.fields_initialized[field_index];
}
