#include "type_checker_util_struct.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

bool is_c_compatible_type(Type *type)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
        /* Primitive types - all C compatible */
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
        case TYPE_CHAR:
        case TYPE_BYTE:
        case TYPE_BOOL:
        case TYPE_VOID:
        /* Interop types - explicitly C compatible */
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_UINT:
            return true;

        /* Pointer types - C compatible */
        case TYPE_POINTER:
            return true;

        /* Opaque types - represent C types like FILE* */
        case TYPE_OPAQUE:
            return true;

        /* Native function types (callback types) - C compatible function pointers */
        case TYPE_FUNCTION:
            return type->as.function.is_native;

        /* Non-C-compatible Sindarin types */
        case TYPE_STRING:  /* Sindarin strings are managed */
        case TYPE_ARRAY:   /* Sindarin arrays have metadata (length, etc.) */
        case TYPE_NIL:
        case TYPE_ANY:
        default:
            return false;
    }
}

bool is_valid_field_type(Type *type, SymbolTable *table)
{
    if (type == NULL)
    {
        return false;
    }

    switch (type->kind)
    {
        /* Primitive types - always valid */
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
            return true;

        /* Built-in reference types - always valid */
        case TYPE_ANY:
            return true;

        /* Pointer types - valid (checked for native struct separately) */
        case TYPE_POINTER:
            /* Recursively check base type is valid */
            return is_valid_field_type(type->as.pointer.base_type, table);

        /* Array types - check element type is valid */
        case TYPE_ARRAY:
            return is_valid_field_type(type->as.array.element_type, table);

        /* Opaque types - always valid (represents external C types) */
        case TYPE_OPAQUE:
            return true;

        /* Struct types - need to verify the struct is defined */
        case TYPE_STRUCT:
        {
            if (type->as.struct_type.name == NULL)
            {
                return false;
            }
            /* If this is a forward reference (fields == NULL and field_count == 0),
             * verify the struct actually exists in the symbol table */
            if (type->as.struct_type.fields == NULL && type->as.struct_type.field_count == 0)
            {
                if (table == NULL)
                {
                    return false; /* Can't verify without symbol table */
                }
                Token lookup_tok;
                lookup_tok.start = type->as.struct_type.name;
                lookup_tok.length = (int)strlen(type->as.struct_type.name);
                Symbol *sym = symbol_table_lookup_type(table, lookup_tok);
                if (sym == NULL || sym->type == NULL || sym->type->kind != TYPE_STRUCT)
                {
                    return false; /* Struct not defined */
                }
            }
            return true;
        }

        /* Function types (closures) - valid as fields */
        case TYPE_FUNCTION:
            return true;

        /* TYPE_NIL is not a valid field type */
        case TYPE_NIL:
        default:
            return false;
    }
}

/* Maximum depth for circular dependency detection to prevent stack overflow */
#define MAX_CYCLE_DEPTH 64

/* Helper struct to track visited struct names during cycle detection */
typedef struct {
    const char *names[MAX_CYCLE_DEPTH];
    int count;
} VisitedStructs;

