/* Unit tests for the unified ownership classifier (src/cgen/ownership.c).
 *
 * Classifier operates on AST Expr nodes. No behavior change to codegen in
 * this commit — these tests lock in the classification rules that Commit 2
 * will route every ownership-sensitive emission edge through.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "cgen/ownership.h"
#include "../test_harness.h"

/* Minimal Expr-by-type factory. Only the `type` discriminator matters for
 * classification; the rest stays zeroed. */
static Expr make_expr(ExprType t)
{
    Expr e;
    memset(&e, 0, sizeof(e));
    e.type = t;
    return e;
}

static void test_owned_call_forms(void)
{
    Expr call        = make_expr(EXPR_CALL);
    Expr method_call = make_expr(EXPR_METHOD_CALL);
    Expr static_call = make_expr(EXPR_STATIC_CALL);
    assert(ownership_kind(&call)        == OWNERSHIP_OWNED);
    assert(ownership_kind(&method_call) == OWNERSHIP_OWNED);
    assert(ownership_kind(&static_call) == OWNERSHIP_OWNED);
}

static void test_owned_constructors(void)
{
    Expr struct_lit  = make_expr(EXPR_STRUCT_LITERAL);
    Expr array_lit   = make_expr(EXPR_ARRAY);
    Expr array_slice = make_expr(EXPR_ARRAY_SLICE);
    Expr range       = make_expr(EXPR_RANGE);
    Expr sized_alloc = make_expr(EXPR_SIZED_ARRAY_ALLOC);
    Expr copy_of     = make_expr(EXPR_COPY_OF);
    Expr interp      = make_expr(EXPR_INTERPOLATED);
    Expr lambda      = make_expr(EXPR_LAMBDA);
    Expr match_expr  = make_expr(EXPR_MATCH);
    assert(ownership_kind(&struct_lit)  == OWNERSHIP_OWNED);
    assert(ownership_kind(&array_lit)   == OWNERSHIP_OWNED);
    assert(ownership_kind(&array_slice) == OWNERSHIP_OWNED);
    assert(ownership_kind(&range)       == OWNERSHIP_OWNED);
    assert(ownership_kind(&sized_alloc) == OWNERSHIP_OWNED);
    assert(ownership_kind(&copy_of)     == OWNERSHIP_OWNED);
    assert(ownership_kind(&interp)      == OWNERSHIP_OWNED);
    assert(ownership_kind(&lambda)      == OWNERSHIP_OWNED);
    assert(ownership_kind(&match_expr)  == OWNERSHIP_OWNED);
}

static void test_owned_thread_forms(void)
{
    Expr spawn = make_expr(EXPR_THREAD_SPAWN);
    Expr sync  = make_expr(EXPR_THREAD_SYNC);
    assert(ownership_kind(&spawn) == OWNERSHIP_OWNED);
    assert(ownership_kind(&sync)  == OWNERSHIP_OWNED);
}

static void test_owned_operators(void)
{
    /* Binary/unary ops on heap types (string concat, array concat) allocate
     * fresh results; scalar ops return scalars and the classification is
     * moot. Classifying OWNED is safe for both. */
    Expr bin = make_expr(EXPR_BINARY);
    Expr un  = make_expr(EXPR_UNARY);
    assert(ownership_kind(&bin) == OWNERSHIP_OWNED);
    assert(ownership_kind(&un)  == OWNERSHIP_OWNED);
}

static void test_borrow_reads(void)
{
    Expr var         = make_expr(EXPR_VARIABLE);
    Expr member      = make_expr(EXPR_MEMBER);
    Expr member_acc  = make_expr(EXPR_MEMBER_ACCESS);
    Expr arr_access  = make_expr(EXPR_ARRAY_ACCESS);
    Expr value_of    = make_expr(EXPR_VALUE_OF);
    Expr address_of  = make_expr(EXPR_ADDRESS_OF);
    assert(ownership_kind(&var)        == OWNERSHIP_BORROW);
    assert(ownership_kind(&member)     == OWNERSHIP_BORROW);
    assert(ownership_kind(&member_acc) == OWNERSHIP_BORROW);
    assert(ownership_kind(&arr_access) == OWNERSHIP_BORROW);
    assert(ownership_kind(&value_of)   == OWNERSHIP_BORROW);
    assert(ownership_kind(&address_of) == OWNERSHIP_BORROW);
}

