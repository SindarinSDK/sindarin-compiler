#include "code_gen/code_gen_util.h"

/* ============================================================================
 * Native C Operator Generation for Unchecked Arithmetic Mode
 * ============================================================================
 * These functions generate native C operators instead of runtime function calls
 * when ARITH_UNCHECKED mode is enabled. This eliminates function call overhead
 * but removes overflow checking.
 */

const char *get_native_c_operator(SnTokenType op)
{
    switch (op)
    {
    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_SLASH:
        return "/";
    case TOKEN_MODULO:
        return "%";
    case TOKEN_EQUAL_EQUAL:
        return "==";
    case TOKEN_BANG_EQUAL:
        return "!=";
    case TOKEN_LESS:
        return "<";
    case TOKEN_LESS_EQUAL:
        return "<=";
    case TOKEN_GREATER:
        return ">";
    case TOKEN_GREATER_EQUAL:
        return ">=";
    default:
        return NULL;
    }
}

bool can_use_native_operator(SnTokenType op)
{
    /* Division and modulo still need runtime functions for zero-division check.
       All other arithmetic and comparison operators can use native C operators. */
    switch (op)
    {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
        return true;
    case TOKEN_SLASH:
    case TOKEN_MODULO:
        /* Keep runtime functions for division/modulo to check for zero */
        return false;
    default:
        return false;
    }
}

char *gen_native_arithmetic(CodeGen *gen, const char *left_str, const char *right_str,
                            SnTokenType op, Type *type)
{
    /* Booleans always use native C operators (no overflow issues) */
    if (type->kind == TYPE_BOOL && can_use_native_operator(op))
    {
        const char *c_op = get_native_c_operator(op);
        if (c_op != NULL)
        {
            return arena_sprintf(gen->arena, "((%s) %s (%s))", left_str, c_op, right_str);
        }
    }

    if (gen->arithmetic_mode != ARITH_UNCHECKED)
    {
        return NULL;  /* Not in unchecked mode, use runtime functions */
    }

    if (!can_use_native_operator(op))
    {
        return NULL;  /* This operator needs runtime function (div/mod) */
    }

    const char *c_op = get_native_c_operator(op);
    if (c_op == NULL)
    {
        return NULL;  /* Unknown operator */
    }

    /* Generate native C expression */
    if (type->kind == TYPE_DOUBLE || type->kind == TYPE_FLOAT)
    {
        /* For doubles/floats, no suffix needed */
        return arena_sprintf(gen->arena, "((%s) %s (%s))", left_str, c_op, right_str);
    }
    else if (type->kind == TYPE_INT || type->kind == TYPE_LONG ||
             type->kind == TYPE_INT32 || type->kind == TYPE_UINT ||
             type->kind == TYPE_UINT32)
    {
        /* For integers, result is long long (64-bit on all platforms) */
        return arena_sprintf(gen->arena, "((long long)((%s) %s (%s)))", left_str, c_op, right_str);
    }

    return NULL;  /* Unsupported type */
}

char *gen_native_unary(CodeGen *gen, const char *operand_str, SnTokenType op, Type *type)
{
    if (gen->arithmetic_mode != ARITH_UNCHECKED)
    {
        return NULL;  /* Not in unchecked mode, use runtime functions */
    }

    switch (op)
    {
    case TOKEN_MINUS:
        if (type->kind == TYPE_DOUBLE || type->kind == TYPE_FLOAT)
        {
            return arena_sprintf(gen->arena, "(-(%s))", operand_str);
        }
        else if (type->kind == TYPE_INT || type->kind == TYPE_LONG ||
                 type->kind == TYPE_INT32 || type->kind == TYPE_UINT ||
                 type->kind == TYPE_UINT32)
        {
            return arena_sprintf(gen->arena, "((long long)(-(%s)))", operand_str);
        }
        break;
    case TOKEN_BANG:
        return arena_sprintf(gen->arena, "(!(%s))", operand_str);
    case TOKEN_TILDE:
        return arena_sprintf(gen->arena, "((long long)(~(%s)))", operand_str);
    default:
        break;
    }

    return NULL;  /* Unsupported operator or type */
}

/* ============================================================================
 * Arena Destination Calculation for Scope Escape
 * ============================================================================ */

