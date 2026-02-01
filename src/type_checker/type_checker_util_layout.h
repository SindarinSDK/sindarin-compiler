#ifndef TYPE_CHECKER_UTIL_LAYOUT_H
#define TYPE_CHECKER_UTIL_LAYOUT_H

#include "type_checker_util.h"

/* Get the alignment requirement for a type.
 * Uses natural alignment rules for C compatibility. */
size_t get_type_alignment(Type *type);

/* Calculate the memory layout for a struct type.
 * Computes field offsets, padding, and total size.
 * Updates the struct_type in place with computed values. */
void calculate_struct_layout(Type *struct_type);

#endif /* TYPE_CHECKER_UTIL_LAYOUT_H */
