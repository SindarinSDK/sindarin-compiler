/**
 * code_gen_expr_call_byte_array.c - Code generation for byte array method calls
 *
 * Handles byte[] specific methods: toString, toStringLatin1, toHex, toBase64.
 * These methods convert byte arrays to string representations.
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Generate code for byte array method calls.
 * Only applies to arrays with element type TYPE_BYTE.
 * Returns generated C code string, or NULL if not a byte array method.
 *
 * These runtime functions now return RtHandleV2*. The caller
 * (code_gen_array_method_call) handles converting to the appropriate
 * form based on handle_mode.
 */
char *code_gen_byte_array_method_call(CodeGen *gen, const char *method_name,
                                       char *object_str, int arg_count)
{
    /* byte[].toString() - UTF-8 decoding (returns RtHandleV2*) */
    if (strcmp(method_name, "toString") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_byte_array_to_string(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    /* byte[].toStringLatin1() - Latin-1/ISO-8859-1 decoding (returns RtHandleV2*) */
    if (strcmp(method_name, "toStringLatin1") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_byte_array_to_string_latin1(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    /* byte[].toHex() - hexadecimal encoding (returns RtHandleV2*) */
    if (strcmp(method_name, "toHex") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_byte_array_to_hex(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    /* byte[].toBase64() - Base64 encoding (returns RtHandleV2*) */
    if (strcmp(method_name, "toBase64") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_byte_array_to_base64(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    return NULL;
}
