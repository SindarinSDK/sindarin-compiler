#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "managed_arena.h"

/* Forward declarations */
static void invoke_cleanup_list(RtManagedArena *ma);

/* ============================================================================
 * Internal: GC Thread Forward Declarations
 * ============================================================================ */

extern void *rt_managed_cleaner_thread(void *arg);
extern void *rt_managed_compactor_thread(void *arg);

/* Include split modules */
#include "managed_arena_block.c"
#include "managed_arena_table.c"
#include "managed_arena_cleanup.c"
#include "managed_arena_lifecycle.c"
#include "managed_arena_alloc.c"
#include "managed_arena_promote.c"
#include "managed_arena_pin.c"
#include "managed_arena_diag.c"
