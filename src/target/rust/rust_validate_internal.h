#ifndef SN_RUST_VALIDATE_INTERNAL_H
#define SN_RUST_VALIDATE_INTERNAL_H

/* Private to rust_target.c's included fragments; no external compiler API.
 * Recursive validators retain target-local annotations and diagnostic state.
 * Family hooks must not reorder child validation or reset that state. */
typedef enum {
    RUST_VALIDATION_UNHANDLED,
    RUST_VALIDATION_SUPPORTED,
    RUST_VALIDATION_UNSUPPORTED
} RustValidationResult;

static bool rust_validate_expr(json_object *expr);
static bool rust_validate_statements(json_object *statements);
static bool rust_validate_block(json_object *block);
static bool rust_closure_type_supported(json_object *type);

#endif