/**
 * Get the arena at a specific depth in the arena stack.
 * Depth 0 is the outermost (function-level) arena, depth 1 is the first nested
 * private block, etc.
 *
 * Returns NULL if the depth is out of range.
 */
const char *get_arena_at_depth(CodeGen *gen, int depth)
{
    if (gen == NULL || depth < 0)
    {
        return NULL;
    }

    /* Depth 0 corresponds to the function's base arena (__arena__) */
    if (depth == 0)
    {
        return "__arena__";
    }

    /* Depths 1+ correspond to nested private block arenas in the stack.
     * Stack index 0 = depth 1, stack index 1 = depth 2, etc. */
    int stack_index = depth - 1;
    if (stack_index < gen->arena_stack_depth)
    {
        return gen->arena_stack[stack_index];
    }

    return NULL;
}

/**
 * Calculate the number of arena levels to traverse when escaping from
 * source_depth to target_depth.
 *
 * This is used to determine how many rt_arena_get_parent() calls are needed
 * to get from the current arena to the destination arena.
 *
 * Returns 0 if no traversal is needed (same scope or target is deeper).
 */
int calculate_arena_traversal_depth(CodeGen *gen, int source_depth, int target_depth)
{
    if (gen == NULL)
    {
        return 0;
    }

    /* If source is not deeper than target, no traversal needed */
    if (source_depth <= target_depth)
    {
        return 0;
    }

    /* Calculate the number of levels to go up */
    return source_depth - target_depth;
}

/**
 * Calculate the target arena for an escaping allocation based on scope depth.
 *
 * When a struct or allocation escapes from an inner scope (source_depth) to an
 * outer scope (target_depth), this function determines which arena to allocate
 * in to ensure the value lives long enough.
 *
 * For multi-level nesting (e.g., inner block -> middle block -> outer function),
 * this function either:
 * 1. Returns the arena variable name directly if it's in the stack
 * 2. Generates a parent chain traversal expression if needed
 *
 * Examples:
 * - source_depth=3, target_depth=1 -> returns arena at depth 1
 * - source_depth=2, target_depth=0 -> returns "__arena__" (function level)
 * - source_depth=1, target_depth=1 -> returns current arena (no escape)
 */
const char *get_arena_for_scope_escape(CodeGen *gen, int source_depth, int target_depth)
{
    if (gen == NULL)
    {
        return "NULL";
    }

    /* If source is not deeper than target, no escape - use current arena */
    if (source_depth <= target_depth)
    {
        return gen->current_arena_var ? gen->current_arena_var : "NULL";
    }

    /* Escaping to global/module scope (target_depth <= 0) */
    if (target_depth <= 0)
    {
        return "NULL";
    }

    /* Try to get the arena directly from the stack if it's a known level.
     * The arena_depth field tracks the current nesting level (1 = function,
     * 2+ = nested private blocks). We need to map target_depth to an arena. */

    /* In the code generator:
     * - arena_depth = 1 means we're in a function with __arena__
     * - arena_depth = 2 means we're in first nested private block (__arena_2__)
     * - etc.
     *
     * The arena_stack contains names of nested private block arenas (not including
     * the function's base arena). So:
     * - arena_stack[0] = first nested private block arena
     * - arena_stack[1] = second nested private block arena
     * - etc.
     */

    /* If the target depth corresponds to the function level, return __arena__ */
    if (target_depth == 1)
    {
        return "__arena__";
    }

    /* For nested levels, calculate the stack index */
    int stack_index = target_depth - 2;  /* -2 because depth 2 = stack[0] */

    if (stack_index >= 0 && stack_index < gen->arena_stack_depth)
    {
        return gen->arena_stack[stack_index];
    }

    /* If we can't find the arena directly, generate a parent chain traversal.
     * This happens when the target arena isn't in our stack (e.g., crossing
     * function boundaries). Generate code like:
     * rt_arena_get_parent(rt_arena_get_parent(current_arena)) */
    int levels_up = source_depth - target_depth;
    if (levels_up > 0 && gen->current_arena_var != NULL)
    {
        char *result = arena_strdup(gen->arena, gen->current_arena_var);
        for (int i = 0; i < levels_up; i++)
        {
            result = arena_sprintf(gen->arena, "rt_arena_get_parent(%s)", result);
        }
        return result;
    }

    /* Fallback: return current arena or NULL */
    return gen->current_arena_var ? gen->current_arena_var : "NULL";
}
