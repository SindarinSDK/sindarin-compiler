#include "code_gen/code_gen_expr_incr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>

char *code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_increment_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, expr->as.operand->as.variable.name));
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);

    /* For sync variables, use atomic increment */
    if (symbol && symbol->sync_mod == SYNC_ATOMIC)
    {
        return arena_sprintf(gen->arena, "__atomic_fetch_add(&%s, 1, __ATOMIC_SEQ_CST)", var_name);
    }

    /* For char/byte types, use inline increment to avoid type mismatch
     * (rt_post_inc_long reads 8 bytes, but char/byte are 1 byte) */
    if (symbol && symbol->type &&
        (symbol->type->kind == TYPE_CHAR || symbol->type->kind == TYPE_BYTE))
    {
        if (symbol->mem_qual == MEM_AS_REF)
        {
            return arena_sprintf(gen->arena, "(*%s)++", var_name);
        }
        return arena_sprintf(gen->arena, "%s++", var_name);
    }

    // For 'as ref' variables, they're already pointers, so pass directly
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_inc_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_inc_long(&%s)", var_name);
}

char *code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_decrement_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, expr->as.operand->as.variable.name));
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);

    /* For sync variables, use atomic decrement */
    if (symbol && symbol->sync_mod == SYNC_ATOMIC)
    {
        return arena_sprintf(gen->arena, "__atomic_fetch_sub(&%s, 1, __ATOMIC_SEQ_CST)", var_name);
    }

    /* For char/byte types, use inline decrement to avoid type mismatch
     * (rt_post_dec_long reads 8 bytes, but char/byte are 1 byte) */
    if (symbol && symbol->type &&
        (symbol->type->kind == TYPE_CHAR || symbol->type->kind == TYPE_BYTE))
    {
        if (symbol->mem_qual == MEM_AS_REF)
        {
            return arena_sprintf(gen->arena, "(*%s)--", var_name);
        }
        return arena_sprintf(gen->arena, "%s--", var_name);
    }

    // For 'as ref' variables, they're already pointers, so pass directly
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_dec_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_dec_long(&%s)", var_name);
}
