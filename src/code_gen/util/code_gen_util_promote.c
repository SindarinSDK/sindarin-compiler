#include "code_gen/util/code_gen_util.h"

/* ============================================================================
 * Tail Call Optimization Helpers
 * ============================================================================ */

/* Check if an expression contains a marked tail call */
static bool expr_has_marked_tail_call(Expr *expr)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    case EXPR_CALL:
        return expr->as.call.is_tail_call;

    case EXPR_BINARY:
        return expr_has_marked_tail_call(expr->as.binary.left) ||
               expr_has_marked_tail_call(expr->as.binary.right);

    case EXPR_UNARY:
        return expr_has_marked_tail_call(expr->as.unary.operand);

    case EXPR_ASSIGN:
        return expr_has_marked_tail_call(expr->as.assign.value);

    case EXPR_INDEX_ASSIGN:
        return expr_has_marked_tail_call(expr->as.index_assign.array) ||
               expr_has_marked_tail_call(expr->as.index_assign.index) ||
               expr_has_marked_tail_call(expr->as.index_assign.value);

    case EXPR_ARRAY_ACCESS:
        return expr_has_marked_tail_call(expr->as.array_access.array) ||
               expr_has_marked_tail_call(expr->as.array_access.index);

    default:
        return false;
    }
}

bool stmt_has_marked_tail_calls(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
        return stmt->as.return_stmt.value &&
               expr_has_marked_tail_call(stmt->as.return_stmt.value);

    case STMT_EXPR:
        return expr_has_marked_tail_call(stmt->as.expression.expression);

    case STMT_VAR_DECL:
        return stmt->as.var_decl.initializer &&
               expr_has_marked_tail_call(stmt->as.var_decl.initializer);

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_has_marked_tail_calls(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (stmt_has_marked_tail_calls(stmt->as.if_stmt.then_branch))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            stmt_has_marked_tail_calls(stmt->as.if_stmt.else_branch))
        {
            return true;
        }
        return false;

    case STMT_WHILE:
        return stmt_has_marked_tail_calls(stmt->as.while_stmt.body);

    case STMT_FOR:
        return stmt_has_marked_tail_calls(stmt->as.for_stmt.body);

    case STMT_FOR_EACH:
        return stmt_has_marked_tail_calls(stmt->as.for_each_stmt.body);

    case STMT_LOCK:
        return stmt_has_marked_tail_calls(stmt->as.lock_stmt.body);

    default:
        return false;
    }
}

bool function_has_marked_tail_calls(FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL) return false;

    for (int i = 0; i < fn->body_count; i++)
    {
        if (stmt_has_marked_tail_calls(fn->body[i]))
        {
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Struct Field Promotion Helpers
 * ============================================================================
 * These functions help generate code to promote handle fields in structs
 * when returning from functions or synchronizing threads.
 * ============================================================================ */

/* Check if a struct type has any handle fields that need promotion */
bool struct_has_handle_fields(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT) {
        return false;
    }

    int field_count = struct_type->as.struct_type.field_count;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) {
            continue;
        }

        TypeKind kind = field->type->kind;
        if (kind == TYPE_STRING || kind == TYPE_ARRAY || kind == TYPE_ANY ||
            kind == TYPE_STRUCT || kind == TYPE_FUNCTION)
        {
            return true;
        }
    }
    return false;
}

