#include "type_checker_util.h"
#include "type_checker_util_escape.h"
#include "type_checker_util_suggest.h"
#include "type_checker_util_struct.h"
#include "type_checker_util_layout.h"
#include "diagnostic.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int had_type_error = 0;
static int native_context_depth = 0;

void type_checker_reset_error(void)
{
    had_type_error = 0;
}

int type_checker_had_error(void)
{
    return had_type_error;
}

void type_checker_set_error(void)
{
    had_type_error = 1;
}

const char *type_name(Type *type)
{
    if (type == NULL) return "unknown";
    switch (type->kind) {
        case TYPE_INT:         return "int";
        case TYPE_INT32:       return "int32";
        case TYPE_UINT:        return "uint";
        case TYPE_UINT32:      return "uint32";
        case TYPE_LONG:        return "long";
        case TYPE_DOUBLE:      return "double";
        case TYPE_FLOAT:       return "float";
        case TYPE_CHAR:        return "char";
        case TYPE_STRING:      return "str";
        case TYPE_BOOL:        return "bool";
        case TYPE_BYTE:        return "byte";
        case TYPE_VOID:        return "void";
        case TYPE_NIL:         return "nil";
        case TYPE_ANY:         return "any";
        case TYPE_ARRAY:       return "array";
        case TYPE_FUNCTION:    return "function";
        case TYPE_POINTER:      return "pointer";
        default:                return "unknown";
    }
}

void type_error(Token *token, const char *msg)
{
    diagnostic_error_at(token, "%s", msg);
    had_type_error = 1;
}

void type_error_with_suggestion(Token *token, const char *msg, const char *suggestion)
{
    diagnostic_error_with_suggestion(token, suggestion, "%s", msg);
    had_type_error = 1;
}

void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context)
{
    diagnostic_error_at(token, "type mismatch in %s: expected '%s', got '%s'",
                        context, type_name(expected), type_name(actual));
    had_type_error = 1;
}

bool is_numeric_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_INT32 ||
                           type->kind == TYPE_UINT || type->kind == TYPE_UINT32 ||
                           type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE ||
                           type->kind == TYPE_FLOAT || type->kind == TYPE_BYTE ||
                           type->kind == TYPE_CHAR);
    DEBUG_VERBOSE("Checking if type is numeric: %s", result ? "true" : "false");
    return result;
}

bool is_comparison_operator(SnTokenType op)
{
    bool result = op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS ||
                  op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
    DEBUG_VERBOSE("Checking if operator is comparison: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_arithmetic_operator(SnTokenType op)
{
    bool result = op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
    DEBUG_VERBOSE("Checking if operator is arithmetic: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_printable_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_INT32 ||
                           type->kind == TYPE_UINT || type->kind == TYPE_UINT32 ||
                           type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE ||
                           type->kind == TYPE_FLOAT || type->kind == TYPE_CHAR ||
                           type->kind == TYPE_STRING || type->kind == TYPE_BOOL ||
                           type->kind == TYPE_BYTE || type->kind == TYPE_ARRAY ||
                           type->kind == TYPE_ANY || type->kind == TYPE_STRUCT);
    DEBUG_VERBOSE("Checking if type is printable: %s", result ? "true" : "false");
    return result;
}

/* Check if a type can be passed as a variadic argument.
 * Per spec: primitives, str, and pointer types are allowed.
 * Arrays cannot be passed as variadic arguments. */
bool is_variadic_compatible_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_INT ||
                  type->kind == TYPE_INT32 ||
                  type->kind == TYPE_UINT ||
                  type->kind == TYPE_UINT32 ||
                  type->kind == TYPE_LONG ||
                  type->kind == TYPE_DOUBLE ||
                  type->kind == TYPE_FLOAT ||
                  type->kind == TYPE_CHAR ||
                  type->kind == TYPE_BOOL ||
                  type->kind == TYPE_BYTE ||
                  type->kind == TYPE_STRING ||
                  type->kind == TYPE_POINTER;
    DEBUG_VERBOSE("Checking if type is variadic-compatible: %s", result ? "true" : "false");
    return result;
}

bool is_primitive_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_INT ||
                  type->kind == TYPE_INT32 ||
                  type->kind == TYPE_UINT ||
                  type->kind == TYPE_UINT32 ||
                  type->kind == TYPE_LONG ||
                  type->kind == TYPE_DOUBLE ||
                  type->kind == TYPE_FLOAT ||
                  type->kind == TYPE_CHAR ||
                  type->kind == TYPE_BOOL ||
                  type->kind == TYPE_BYTE ||
                  type->kind == TYPE_VOID;
    DEBUG_VERBOSE("Checking if type is primitive: %s", result ? "true" : "false");
    return result;
}

bool is_reference_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_STRING ||
                  type->kind == TYPE_ARRAY ||
                  type->kind == TYPE_FUNCTION;
    DEBUG_VERBOSE("Checking if type is reference: %s", result ? "true" : "false");
    return result;
}

void memory_context_init(MemoryContext *ctx)
{
    ctx->in_private_block = false;
    ctx->in_private_function = false;
    ctx->private_depth = 0;
    ctx->scope_depth = 0;
}

void memory_context_enter_private(MemoryContext *ctx)
{
    ctx->in_private_block = true;
    ctx->private_depth++;
}

void memory_context_exit_private(MemoryContext *ctx)
{
    ctx->private_depth--;
    if (ctx->private_depth <= 0)
    {
        ctx->in_private_block = false;
        ctx->private_depth = 0;
    }
}

bool memory_context_is_private(MemoryContext *ctx)
{
    return ctx->in_private_block || ctx->in_private_function;
}

