/* ============================================================================
 * Handle-Based Array Functions Implementation
 * ============================================================================
 * RtHandle-returning variants of all allocating array functions.
 * Array handle layout: [RtArrayMetadata][element data...]
 *
 * For source arrays passed as raw pointers, metadata is at:
 *   ((RtArrayMetadata *)arr)[-1]
 *
 * For handle-based arrays, metadata is at the start of the allocation.
 * ============================================================================ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "runtime_array_h.h"
#include "runtime_array.h"
#include "runtime_arena.h"

/* Include all implementation files */
#include "runtime_array_h_create.c"
#include "runtime_array_h_push.c"
#include "runtime_array_h_pop_clone.c"
#include "runtime_array_h_concat_slice.c"
#include "runtime_array_h_modify.c"
#include "runtime_array_h_alloc.c"
#include "runtime_array_h_string_ops.c"
#include "runtime_array_h_tostring.c"
#include "runtime_array_h_tostring_3d.c"
#include "runtime_array_h_convert.c"
#include "runtime_array_h_promote.c"
