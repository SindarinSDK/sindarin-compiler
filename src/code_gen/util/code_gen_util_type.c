#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Type *resolve_struct_type(CodeGen *gen, Type *type)
{
    /* Resolve a struct type that might be an unresolved forward reference.
     * Forward references lack c_alias because they were created before the
     * complete struct definition was parsed. Look up the complete type
     * from the symbol table to get the correct c_alias and other metadata. */
    if (type == NULL || type->kind != TYPE_STRUCT)
    {
        return type;
    }
    if (type->as.struct_type.c_alias != NULL)
    {
        /* Already has c_alias, no need to resolve */
        return type;
    }
    if (type->as.struct_type.name == NULL)
    {
        return type;
    }

    Token lookup_tok;
    lookup_tok.start = type->as.struct_type.name;
    lookup_tok.length = strlen(type->as.struct_type.name);
    Symbol *sym = symbol_table_lookup_type(gen->symbol_table, lookup_tok);
    if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
    {
        return sym->type;
    }
    return type;
}

const char *get_c_type(Arena *arena, Type *type)
{
    DEBUG_VERBOSE("Entering get_c_type");

    if (type == NULL)
    {
        return arena_strdup(arena, "void");
    }

    switch (type->kind)
    {
    case TYPE_INT:
        return arena_strdup(arena, "long long");
    case TYPE_LONG:
        return arena_strdup(arena, "long long");
    case TYPE_INT32:
        return arena_strdup(arena, "int32_t");
    case TYPE_UINT:
        return arena_strdup(arena, "uint64_t");
    case TYPE_UINT32:
        return arena_strdup(arena, "uint32_t");
    case TYPE_DOUBLE:
        return arena_strdup(arena, "double");
    case TYPE_FLOAT:
        return arena_strdup(arena, "float");
    case TYPE_CHAR:
        return arena_strdup(arena, "char");
    case TYPE_STRING:
        return arena_strdup(arena, "RtHandleV2 *");
    case TYPE_BOOL:
        return arena_strdup(arena, "bool");
    case TYPE_BYTE:
        return arena_strdup(arena, "unsigned char");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "void *");
    case TYPE_ANY:
        return arena_strdup(arena, "RtAny");
    case TYPE_ARRAY:
        return arena_strdup(arena, "RtHandleV2 *");
    case TYPE_POINTER:
    {
        /* For pointer types: *T becomes T* in C */
        const char *base_c_type = get_c_type(arena, type->as.pointer.base_type);
        size_t len = strlen(base_c_type) + 2;  /* base_type + "*" + '\0' */
        char *result = arena_alloc(arena, len);
        snprintf(result, len, "%s*", base_c_type);
        return result;
    }
    case TYPE_FUNCTION:
    {
        /* Native callback types with a typedef name use that name */
        if (type->as.function.is_native && type->as.function.typedef_name != NULL)
        {
            return arena_strdup(arena, type->as.function.typedef_name);
        }
        /* Regular function values are represented as closures */
        return arena_strdup(arena, "__Closure__ *");
    }
    case TYPE_OPAQUE:
    {
        /* Opaque types use their name directly (e.g., FILE) */
        if (type->as.opaque.name != NULL)
        {
            return arena_strdup(arena, type->as.opaque.name);
        }
        return arena_strdup(arena, "void");  /* Fallback for unnamed opaque types */
    }
    case TYPE_STRUCT:
    {
        /* Struct types use c_alias if available, otherwise their Sindarin name */
        if (type->as.struct_type.c_alias != NULL)
        {
            /* Native structs with c_alias are treated as opaque handle types.
             * Generate as pointer type (like built-in TextFile, etc.) */
            if (type->as.struct_type.is_native)
            {
                return arena_sprintf(arena, "%s *", type->as.struct_type.c_alias);
            }
            return arena_strdup(arena, type->as.struct_type.c_alias);
        }
        if (type->as.struct_type.name != NULL)
        {
            return sn_mangle_name(arena, type->as.struct_type.name);
        }
        return arena_strdup(arena, "void");  /* Fallback for unnamed struct types */
    }
    default:
        fprintf(stderr, "Error: Unknown type kind %d\n", type->kind);
        exit(1);
    }

    return NULL;
}

bool is_handle_type(Type *type)
{
    if (type == NULL) return false;
    return (type->kind == TYPE_STRING || type->kind == TYPE_ARRAY);
}

