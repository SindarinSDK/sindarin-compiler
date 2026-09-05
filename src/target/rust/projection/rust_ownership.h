/* Rust-private projection, migrated from main 7af0d192. */
#ifndef RUST_PROJECTION_OWNERSHIP_H
#define RUST_PROJECTION_OWNERSHIP_H

#include "ast.h"

/* Unified ownership classifier.
 *
 * Every expression that produces a heap resource (str, array, as-ref struct,
 * val-struct with heap fields) falls into exactly one ownership class:
 *
 *   BORROW — the expression reads through a live owner. The owner (a named
 *     local, parameter, struct field, array slot, static string) stays live
 *     across the statement. A destination that consumes the value needs its
 *     own owner credit (strdup / sn_array_copy / __sn__T_retain / __sn__T_copy).
 *
 *   OWNED — the expression produces a fresh +1 credit with no other live
 *     owner (call result, struct literal, array literal, copyOf, ...). The
 *     destination consumes that credit directly; no acquire needed.
 *
 * This classifier is the single replacement for the shape-based if-else
 * trees ("is src EXPR_VARIABLE or EXPR_MEMBER or EXPR_ARRAY_ACCESS or ...?")
 * scattered across rust_gen_model_stmt.c and rust_gen_model_expr.c at every edge where
 * ownership is decided (var-bind, assign, push, struct-lit field, return,
 * arg-pass, thread-spawn).
 *
 * The result is orthogonal to type family: combine with rust_gen_model_type_category()
 * to pick the concrete acquire function when BORROW.
 */

typedef enum {
    RUST_OWNERSHIP_BORROW,
    RUST_OWNERSHIP_OWNED
} RustOwnershipKind;

RustOwnershipKind rust_ownership_kind(const Expr *src);
const char *rust_ownership_kind_str(RustOwnershipKind k);

/* True when member lowering lifts an owned value-struct call result into an
 * auto-cleaned temporary and applies a keeper operation to the selected
 * field.  The member expression therefore produces an owned result rather
 * than borrowing from a live struct owner. */
bool rust_ownership_is_lifted_member(const Expr *src);

#endif