void memory_context_enter_scope(MemoryContext *ctx)
{
    if (ctx != NULL)
    {
        ctx->scope_depth++;
        DEBUG_VERBOSE("Entering scope in memory context (depth: %d)", ctx->scope_depth);
    }
}

void memory_context_exit_scope(MemoryContext *ctx)
{
    if (ctx != NULL && ctx->scope_depth > 0)
    {
        ctx->scope_depth--;
        DEBUG_VERBOSE("Exiting scope in memory context (depth: %d)", ctx->scope_depth);
    }
}

int memory_context_get_scope_depth(MemoryContext *ctx)
{
    if (ctx == NULL)
    {
        return 0;
    }
    return ctx->scope_depth;
}

bool can_promote_numeric(Type *from, Type *to)
{
    if (from == NULL || to == NULL) return false;
    /* int can promote to double or long */
    if (from->kind == TYPE_INT && (to->kind == TYPE_DOUBLE || to->kind == TYPE_LONG))
        return true;
    /* long can promote to double */
    if (from->kind == TYPE_LONG && to->kind == TYPE_DOUBLE)
        return true;
    /* float can promote to double */
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_DOUBLE)
        return true;
    /* int32 can promote to float or double */
    if (from->kind == TYPE_INT32 && (to->kind == TYPE_FLOAT || to->kind == TYPE_DOUBLE))
        return true;
    /* uint32 can promote to float or double */
    if (from->kind == TYPE_UINT32 && (to->kind == TYPE_FLOAT || to->kind == TYPE_DOUBLE))
        return true;
    /* uint can promote to double or long */
    if (from->kind == TYPE_UINT && (to->kind == TYPE_DOUBLE || to->kind == TYPE_LONG))
        return true;
    return false;
}

Type *get_promoted_type(Arena *arena, Type *left, Type *right)
{
    if (left == NULL || right == NULL) return NULL;

    /* Check for numeric type promotion FIRST (before ast_type_equals check) */
    /* because ast_type_equals treats compatible types as equal */
    if (is_numeric_type(left) && is_numeric_type(right))
    {
        /* double is the widest numeric type */
        if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE)
            return ast_create_primitive_type(arena, TYPE_DOUBLE);
        /* float is wider than integer types */
        if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT)
            return ast_create_primitive_type(arena, TYPE_FLOAT);
        /* long is wider than int/uint */
        if (left->kind == TYPE_LONG || right->kind == TYPE_LONG)
            return ast_create_primitive_type(arena, TYPE_LONG);
        /* If both are the exact same type, return it */
        if (left->kind == right->kind)
            return left;
        /* int32 and uint32 stay as their own types (fixed-width) */
        if ((left->kind == TYPE_INT32 && right->kind == TYPE_INT32) ||
            (left->kind == TYPE_UINT32 && right->kind == TYPE_UINT32))
            return left;
        /* int and uint stay as their own types */
        if ((left->kind == TYPE_INT && right->kind == TYPE_INT) ||
            (left->kind == TYPE_UINT && right->kind == TYPE_UINT))
            return left;
        /* int promotes to int32/uint32 when mixed */
        if (left->kind == TYPE_INT32 && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_INT32)
            return right;
        if (left->kind == TYPE_UINT32 && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_UINT32)
            return right;
        /* int promotes to uint when mixed */
        if (left->kind == TYPE_UINT && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_UINT)
            return right;
        /* byte and char promote to int in arithmetic operations */
        if (left->kind == TYPE_BYTE || left->kind == TYPE_CHAR ||
            right->kind == TYPE_BYTE || right->kind == TYPE_CHAR)
            return ast_create_primitive_type(arena, TYPE_INT);
        /* Mixed int/uint - no automatic promotion for other cases */
        return NULL;
    }

    /* If both are the same type, return it */
    if (ast_type_equals(left, right))
        return left;

    /* No valid promotion */
    return NULL;
}

/* ============================================================================
 * Native Function Context Tracking
 * ============================================================================ */

void native_context_enter(void)
{
    native_context_depth++;
    DEBUG_VERBOSE("Entering native function context (depth: %d)", native_context_depth);
}

void native_context_exit(void)
{
    if (native_context_depth > 0)
    {
        native_context_depth--;
        DEBUG_VERBOSE("Exiting native function context (depth: %d)", native_context_depth);
    }
}

bool native_context_is_active(void)
{
    return native_context_depth > 0;
}

/* ============================================================================
 * Method Context Tracking (for struct methods with pointer 'self')
 * ============================================================================ */

static int method_context_depth = 0;

void method_context_enter(void)
{
    method_context_depth++;
    DEBUG_VERBOSE("Entering method context (depth: %d)", method_context_depth);
}

void method_context_exit(void)
{
    if (method_context_depth > 0)
    {
        method_context_depth--;
        DEBUG_VERBOSE("Exiting method context (depth: %d)", method_context_depth);
    }
}

bool method_context_is_active(void)
{
    return method_context_depth > 0;
}

/* ============================================================================
 * 'as val' Operand Context Tracking
 * ============================================================================ */

static int as_val_context_depth = 0;

void as_val_context_enter(void)
{
    as_val_context_depth++;
    DEBUG_VERBOSE("Entering 'as val' context (depth: %d)", as_val_context_depth);
}

void as_val_context_exit(void)
{
    if (as_val_context_depth > 0)
    {
        as_val_context_depth--;
        DEBUG_VERBOSE("Exiting 'as val' context (depth: %d)", as_val_context_depth);
    }
}

bool as_val_context_is_active(void)
{
    return as_val_context_depth > 0;
}
