#include "type_checker/expr/type_checker_expr_struct.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>

/* Struct literal: StructName { field1: value1, field2: value2, ... } */
Type *type_check_struct_literal(Expr *expr, SymbolTable *table)
{
    Token struct_name = expr->as.struct_literal.struct_name;
    Symbol *struct_sym = symbol_table_lookup_type(table, struct_name);
    if (struct_sym == NULL || struct_sym->type == NULL)
    {
        type_error(&struct_name, "Unknown struct type");
        return NULL;
    }
    else if (struct_sym->type->kind != TYPE_STRUCT)
    {
        type_error(&struct_name, "Expected struct type");
        return NULL;
    }

    Type *struct_type = struct_sym->type;

    /* Native struct usage context validation:
     * Native structs can only be instantiated inside native fn context */
    if (struct_type->as.struct_type.is_native && !native_context_is_active())
    {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Native struct '%s' can only be used in native function context. "
                 "Declare the function with 'native fn':\n"
                 "    native fn example(): void =>\n"
                 "        var x: %s = %s { ... }",
                 struct_type->as.struct_type.name,
                 struct_type->as.struct_type.name,
                 struct_type->as.struct_type.name);
        type_error(&struct_name, msg);
        return NULL;
    }

    /* Store resolved struct type for code generation */
    expr->as.struct_literal.struct_type = struct_type;

    /* Allocate and initialize the fields_initialized tracking array */
    int total_fields = struct_type->as.struct_type.field_count;
    expr->as.struct_literal.total_field_count = total_fields;
    if (total_fields > 0)
    {
        expr->as.struct_literal.fields_initialized = arena_alloc(table->arena, sizeof(bool) * total_fields);
        if (expr->as.struct_literal.fields_initialized == NULL)
        {
            type_error(&struct_name, "Out of memory allocating field initialization tracking");
            return NULL;
        }
        /* Initialize all fields as not initialized */
        for (int i = 0; i < total_fields; i++)
        {
            expr->as.struct_literal.fields_initialized[i] = false;
        }
    }

    /* Type check each field initializer and mark fields as initialized */
    for (int i = 0; i < expr->as.struct_literal.field_count; i++)
    {
        FieldInitializer *init = &expr->as.struct_literal.fields[i];
        Type *init_type = type_check_expr(init->value, table);

        /* Find the field in the struct definition */
        bool found = false;
        for (int j = 0; j < struct_type->as.struct_type.field_count; j++)
        {
            StructField *field = &struct_type->as.struct_type.fields[j];
            if ((size_t)init->name.length == strlen(field->name) &&
                strncmp(init->name.start, field->name, init->name.length) == 0)
            {
                found = true;
                /* Mark this field as explicitly initialized */
                if (expr->as.struct_literal.fields_initialized != NULL)
                {
                    expr->as.struct_literal.fields_initialized[j] = true;
                }
                /* Type check: initializer type must match field type */
                if (init_type != NULL && field->type != NULL &&
                    !ast_type_equals(init_type, field->type))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Type mismatch for field '%s' in struct literal",
                             field->name);
                    type_error(&init->name, msg);
                }
                break;
            }
        }
        if (!found)
        {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Unknown field '%.*s' in struct literal",
                     init->name.length, init->name.start);
            type_error(&init->name, msg);
        }
    }

    /* Apply default values for uninitialized fields that have defaults.
     * Count how many defaults we need to add, then reallocate the fields array. */
    int defaults_to_add = 0;
    for (int i = 0; i < total_fields; i++)
    {
        if (!expr->as.struct_literal.fields_initialized[i] &&
            struct_type->as.struct_type.fields[i].default_value != NULL)
        {
            defaults_to_add++;
        }
    }

    if (defaults_to_add > 0)
    {
        /* Reallocate the fields array to hold additional synthetic initializers */
        int new_field_count = expr->as.struct_literal.field_count + defaults_to_add;
        FieldInitializer *new_fields = arena_alloc(table->arena, sizeof(FieldInitializer) * new_field_count);
        if (new_fields == NULL)
        {
            type_error(&struct_name, "Out of memory allocating field initializers for defaults");
            return NULL;
        }

        /* Copy existing field initializers */
        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
        {
            new_fields[i] = expr->as.struct_literal.fields[i];
        }

        /* Add synthetic initializers for fields with default values */
        int added = 0;
        for (int i = 0; i < total_fields; i++)
        {
            if (!expr->as.struct_literal.fields_initialized[i] &&
                struct_type->as.struct_type.fields[i].default_value != NULL)
            {
                StructField *field = &struct_type->as.struct_type.fields[i];

                /* Type-check the default value */
                Type *default_type = type_check_expr(field->default_value, table);
                if (default_type != NULL && field->type != NULL &&
                    !ast_type_equals(default_type, field->type))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Type mismatch for default value of field '%s' in struct '%s'",
                             field->name, struct_type->as.struct_type.name);
                    type_error(expr->token, msg);
                }

                /* Create synthetic field initializer with the default value */
                int idx = expr->as.struct_literal.field_count + added;

                /* Create a Token for the field name */
                new_fields[idx].name.start = field->name;
                new_fields[idx].name.length = strlen(field->name);
                new_fields[idx].name.line = expr->token ? expr->token->line : 0;
                new_fields[idx].name.type = TOKEN_IDENTIFIER;
                new_fields[idx].name.filename = expr->token ? expr->token->filename : NULL;
                new_fields[idx].value = field->default_value;

                /* Mark this field as initialized (via default) */
                expr->as.struct_literal.fields_initialized[i] = true;

                added++;
            }
        }

        /* Update the struct literal with the new fields array */
        expr->as.struct_literal.fields = new_fields;
        expr->as.struct_literal.field_count = new_field_count;
    }

    /* Check for uninitialized fields that have no default values.
     * These are required fields that must be explicitly provided. */
    int missing_count = 0;
    char missing_fields[512] = "";
    int missing_pos = 0;

    for (int i = 0; i < total_fields; i++)
    {
        if (!expr->as.struct_literal.fields_initialized[i])
        {
            StructField *field = &struct_type->as.struct_type.fields[i];
            /* This field is not initialized and has no default - it's required */
            if (missing_count > 0 && missing_pos < (int)sizeof(missing_fields) - 3)
            {
                missing_pos += snprintf(missing_fields + missing_pos,
                                        sizeof(missing_fields) - missing_pos, ", ");
            }
            if (missing_pos < (int)sizeof(missing_fields) - 1)
            {
                missing_pos += snprintf(missing_fields + missing_pos,
                                        sizeof(missing_fields) - missing_pos, "'%s'",
                                        field->name);
            }
            missing_count++;
        }
    }

    if (missing_count > 0)
    {
        char msg[768];
        if (missing_count == 1)
        {
            snprintf(msg, sizeof(msg),
                     "Missing required field %s in struct literal '%s'",
                     missing_fields, struct_type->as.struct_type.name);
        }
        else
        {
            snprintf(msg, sizeof(msg),
                     "Missing %d required fields in struct literal '%s': %s",
                     missing_count, struct_type->as.struct_type.name, missing_fields);
        }
        type_error(&struct_name, msg);
        return NULL;
    }

    DEBUG_VERBOSE("Struct literal type check: returns struct type '%s'",
                  struct_type->as.struct_type.name);
    return struct_type;
}

