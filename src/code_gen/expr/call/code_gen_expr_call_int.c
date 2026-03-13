/**
 * code_gen_expr_call_int.c - Code generation for int method calls
 *
 * Handles int type methods: toDouble, toLong, toUint, toByte, toChar
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include <string.h>

char *code_gen_int_method_call(CodeGen *gen, const char *method_name,
                               Expr *object, int arg_count)
{
    char *object_str = code_gen_expression(gen, object);

    if (strcmp(method_name, "toDouble") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((double)%s)", object_str);
    }

    if (strcmp(method_name, "toLong") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((long long)%s)", object_str);
    }

    if (strcmp(method_name, "toUint") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((unsigned long long)%s)", object_str);
    }

    if (strcmp(method_name, "toByte") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((unsigned char)%s)", object_str);
    }

    if (strcmp(method_name, "toChar") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((char)%s)", object_str);
    }

    return NULL;
}