/* Helper to generate promotion code for a single field */
static char *gen_field_promotion_code(Arena *arena, Type *field_type, const char *field_access,
                                       const char *dest_arena, const char *src_arena)
{
    if (field_type == NULL) return "";

    TypeKind kind = field_type->kind;

    if (kind == TYPE_STRING)
    {
        return arena_sprintf(arena, "        %s = rt_managed_promote(%s, %s, %s);\n",
                             field_access, dest_arena, src_arena, field_access);
    }
    else if (kind == TYPE_ARRAY)
    {
        Type *elem_type = field_type->as.array.element_type;
        if (elem_type == NULL)
        {
            return arena_sprintf(arena, "        %s = rt_managed_promote(%s, %s, %s);\n",
                                 field_access, dest_arena, src_arena, field_access);
        }

        if (elem_type->kind == TYPE_STRING)
        {
            /* str[] */
            return arena_sprintf(arena, "        %s = rt_managed_promote_array_string(%s, %s, %s);\n",
                                 field_access, dest_arena, src_arena, field_access);
        }
        else if (elem_type->kind == TYPE_ARRAY)
        {
            /* 2D or 3D array */
            Type *inner_elem = elem_type->as.array.element_type;
            if (inner_elem && inner_elem->kind == TYPE_STRING)
            {
                /* str[][] */
                return arena_sprintf(arena, "        %s = rt_managed_promote_array2_string(%s, %s, %s);\n",
                                     field_access, dest_arena, src_arena, field_access);
            }
            else if (inner_elem && inner_elem->kind == TYPE_ARRAY)
            {
                /* 3D array - check innermost */
                Type *innermost = inner_elem->as.array.element_type;
                if (innermost && innermost->kind == TYPE_STRING)
                {
                    /* str[][][] */
                    return arena_sprintf(arena, "        %s = rt_managed_promote_array3_string(%s, %s, %s);\n",
                                         field_access, dest_arena, src_arena, field_access);
                }
                else
                {
                    /* T[][][] */
                    return arena_sprintf(arena, "        %s = rt_managed_promote_array_handle_3d(%s, %s, %s);\n",
                                         field_access, dest_arena, src_arena, field_access);
                }
            }
            else
            {
                /* T[][] */
                return arena_sprintf(arena, "        %s = rt_managed_promote_array_handle(%s, %s, %s);\n",
                                     field_access, dest_arena, src_arena, field_access);
            }
        }
        else if (elem_type->kind == TYPE_ANY)
        {
            /* any[] - shallow promote for now */
            return arena_sprintf(arena, "        %s = rt_managed_promote(%s, %s, %s);\n",
                                 field_access, dest_arena, src_arena, field_access);
        }
        else if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type))
        {
            /* Struct[] where struct has handle fields - need to promote handles in each element */
            const char *struct_c_name = elem_type->as.struct_type.c_alias != NULL
                ? elem_type->as.struct_type.c_alias
                : arena_sprintf(arena, "__sn__%s", elem_type->as.struct_type.name);

            /* Generate a unique index variable name using field_access hash */
            unsigned int hash = 0;
            for (const char *p = field_access; *p; p++) hash = hash * 31 + *p;

            /* Build promotion code for each handle field in the struct */
            char *field_promotions = arena_strdup(arena, "");
            int struct_field_count = elem_type->as.struct_type.field_count;
            for (int fi = 0; fi < struct_field_count; fi++)
            {
                StructField *sf = &elem_type->as.struct_type.fields[fi];
                if (sf->type == NULL) continue;

                const char *sf_c_name = sf->c_alias != NULL ? sf->c_alias
                    : arena_sprintf(arena, "__sn__%s", sf->name);

                if (sf->type->kind == TYPE_STRING)
                {
                    field_promotions = arena_sprintf(arena,
                        "%s            __parr_%u__[__pi_%u__].%s = rt_managed_promote(%s, %s, __parr_%u__[__pi_%u__].%s);\n",
                        field_promotions, hash, hash, sf_c_name, dest_arena, src_arena, hash, hash, sf_c_name);
                }
                else if (sf->type->kind == TYPE_ARRAY)
                {
                    Type *sf_elem = sf->type->as.array.element_type;
                    if (sf_elem && sf_elem->kind == TYPE_STRING)
                    {
                        field_promotions = arena_sprintf(arena,
                            "%s            __parr_%u__[__pi_%u__].%s = rt_managed_promote_array_string(%s, %s, __parr_%u__[__pi_%u__].%s);\n",
                            field_promotions, hash, hash, sf_c_name, dest_arena, src_arena, hash, hash, sf_c_name);
                    }
                    else
                    {
                        field_promotions = arena_sprintf(arena,
                            "%s            __parr_%u__[__pi_%u__].%s = rt_managed_promote(%s, %s, __parr_%u__[__pi_%u__].%s);\n",
                            field_promotions, hash, hash, sf_c_name, dest_arena, src_arena, hash, hash, sf_c_name);
                    }
                }
            }

            return arena_sprintf(arena,
                "        { /* Promote handles in struct array elements */\n"
                "            %s *__parr_%u__ = ((%s *)rt_managed_pin_array(%s, %s));\n"
                "            long __plen_%u__ = rt_array_length(__parr_%u__);\n"
                "            for (long __pi_%u__ = 0; __pi_%u__ < __plen_%u__; __pi_%u__++) {\n"
                "%s"
                "            }\n"
                "            %s = rt_managed_promote(%s, %s, %s);\n"
                "        }\n",
                struct_c_name, hash, struct_c_name, src_arena, field_access,
                hash, hash,
                hash, hash, hash, hash,
                field_promotions,
                field_access, dest_arena, src_arena, field_access);
        }
        else
        {
            /* Primitive array (int[], long[], etc.) or struct without handle fields */
            return arena_sprintf(arena, "        %s = rt_managed_promote(%s, %s, %s);\n",
                                 field_access, dest_arena, src_arena, field_access);
        }
    }
    else if (kind == TYPE_ANY)
    {
        /* any type - shallow promote for now */
        /* TODO: For any containing strings/arrays, would need runtime type check */
        return "";
    }

    return "";
}

/* Generate code to promote all handle fields in a struct */
char *gen_struct_field_promotion(CodeGen *gen, Type *struct_type, const char *var_name,
                                  const char *dest_arena, const char *src_arena)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT) return "";
    if (!struct_has_handle_fields(struct_type)) return "";

    char *result = arena_strdup(gen->arena, "");
    int field_count = struct_type->as.struct_type.field_count;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
        char *field_access = arena_sprintf(gen->arena, "%s.%s", var_name, c_field_name);

        char *promo_code = gen_field_promotion_code(gen->arena, field->type, field_access, dest_arena, src_arena);
        result = arena_sprintf(gen->arena, "%s%s", result, promo_code);
    }

    return result;
}
