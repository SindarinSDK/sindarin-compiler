/**
 * code_gen_expr_call_long.c - Code generation for long method calls
 *
 * Handles long type methods: toInt, toDouble
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include <string.h>

char *code_gen_long_method_call(CodeGen *gen, const char *method_name,
                                Expr *object, int arg_count)
{
    char *object_str = code_gen_expression(gen, object);

    if (strcmp(method_name, "toInt") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((long long)%s)", object_str);
    }

    if (strcmp(method_name, "toDouble") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((double)%s)", object_str);
    }

    return NULL;
}