/* sizeof operator: returns the size in bytes of a type or expression */
/* sizeof(Type) or sizeof(expr) - returns int */
Type *type_check_sizeof(Expr *expr, SymbolTable *table)
{
    if (expr->as.sizeof_expr.type_operand != NULL)
    {
        /* sizeof(Type) - resolve the type and get its size */
        Type *type_operand = expr->as.sizeof_expr.type_operand;

        /* If it's a struct type reference, resolve it */
        if (type_operand->kind == TYPE_STRUCT && type_operand->as.struct_type.fields == NULL)
        {
            /* Forward reference - look up the actual struct */
            Token name_token;
            name_token.start = type_operand->as.struct_type.name;
            name_token.length = strlen(type_operand->as.struct_type.name);
            Symbol *struct_sym = symbol_table_lookup_type(table, name_token);
            if (struct_sym != NULL && struct_sym->type != NULL &&
                struct_sym->type->kind == TYPE_STRUCT)
            {
                expr->as.sizeof_expr.type_operand = struct_sym->type;
            }
            else
            {
                char msg[256];
                snprintf(msg, sizeof(msg), "Unknown type '%s' in sizeof",
                         type_operand->as.struct_type.name);
                type_error(expr->token, msg);
                return NULL;
            }
        }
        DEBUG_VERBOSE("sizeof type: returns int");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }
    else if (expr->as.sizeof_expr.expr_operand != NULL)
    {
        /* sizeof(expr) - type check the expression to get its type */
        Type *operand_type = type_check_expr(expr->as.sizeof_expr.expr_operand, table);
        if (operand_type == NULL)
        {
            type_error(expr->token, "Invalid operand in sizeof expression");
            return NULL;
        }
        else
        {
            DEBUG_VERBOSE("sizeof expression: returns int");
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
    }
    else
    {
        type_error(expr->token, "sizeof requires a type or expression operand");
        return NULL;
    }
}