/* Check if a struct name is already in the visited set */
static bool is_struct_visited(VisitedStructs *visited, const char *name)
{
    if (name == NULL) return false;
    for (int i = 0; i < visited->count; i++)
    {
        if (visited->names[i] != NULL && strcmp(visited->names[i], name) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Add a struct name to the visited set. Returns false if full. */
static bool add_visited_struct(VisitedStructs *visited, const char *name)
{
    if (visited->count >= MAX_CYCLE_DEPTH || name == NULL)
    {
        return false;
    }
    visited->names[visited->count++] = name;
    return true;
}

/* Build the dependency chain string from visited structs */
static void build_chain_string(VisitedStructs *visited, const char *cycle_start,
                                char *chain_out, int chain_size)
{
    int offset = 0;
    bool found_start = false;

    for (int i = 0; i < visited->count && offset < chain_size - 1; i++)
    {
        if (!found_start && strcmp(visited->names[i], cycle_start) == 0)
        {
            found_start = true;
        }
        if (found_start)
        {
            int written = snprintf(chain_out + offset, chain_size - offset,
                                    "%s%s", (offset > 0) ? " -> " : "", visited->names[i]);
            if (written > 0) offset += written;
        }
    }
    /* Add the cycle-completing reference back to the start */
    if (offset < chain_size - 1)
    {
        snprintf(chain_out + offset, chain_size - offset, " -> %s", cycle_start);
    }
}

/* Recursive helper to detect circular dependencies in struct field types. */
static bool detect_cycle_in_type(Type *type, SymbolTable *table, VisitedStructs *visited,
                                  char *chain_out, int chain_size)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
        case TYPE_STRUCT:
        {
            const char *struct_name = type->as.struct_type.name;
            if (struct_name == NULL) return false;

            /* Check if we've already visited this struct (cycle detected) */
            if (is_struct_visited(visited, struct_name))
            {
                build_chain_string(visited, struct_name, chain_out, chain_size);
                return true;
            }

            /* Add this struct to visited set */
            if (!add_visited_struct(visited, struct_name))
            {
                /* Too deep, treat as no cycle to avoid stack issues */
                return false;
            }

            /* If this is a forward reference, look up the actual struct type */
            Type *resolved_type = type;
            if (type->as.struct_type.field_count == 0 && type->as.struct_type.fields == NULL && table != NULL)
            {
                Token lookup_tok;
                lookup_tok.start = struct_name;
                lookup_tok.length = (int)strlen(struct_name);

                Symbol *sym = symbol_table_lookup_type(table, lookup_tok);
                if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
                {
                    resolved_type = sym->type;
                }
            }

            /* Check all fields for cycles */
            for (int i = 0; i < resolved_type->as.struct_type.field_count; i++)
            {
                StructField *field = &resolved_type->as.struct_type.fields[i];
                if (detect_cycle_in_type(field->type, table, visited, chain_out, chain_size))
                {
                    return true;
                }
            }

            /* Remove this struct from visited set (backtrack) */
            visited->count--;
            return false;
        }

        case TYPE_ARRAY:
            /* Arrays of structs can also cause circular dependencies */
            return detect_cycle_in_type(type->as.array.element_type, table, visited, chain_out, chain_size);

        case TYPE_POINTER:
            /* Pointers break cycles - a pointer to a struct is fine */
            return false;

        default:
            /* Other types cannot cause struct circular dependencies */
            return false;
    }
}

bool detect_struct_circular_dependency(Type *struct_type, SymbolTable *table, char *chain_out, int chain_size)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return false;
    }

    if (chain_out != NULL && chain_size > 0)
    {
        chain_out[0] = '\0';
    }

    VisitedStructs visited;
    visited.count = 0;

    /* Start with the root struct */
    const char *root_name = struct_type->as.struct_type.name;
    if (root_name == NULL) return false;

    if (!add_visited_struct(&visited, root_name))
    {
        return false;
    }

    /* Check all fields for cycles */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (detect_cycle_in_type(field->type, table, &visited, chain_out, chain_size))
        {
            return true;
        }
    }

    return false;
}

Type *resolve_struct_forward_reference(Type *type, SymbolTable *table)
{
    if (type == NULL || table == NULL)
    {
        return type;
    }

    /* Recurse into array element types */
    if (type->kind == TYPE_ARRAY)
    {
        type->as.array.element_type =
            resolve_struct_forward_reference(type->as.array.element_type, table);
        return type;
    }

    if (type->kind != TYPE_STRUCT)
    {
        return type;
    }

    /* Check if this is a forward reference (0 fields and NULL fields pointer) */
    if (type->as.struct_type.field_count == 0 && type->as.struct_type.fields == NULL)
    {
        const char *struct_name = type->as.struct_type.name;
        if (struct_name == NULL)
        {
            return type;
        }

        Symbol *sym = NULL;

        /* Check for namespace-qualified type name (e.g., "L15.State") */
        const char *dot_pos = strchr(struct_name, '.');
        if (dot_pos != NULL)
        {
            /* Split into namespace and type name */
            int ns_len = (int)(dot_pos - struct_name);
            const char *type_name_part = dot_pos + 1;
            int type_len = (int)strlen(type_name_part);

            Token ns_tok;
            ns_tok.start = struct_name;
            ns_tok.length = ns_len;

            Token type_tok;
            type_tok.start = type_name_part;
            type_tok.length = type_len;

            /* Look up the type in the namespace */
            sym = symbol_table_lookup_in_namespace(table, ns_tok, type_tok);
        }
        else
        {
            /* Regular unqualified type lookup */
            Token lookup_tok;
            lookup_tok.start = struct_name;
            lookup_tok.length = (int)strlen(struct_name);

            sym = symbol_table_lookup_type(table, lookup_tok);
        }

        if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
        {
            /* Found the complete type - use it instead */
            return sym->type;
        }
    }

    return type;
}

