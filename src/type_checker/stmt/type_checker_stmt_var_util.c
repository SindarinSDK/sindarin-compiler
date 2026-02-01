#include "type_checker/stmt/type_checker_stmt_var_util.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"

bool apply_array_coercion(Stmt *stmt, Type *decl_type, Type **init_type_ptr)
{
    Type *init_type = *init_type_ptr;
    if (decl_type == NULL || init_type == NULL)
        return false;

    /* For empty array literals, adopt the declared type for code generation */
    if (decl_type->kind == TYPE_ARRAY && init_type->kind == TYPE_ARRAY &&
        init_type->as.array.element_type->kind == TYPE_NIL &&
        decl_type->kind == TYPE_ARRAY)
    {
        stmt->as.var_decl.initializer->expr_type = decl_type;
        *init_type_ptr = decl_type;
        return true;
    }

    /* For int[] assigned to byte[], update the expression type to byte[] */
    if (decl_type->kind == TYPE_ARRAY &&
        decl_type->as.array.element_type->kind == TYPE_BYTE &&
        init_type->kind == TYPE_ARRAY &&
        init_type->as.array.element_type->kind == TYPE_INT)
    {
        stmt->as.var_decl.initializer->expr_type = decl_type;
        *init_type_ptr = decl_type;
        return true;
    }

    /* For int[] assigned to int32[], uint32[], uint[], or float[] */
    if (decl_type->kind == TYPE_ARRAY &&
        init_type->kind == TYPE_ARRAY &&
        init_type->as.array.element_type->kind == TYPE_INT)
    {
        TypeKind decl_elem = decl_type->as.array.element_type->kind;
        if (decl_elem == TYPE_INT32 || decl_elem == TYPE_UINT32 ||
            decl_elem == TYPE_UINT || decl_elem == TYPE_FLOAT)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            *init_type_ptr = decl_type;
            return true;
        }
    }

    /* For double[] assigned to float[], update the expression type */
    if (decl_type->kind == TYPE_ARRAY &&
        decl_type->as.array.element_type->kind == TYPE_FLOAT &&
        init_type->kind == TYPE_ARRAY &&
        init_type->as.array.element_type->kind == TYPE_DOUBLE)
    {
        stmt->as.var_decl.initializer->expr_type = decl_type;
        *init_type_ptr = decl_type;
        return true;
    }

    /* For 2D arrays: byte[][] = {{1,2,3},...} - coerce inner arrays */
    if (decl_type->kind == TYPE_ARRAY &&
        decl_type->as.array.element_type->kind == TYPE_ARRAY &&
        init_type->kind == TYPE_ARRAY &&
        init_type->as.array.element_type->kind == TYPE_ARRAY)
    {
        Type *decl_inner = decl_type->as.array.element_type;
        Type *init_inner = init_type->as.array.element_type;
        TypeKind decl_elem = decl_inner->as.array.element_type->kind;
        TypeKind init_elem = init_inner->as.array.element_type->kind;

        bool needs_coercion = init_elem == TYPE_INT &&
            (decl_elem == TYPE_BYTE || decl_elem == TYPE_INT32 ||
             decl_elem == TYPE_UINT32 || decl_elem == TYPE_UINT ||
             decl_elem == TYPE_FLOAT);

        if (!needs_coercion && init_elem == TYPE_DOUBLE && decl_elem == TYPE_FLOAT)
        {
            needs_coercion = true;
        }

        if (needs_coercion)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            Expr *init = stmt->as.var_decl.initializer;
            if (init->type == EXPR_ARRAY)
            {
                for (int i = 0; i < init->as.array.element_count; i++)
                {
                    Expr *elem = init->as.array.elements[i];
                    if (elem->type == EXPR_ARRAY)
                    {
                        elem->expr_type = decl_inner;
                    }
                }
            }
            *init_type_ptr = decl_type;
            return true;
        }
    }

    return false;
}

bool check_var_type_compatibility(Type *decl_type, Type *init_type, Stmt *stmt)
{
    /* Allow assigning any concrete type to an 'any' variable (boxing) */
    bool types_compatible = ast_type_equals(init_type, decl_type) ||
                           (decl_type->kind == TYPE_ANY && init_type != NULL) ||
                           (init_type != NULL && init_type->kind == TYPE_NIL &&
                            (decl_type->kind == TYPE_POINTER || decl_type->kind == TYPE_STRING ||
                             decl_type->kind == TYPE_ARRAY || decl_type->kind == TYPE_FUNCTION));

    /* Check for any[] assignment compatibility at any nesting level */
    if (!types_compatible && decl_type->kind == TYPE_ARRAY &&
        init_type != NULL && init_type->kind == TYPE_ARRAY)
    {
        Type *decl_elem = decl_type->as.array.element_type;
        Type *init_elem = init_type->as.array.element_type;

        while (decl_elem != NULL && init_elem != NULL &&
               decl_elem->kind == TYPE_ARRAY && init_elem->kind == TYPE_ARRAY)
        {
            decl_elem = decl_elem->as.array.element_type;
            init_elem = init_elem->as.array.element_type;
        }

        if (decl_elem != NULL && decl_elem->kind == TYPE_ANY && init_elem != NULL)
        {
            types_compatible = true;
        }
        else if (decl_elem != NULL && init_elem != NULL &&
                 is_numeric_type(decl_elem) && is_numeric_type(init_elem) &&
                 decl_elem->kind != TYPE_DOUBLE && decl_elem->kind != TYPE_FLOAT &&
                 init_elem->kind != TYPE_DOUBLE && init_elem->kind != TYPE_FLOAT)
        {
            types_compatible = true;
        }
    }

    /* Allow int literal to be assigned to char variable */
    if (!types_compatible && decl_type->kind == TYPE_CHAR && init_type != NULL &&
        (init_type->kind == TYPE_INT || init_type->kind == TYPE_BYTE))
    {
        types_compatible = true;
        stmt->as.var_decl.initializer->expr_type = decl_type;
    }

    /* Allow numeric widening promotions */
    if (!types_compatible && decl_type != NULL && init_type != NULL &&
        can_promote_numeric(init_type, decl_type))
    {
        types_compatible = true;
    }

    return types_compatible;
}
