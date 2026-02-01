#include "type_checker/stmt/type_checker_stmt_struct.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

void type_check_struct_decl(Stmt *stmt, SymbolTable *table)
{
    StructDeclStmt *struct_decl = &stmt->as.struct_decl;

    DEBUG_VERBOSE("Type checking struct declaration: %.*s with %d fields",
                  struct_decl->name.length, struct_decl->name.start,
                  struct_decl->field_count);

    /* Check each field */
    for (int i = 0; i < struct_decl->field_count; i++)
    {
        StructField *field = &struct_decl->fields[i];
        Type *field_type = field->type;

        if (field_type == NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Field '%s' has no type", field->name);
            type_error(&struct_decl->name, msg);
            continue;
        }

        /* Validate the field type is resolvable */
        if (!is_valid_field_type(field_type, table))
        {
            char msg[512];
            char struct_name_buf[128];
            int struct_name_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
            memcpy(struct_name_buf, struct_decl->name.start, struct_name_len);
            struct_name_buf[struct_name_len] = '\0';

            const char *type_name = "unknown";
            if (field_type->kind == TYPE_STRUCT && field_type->as.struct_type.name != NULL)
            {
                type_name = field_type->as.struct_type.name;
            }

            snprintf(msg, sizeof(msg),
                     "In struct '%s': field '%s' has undefined type '%s'",
                     struct_name_buf, field->name, type_name);
            type_error(&struct_decl->name, msg);
            continue;
        }

        /* Pointer fields require native struct */
        if (!struct_decl->is_native && field_type->kind == TYPE_POINTER)
        {
            char msg[512];
            char sname[128];
            int sname_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
            memcpy(sname, struct_decl->name.start, sname_len);
            sname[sname_len] = '\0';

            snprintf(msg, sizeof(msg),
                     "Pointer field '%s' not allowed in struct '%s'. "
                     "Use 'native struct' for structs with pointer fields:\n"
                     "    native struct %s =>\n"
                     "        %s: *...",
                     field->name, sname, sname, field->name);
            type_error(&struct_decl->name, msg);
        }

        /* Type check default value if present */
        if (field->default_value != NULL)
        {
            Type *default_type = type_check_expr(field->default_value, table);
            if (default_type != NULL && !ast_type_equals(default_type, field_type))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Default value type does not match field '%s' type",
                         field->name);
                type_error(&struct_decl->name, msg);
            }
        }

        DEBUG_VERBOSE("  Field '%s' type validated", field->name);
    }

    /* Type check each method */
    for (int i = 0; i < struct_decl->method_count; i++)
    {
        StructMethod *method = &struct_decl->methods[i];

        DEBUG_VERBOSE("  Type checking method '%s' (static=%d, native=%d)",
                      method->name, method->is_static, method->is_native);

        /* Resolve forward references in method return type and parameter types */
        if (method->return_type != NULL)
        {
            method->return_type = resolve_struct_forward_reference(method->return_type, table);
        }
        for (int j = 0; j < method->param_count; j++)
        {
            if (method->params[j].type != NULL)
            {
                method->params[j].type = resolve_struct_forward_reference(method->params[j].type, table);
            }
        }

        /* For non-native methods, type check the body */
        if (!method->is_native && method->body != NULL)
        {
            symbol_table_push_scope(table);

            /* Add 'arena' built-in identifier for non-native methods */
            add_arena_builtin(table, &struct_decl->name);

            /* For instance methods, add 'self' parameter to scope */
            if (!method->is_static)
            {
                Symbol *struct_sym = symbol_table_lookup_type(table, struct_decl->name);
                if (struct_sym != NULL && struct_sym->type != NULL)
                {
                    Token self_token;
                    self_token.start = "self";
                    self_token.length = 4;
                    self_token.line = struct_decl->name.line;
                    self_token.filename = struct_decl->name.filename;
                    self_token.type = TOKEN_IDENTIFIER;

                    /* For opaque handle types, self is the struct type itself.
                     * Otherwise, self is a pointer to the struct type. */
                    Type *self_type;
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        self_type = struct_sym->type;
                    }
                    else
                    {
                        self_type = ast_create_pointer_type(table->arena, struct_sym->type);
                    }
                    symbol_table_add_symbol(table, self_token, self_type);
                }
            }

            /* Add method parameters to scope */
            for (int j = 0; j < method->param_count; j++)
            {
                Parameter *param = &method->params[j];
                if (param->type != NULL)
                {
                    symbol_table_add_symbol_full(table, param->name, param->type, SYMBOL_PARAM, param->mem_qualifier);
                }
            }

            /* Enter method context to allow pointer-to-struct access for 'self' */
            method_context_enter();

            /* Type check the method body */
            for (int j = 0; j < method->body_count; j++)
            {
                if (method->body[j] != NULL)
                {
                    type_check_stmt(method->body[j], table, method->return_type);
                }
            }

            method_context_exit();
            symbol_table_pop_scope(table);
        }
    }

    /* Check for circular dependencies in struct types */
    Type temp_struct_type;
    temp_struct_type.kind = TYPE_STRUCT;
    temp_struct_type.as.struct_type.name = NULL;

    char struct_name[128];
    int name_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
    memcpy(struct_name, struct_decl->name.start, name_len);
    struct_name[name_len] = '\0';
    temp_struct_type.as.struct_type.name = struct_name;

    temp_struct_type.as.struct_type.fields = struct_decl->fields;
    temp_struct_type.as.struct_type.field_count = struct_decl->field_count;
    temp_struct_type.as.struct_type.methods = struct_decl->methods;
    temp_struct_type.as.struct_type.method_count = struct_decl->method_count;
    temp_struct_type.as.struct_type.is_native = struct_decl->is_native;
    temp_struct_type.as.struct_type.size = 0;
    temp_struct_type.as.struct_type.alignment = 0;

    char cycle_chain[512];
    if (detect_struct_circular_dependency(&temp_struct_type, table, cycle_chain, sizeof(cycle_chain)))
    {
        char msg[768];
        snprintf(msg, sizeof(msg),
                 "Circular dependency detected in struct '%s': %s",
                 struct_name, cycle_chain);
        type_error(&struct_decl->name, msg);
        return;
    }

    /* Calculate struct layout */
    Symbol *struct_sym = symbol_table_lookup_type(table, struct_decl->name);
    if (struct_sym != NULL && struct_sym->type != NULL && struct_sym->type->kind == TYPE_STRUCT)
    {
        calculate_struct_layout(struct_sym->type);
        DEBUG_VERBOSE("Struct '%s' layout: size=%zu, alignment=%zu",
                      struct_name,
                      struct_sym->type->as.struct_type.size,
                      struct_sym->type->as.struct_type.alignment);
    }
}
