#ifndef TYPE_CHECKER_UTIL_ESCAPE_H
#define TYPE_CHECKER_UTIL_ESCAPE_H

#include "type_checker_util.h"

/* Check if a type can escape from a private block/function.
 * Primitives can always escape.
 * Structs can escape only if they contain only primitive fields.
 * Arrays, strings, pointers cannot escape. */
bool can_escape_private(Type *type);

/* Get a human-readable reason why a type cannot escape a private block.
 * Returns NULL if the type can escape.
 * Uses a static buffer for the return value. */
const char *get_private_escape_block_reason(Type *type);

#endif /* TYPE_CHECKER_UTIL_ESCAPE_H */
