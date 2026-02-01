#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_expr_ops.h"
#include "type_checker/type_checker_expr_assign.h"
#include "type_checker/type_checker_expr_member.h"
#include "type_checker/type_checker_expr_thread.h"
#include "type_checker/type_checker_expr_basic.h"
#include "type_checker/type_checker_expr_cast.h"
#include "type_checker/type_checker_expr_struct.h"
#include "type_checker/type_checker_expr_access.h"
#include "type_checker/type_checker_expr_misc.h"
#include "type_checker/type_checker_expr_call.h"
#include "type_checker/type_checker_expr_call_array.h"
#include "type_checker/type_checker_expr_call_char.h"
#include "type_checker/type_checker_expr_call_string.h"
#include "type_checker/type_checker_expr_array.h"
#include "type_checker/type_checker_expr_lambda.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "symbol_table/symbol_table_thread.h"
#include "debug.h"

Type *type_check_expr(Expr *expr, SymbolTable *table)
{
    if (expr == NULL)
    {
        DEBUG_VERBOSE("Expression is NULL");
        return NULL;
    }
    if (expr->expr_type)
    {
        DEBUG_VERBOSE("Using cached expression type: %d", expr->expr_type->kind);
        return expr->expr_type;
    }

    Type *t = NULL;
    DEBUG_VERBOSE("Type checking expression type: %d", expr->type);

    switch (expr->type)
    {
    case EXPR_BINARY:
        t = type_check_binary(expr, table);
        break;
    case EXPR_UNARY:
        t = type_check_unary(expr, table);
        break;
    case EXPR_LITERAL:
        t = type_check_literal(expr, table);
        break;
    case EXPR_VARIABLE:
        t = type_check_variable(expr, table);
        break;
    case EXPR_ASSIGN:
        t = type_check_assign(expr, table);
        break;
    case EXPR_INDEX_ASSIGN:
        t = type_check_index_assign(expr, table);
        break;
    case EXPR_CALL:
        t = type_check_call_expression(expr, table);
        break;
    case EXPR_ARRAY:
        t = type_check_array(expr, table);
        break;
    case EXPR_ARRAY_ACCESS:
        t = type_check_array_access(expr, table);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        t = type_check_increment_decrement(expr, table);
        break;
    case EXPR_INTERPOLATED:
        t = type_check_interpolated(expr, table);
        break;
    case EXPR_MEMBER:
        t = type_check_member(expr, table);
        break;
    case EXPR_ARRAY_SLICE:
        t = type_check_array_slice(expr, table);
        break;
    case EXPR_RANGE:
        t = type_check_range(expr, table);
        break;
    case EXPR_SPREAD:
        t = type_check_spread(expr, table);
        break;
    case EXPR_LAMBDA:
        t = type_check_lambda(expr, table);
        break;
    case EXPR_STATIC_CALL:
        t = type_check_static_method_call(expr, table);
        break;
    case EXPR_METHOD_CALL:
        /* Method calls are handled via EXPR_MEMBER + EXPR_CALL pattern.
         * If we reach here directly, it's a type error. */
        type_error(expr->token, "Internal error: EXPR_METHOD_CALL reached directly");
        t = NULL;
        break;
    case EXPR_SIZED_ARRAY_ALLOC:
        t = type_check_sized_array_alloc(expr, table);
        break;
    case EXPR_THREAD_SPAWN:
        t = type_check_thread_spawn(expr, table);
        break;
    case EXPR_THREAD_SYNC:
        t = type_check_thread_sync(expr, table);
        break;
    case EXPR_SYNC_LIST:
        /* Sync lists are only valid as part of thread sync: [r1, r2]!
         * A standalone sync list is a type error */
        type_error(expr->token, "Sync list [r1, r2, ...] must be followed by '!' for synchronization");
        t = NULL;
        break;
    case EXPR_AS_VAL:
        t = type_check_as_val(expr, table);
        break;
    case EXPR_AS_REF:
        t = type_check_as_ref(expr, table);
        break;
    case EXPR_TYPEOF:
        t = type_check_typeof(expr, table);
        break;
    case EXPR_IS:
        t = type_check_is(expr, table);
        break;
    case EXPR_AS_TYPE:
        t = type_check_as_type(expr, table);
        break;
    case EXPR_STRUCT_LITERAL:
        t = type_check_struct_literal(expr, table);
        break;
    case EXPR_MEMBER_ACCESS:
        t = type_check_member_access(expr, table);
        break;
    case EXPR_MEMBER_ASSIGN:
        t = type_check_member_assign(expr, table);
        break;
    case EXPR_SIZEOF:
        t = type_check_sizeof(expr, table);
        break;
    case EXPR_COMPOUND_ASSIGN:
        t = type_check_compound_assign(expr, table);
        break;
    case EXPR_MATCH:
        t = type_check_match(expr, table);
        break;
    }

    expr->expr_type = t;
    if (t != NULL)
    {
        DEBUG_VERBOSE("Expression type check result: %d", t->kind);
    }
    else
    {
        DEBUG_VERBOSE("Expression type check failed: NULL type");
    }
    return t;
}