void get_module_symbols(Module *imported_module, SymbolTable *table,
                        Token ***symbols_out, Type ***types_out, int *count_out)
{
    /* Initialize outputs to empty/NULL */
    *symbols_out = NULL;
    *types_out = NULL;
    *count_out = 0;

    if (imported_module == NULL || imported_module->statements == NULL || table == NULL)
    {
        return;
    }

    Arena *arena = table->arena;
    int capacity = 8;
    int count = 0;
    Token **symbols = arena_alloc(arena, sizeof(Token *) * capacity);
    Type **types = arena_alloc(arena, sizeof(Type *) * capacity);

    if (symbols == NULL || types == NULL)
    {
        DEBUG_ERROR("Failed to allocate memory for module symbols");
        return;
    }

    /* Walk through module statements and extract function definitions */
    for (int i = 0; i < imported_module->count; i++)
    {
        Stmt *stmt = imported_module->statements[i];
        if (stmt == NULL)
            continue;

        if (stmt->type == STMT_FUNCTION)
        {
            FunctionStmt *func = &stmt->as.function;

            /* Grow arrays if needed */
            if (count >= capacity)
            {
                int new_capacity = capacity * 2;
                Token **new_symbols = arena_alloc(arena, sizeof(Token *) * new_capacity);
                Type **new_types = arena_alloc(arena, sizeof(Type *) * new_capacity);
                if (new_symbols == NULL || new_types == NULL)
                {
                    DEBUG_ERROR("Failed to grow module symbols arrays");
                    *symbols_out = symbols;
                    *types_out = types;
                    *count_out = count;
                    return;
                }
                memcpy(new_symbols, symbols, sizeof(Token *) * count);
                memcpy(new_types, types, sizeof(Type *) * count);
                symbols = new_symbols;
                types = new_types;
                capacity = new_capacity;
            }

            /* Store symbol name token */
            Token *name_token = arena_alloc(arena, sizeof(Token));
            if (name_token == NULL)
            {
                DEBUG_ERROR("Failed to allocate token for function name");
                continue;
            }
            *name_token = func->name;
            symbols[count] = name_token;

            /* Build function type */
            Type **param_types = NULL;
            if (func->param_count > 0)
            {
                param_types = arena_alloc(arena, sizeof(Type *) * func->param_count);
                if (param_types == NULL)
                {
                    DEBUG_ERROR("Failed to allocate param types");
                    continue;
                }
                for (int j = 0; j < func->param_count; j++)
                {
                    Type *param_type = func->params[j].type;
                    if (param_type == NULL)
                    {
                        param_type = ast_create_primitive_type(arena, TYPE_NIL);
                    }
                    param_types[j] = param_type;
                }
            }

            Type *func_type = ast_create_function_type(arena, func->return_type, param_types, func->param_count);
            /* Carry over variadic, native, has_body, and has_arena_param flags from function statement */
            func_type->as.function.is_variadic = func->is_variadic;
            func_type->as.function.is_native = func->is_native;
            func_type->as.function.has_body = (func->body != NULL);
            func_type->as.function.has_arena_param = func->has_arena_param;

            /* Store parameter memory qualifiers if any non-default exist */
            if (func->param_count > 0)
            {
                bool has_non_default_qual = false;
                for (int j = 0; j < func->param_count; j++)
                {
                    if (func->params[j].mem_qualifier != MEM_DEFAULT)
                    {
                        has_non_default_qual = true;
                        break;
                    }
                }
                if (has_non_default_qual)
                {
                    func_type->as.function.param_mem_quals = arena_alloc(arena,
                        sizeof(MemoryQualifier) * func->param_count);
                    if (func_type->as.function.param_mem_quals != NULL)
                    {
                        for (int j = 0; j < func->param_count; j++)
                        {
                            func_type->as.function.param_mem_quals[j] = func->params[j].mem_qualifier;
                        }
                    }
                }
            }

            types[count] = func_type;
            count++;
        }
    }

    *symbols_out = symbols;
    *types_out = types;
    *count_out = count;
}