const char *get_c_param_type(Arena *arena, Type *type)
{
    /* After handle migration, function parameters use the same types as variables:
     * TYPE_STRING and TYPE_ARRAY are both RtHandle. */
    return get_c_type(arena, type);
}

const char *get_c_native_param_type(Arena *arena, Type *type)
{
    /* Native C functions expect raw pointer types, not RtHandle.
     * TYPE_STRING → const char *, TYPE_ARRAY → element_type * */
    if (type == NULL) return arena_strdup(arena, "void *");

    if (type->kind == TYPE_STRING)
    {
        return arena_strdup(arena, "const char *");
    }
    if (type->kind == TYPE_ARRAY)
    {
        const char *elem_c = get_c_array_elem_type(arena, type->as.array.element_type);
        return arena_sprintf(arena, "%s *", elem_c);
    }
    /* All other types use the standard C type */
    return get_c_type(arena, type);
}

const char *get_c_array_elem_type(Arena *arena, Type *elem_type)
{
    /* Returns the C storage type for elements within an array.
     * This is what you get after pinning the array and casting to pointer.
     * Special case: bool is stored as int in arrays for alignment. */
    if (elem_type == NULL) return arena_strdup(arena, "void");

    if (elem_type->kind == TYPE_BOOL)
    {
        return arena_strdup(arena, "int");
    }
    /* String and array elements are stored as RtHandle values in the array data */
    return get_c_type(arena, elem_type);
}

const char *get_c_sizeof_elem(Arena *arena, Type *elem_type)
{
    /* Returns sizeof(elem_type) expression for generic array operations.
     * This wraps the C type in sizeof() for use in codegen. */
    const char *c_type = get_c_array_elem_type(arena, elem_type);
    return arena_sprintf(arena, "sizeof(%s)", c_type);
}

const char *get_array_accessor_suffix(Type *elem_type)
{
    if (elem_type == NULL) return "long"; /* default */
    switch (elem_type->kind) {
        case TYPE_INT:
        case TYPE_LONG: return "long";
        case TYPE_INT32: return "int32";
        case TYPE_UINT: return "uint";
        case TYPE_UINT32: return "uint32";
        case TYPE_DOUBLE: return "double";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_BOOL: return "bool";
        case TYPE_BYTE: return "byte";
        case TYPE_STRING:
        case TYPE_ARRAY: return "handle";
        default: return NULL; /* struct/complex: use rt_array_data_begin_v2 */
    }
}

char *get_var_name(Arena *arena, Token name)
{
    DEBUG_VERBOSE("Entering get_var_name");
    char *var_name = arena_alloc(arena, name.length + 1);
    if (var_name == NULL)
    {
        exit(1);
    }
    strncpy(var_name, name.start, name.length);
    var_name[name.length] = '\0';
    return var_name;
}

char *sn_mangle_name(Arena *arena, const char *name)
{
    return arena_sprintf(arena, "__sn__%s", name);
}

void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...)
{
    for (int i = 0; i < indent; i++)
    {
        fprintf(gen->output, "    ");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
}

char *code_gen_binary_op_str(SnTokenType op)
{
    DEBUG_VERBOSE("Entering code_gen_binary_op_str");
    switch (op)
    {
    case TOKEN_PLUS:
        return "add";
    case TOKEN_MINUS:
        return "sub";
    case TOKEN_STAR:
        return "mul";
    case TOKEN_SLASH:
        return "div";
    case TOKEN_MODULO:
        return "mod";
    case TOKEN_EQUAL_EQUAL:
        return "eq";
    case TOKEN_BANG_EQUAL:
        return "ne";
    case TOKEN_LESS:
        return "lt";
    case TOKEN_LESS_EQUAL:
        return "le";
    case TOKEN_GREATER:
        return "gt";
    case TOKEN_GREATER_EQUAL:
        return "ge";
    default:
        return NULL;
    }
}

char *code_gen_type_suffix(Type *type)
{
    DEBUG_VERBOSE("Entering code_gen_type_suffix");
    if (type == NULL)
    {
        return "void";
    }
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "long";
    case TYPE_INT32:
        return "int32";
    case TYPE_UINT:
        return "uint";
    case TYPE_UINT32:
        return "uint32";
    case TYPE_CHAR:
        return "char";
    case TYPE_BYTE:
        return "byte";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_FLOAT:
        return "float";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    default:
        return "void";
    }
}