static void test_borrow_literals(void)
{
    /* String literals live in static storage — destination must strdup if it
     * wants to own. That matches BORROW semantics: source stays live. */
    Expr lit = make_expr(EXPR_LITERAL);
    assert(ownership_kind(&lit) == OWNERSHIP_BORROW);
}

static void test_null_source_is_safe(void)
{
    /* Defensive: NULL input must not crash. Picking OWNED avoids emitting a
     * spurious acquire on a nonexistent source. */
    assert(ownership_kind(NULL) == OWNERSHIP_OWNED);
}

static void test_kind_str_roundtrip(void)
{
    assert(strcmp(ownership_kind_str(OWNERSHIP_BORROW), "borrow") == 0);
    assert(strcmp(ownership_kind_str(OWNERSHIP_OWNED),  "owned")  == 0);
}

/* Regression anchors — each asserts the exact ownership class that the
 * current Phase-1-audited behavior relies on at a specific failing edge.
 * If any future change flips one of these, the regression fires at build
 * time, not at ASAN time. */

static void test_regression_queue_drain_take_is_owned(void)
{
    /* out.push(holder.take()) — the failing test's core.
     * holder.take() is EXPR_METHOD_CALL → must classify OWNED so Commit 2's
     * push emission neutralizes the chain_tmp's sn_auto instead of retaining. */
    Expr take_call = make_expr(EXPR_METHOD_CALL);
    assert(ownership_kind(&take_call) == OWNERSHIP_OWNED);
}

static void test_regression_literal_push_is_owned(void)
{
    /* items.push(Item { ... }) — must classify OWNED so Commit 2 consumes the
     * Item__new()'s +1 directly into the array slot without wrapping in
     * a retain that would leak. */
    Expr item_lit = make_expr(EXPR_STRUCT_LITERAL);
    assert(ownership_kind(&item_lit) == OWNERSHIP_OWNED);
}

static void test_regression_lvalue_push_is_borrow(void)
{
    /* out.push(localItem) where localItem is a named variable owning +1 —
     * must classify BORROW so Commit 2's push emission retains into the
     * slot, leaving localItem's sn_auto release still balanced. */
    Expr local = make_expr(EXPR_VARIABLE);
    assert(ownership_kind(&local) == OWNERSHIP_BORROW);
}

static void test_regression_return_from_container_is_borrow(void)
{
    /* return arr[i] / return obj.field — array element / member read must
     * classify BORROW, matching the retain-on-return fix shipped in
     * e4ff276 and the strdup-on-return WIP. */
    Expr arr_read = make_expr(EXPR_ARRAY_ACCESS);
    Expr mem_read = make_expr(EXPR_MEMBER);
    assert(ownership_kind(&arr_read) == OWNERSHIP_BORROW);
    assert(ownership_kind(&mem_read) == OWNERSHIP_BORROW);
}

void test_ownership_main(void)
{
    TEST_SECTION("Ownership classifier");

    TEST_RUN("owned_call_forms",                    test_owned_call_forms);
    TEST_RUN("owned_constructors",                  test_owned_constructors);
    TEST_RUN("owned_thread_forms",                  test_owned_thread_forms);
    TEST_RUN("owned_operators",                     test_owned_operators);
    TEST_RUN("borrow_reads",                        test_borrow_reads);
    TEST_RUN("borrow_literals",                     test_borrow_literals);
    TEST_RUN("null_source_is_safe",                 test_null_source_is_safe);
    TEST_RUN("kind_str_roundtrip",                  test_kind_str_roundtrip);
    TEST_RUN("regression_queue_drain_take_owned",   test_regression_queue_drain_take_is_owned);
    TEST_RUN("regression_literal_push_owned",       test_regression_literal_push_is_owned);
    TEST_RUN("regression_lvalue_push_borrow",       test_regression_lvalue_push_is_borrow);
    TEST_RUN("regression_return_from_container",    test_regression_return_from_container_is_borrow);
}
