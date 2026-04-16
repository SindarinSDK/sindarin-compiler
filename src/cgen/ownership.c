#include "cgen/ownership.h"

OwnershipKind ownership_kind(const Expr *src)
{
    if (!src) return OWNERSHIP_OWNED;

    switch (src->type)
    {
        /* OWNED — expression produces a fresh +1 credit. */
        case EXPR_CALL:
        case EXPR_METHOD_CALL:
        case EXPR_STATIC_CALL:
        case EXPR_STRUCT_LITERAL:
        case EXPR_ARRAY:
        case EXPR_ARRAY_SLICE:
        case EXPR_RANGE:
        case EXPR_SIZED_ARRAY_ALLOC:
        case EXPR_COPY_OF:
        case EXPR_INTERPOLATED:
        case EXPR_LAMBDA:
        case EXPR_THREAD_SPAWN:
        case EXPR_MATCH:
        case EXPR_BINARY:
        case EXPR_UNARY:
            return OWNERSHIP_OWNED;

        /* BORROW — expression reads through a live owner that remains live. */
        case EXPR_VARIABLE:
        case EXPR_MEMBER:
        case EXPR_MEMBER_ACCESS:
        case EXPR_ARRAY_ACCESS:
        case EXPR_VALUE_OF:
        case EXPR_ADDRESS_OF:
            return OWNERSHIP_BORROW;

        case EXPR_THREAD_SYNC:
            /* Handle-based sync (`handle!` where handle is a variable) aliases
             * the thread handle's result payload — both the handle var and the
             * destination would claim ownership of the same pointer unless the
             * destination acquires a fresh copy. Inline sync (`spawn f()!`)
             * returns a unique owned value — no acquire needed. */
            if (src->as.thread_sync.handle &&
                src->as.thread_sync.handle->type == EXPR_VARIABLE)
                return OWNERSHIP_BORROW;
            return OWNERSHIP_OWNED;

        case EXPR_LITERAL:
            /* nil literals produce a NULL pointer; there is nothing to acquire.
             * Classify as OWNED so acquire-emission sites skip strdup(NULL) /
             * sn_array_copy(NULL) / __sn__T_retain(NULL). */
            if (src->as.literal.type && src->as.literal.type->kind == TYPE_NIL)
                return OWNERSHIP_OWNED;
            return OWNERSHIP_BORROW;

        /* Side-effecting forms rarely appear as acquire-context sources; classify
         * conservatively as BORROW so callers emit an acquire if the destination
         * type is heap-owned. Safe default: a spurious retain/copy is a perf
         * cost but not a correctness bug, while a missed acquire is UAF. */
        case EXPR_ASSIGN:
        case EXPR_INDEX_ASSIGN:
        case EXPR_COMPOUND_ASSIGN:
        case EXPR_MEMBER_ASSIGN:
        case EXPR_INCREMENT:
        case EXPR_DECREMENT:
        case EXPR_SPREAD:
        case EXPR_THREAD_DETACH:
        case EXPR_SYNC_LIST:
        case EXPR_SIZEOF:
        case EXPR_TYPEOF:
            return OWNERSHIP_BORROW;
    }
    return OWNERSHIP_BORROW;
}

const char *ownership_kind_str(OwnershipKind k)
{
    switch (k)
    {
        case OWNERSHIP_BORROW: return "borrow";
        case OWNERSHIP_OWNED:  return "owned";
    }
    return "borrow";
}
