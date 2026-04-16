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
        case EXPR_THREAD_SYNC:
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
        case EXPR_LITERAL:
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
