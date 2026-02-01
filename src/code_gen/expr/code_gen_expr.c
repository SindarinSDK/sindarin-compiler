#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/expr/code_gen_expr_core.h"
#include "code_gen/expr/code_gen_expr_binary.h"
#include "code_gen/expr/lambda/code_gen_expr_lambda.h"
#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/thread/code_gen_expr_thread.h"
#include "code_gen/expr/code_gen_expr_array.h"
#include "code_gen/expr/code_gen_expr_static.h"
#include "code_gen/expr/code_gen_expr_string.h"
#include "code_gen/expr/code_gen_expr_incr.h"
#include "code_gen/expr/code_gen_expr_member.h"
#include "code_gen/expr/code_gen_expr_misc.h"
#include "code_gen/expr/code_gen_expr_type.h"
#include "code_gen/expr/code_gen_expr_struct.h"
#include "code_gen/expr/code_gen_expr_access.h"
#include "code_gen/expr/code_gen_expr_match.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *code_gen_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_expression");
    if (expr == NULL)
    {
        return arena_strdup(gen->arena, "0L");
    }
    switch (expr->type)
    {
    case EXPR_BINARY:
        return code_gen_binary_expression(gen, &expr->as.binary);
    case EXPR_UNARY:
        return code_gen_unary_expression(gen, &expr->as.unary);
    case EXPR_LITERAL:
        return code_gen_literal_expression(gen, &expr->as.literal);
    case EXPR_VARIABLE:
        return code_gen_variable_expression(gen, &expr->as.variable);
    case EXPR_ASSIGN:
        return code_gen_assign_expression(gen, &expr->as.assign);
    case EXPR_INDEX_ASSIGN:
        return code_gen_index_assign_expression(gen, &expr->as.index_assign);
    case EXPR_CALL:
        return code_gen_call_expression(gen, expr);
    case EXPR_ARRAY:
        return code_gen_array_expression(gen, expr);
    case EXPR_ARRAY_ACCESS:
        return code_gen_array_access_expression(gen, &expr->as.array_access);
    case EXPR_INCREMENT:
        return code_gen_increment_expression(gen, expr);
    case EXPR_DECREMENT:
        return code_gen_decrement_expression(gen, expr);
    case EXPR_INTERPOLATED:
        return code_gen_interpolated_expression(gen, &expr->as.interpol);
    case EXPR_MEMBER:
        return code_gen_member_expression(gen, expr);
    case EXPR_ARRAY_SLICE:
        return code_gen_array_slice_expression(gen, expr);
    case EXPR_RANGE:
        return code_gen_range_expression(gen, expr);
    case EXPR_SPREAD:
        return code_gen_spread_expression(gen, expr);
    case EXPR_LAMBDA:
        return code_gen_lambda_expression(gen, expr);
    case EXPR_STATIC_CALL:
        return code_gen_static_call_expression(gen, expr);
    case EXPR_SIZED_ARRAY_ALLOC:
        return code_gen_sized_array_alloc_expression(gen, expr);
    case EXPR_THREAD_SPAWN:
        return code_gen_thread_spawn_expression(gen, expr);
    case EXPR_THREAD_SYNC:
        return code_gen_thread_sync_expression(gen, expr);
    case EXPR_SYNC_LIST:
        /* Sync lists are only valid as part of thread sync [r1, r2]! */
        fprintf(stderr, "Error: Sync list without sync operator\n");
        exit(1);
    case EXPR_AS_VAL:
        return code_gen_as_val_expression(gen, expr);
    case EXPR_AS_REF:
        return code_gen_as_ref_expression(gen, expr);
    case EXPR_TYPEOF:
        return code_gen_typeof_expression(gen, expr);
    case EXPR_IS:
        return code_gen_is_expression(gen, expr);
    case EXPR_AS_TYPE:
        return code_gen_as_type_expression(gen, expr);
    case EXPR_STRUCT_LITERAL:
        return code_gen_struct_literal_expression(gen, expr);
    case EXPR_MEMBER_ACCESS:
        return code_gen_member_access_expression(gen, expr);
    case EXPR_MEMBER_ASSIGN:
        return code_gen_member_assign_expression(gen, expr);
    case EXPR_SIZEOF:
        return code_gen_sizeof_expression(gen, expr);
    case EXPR_COMPOUND_ASSIGN:
        return code_gen_compound_assign_expression(gen, expr);
    case EXPR_METHOD_CALL:
        return code_gen_method_call_expression(gen, expr);
    case EXPR_MATCH:
        return code_gen_match_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
