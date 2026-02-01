/*
 * symbol_table_namespace.c - Namespace operations for symbol table
 *
 * This module contains operations for managing namespaced symbols:
 * - Creating namespaces
 * - Adding symbols/functions to namespaces
 * - Looking up symbols within namespaces
 * - Checking if a name is a namespace
 */

#include "../symbol_table.h"
#include "symbol_table_namespace.h"
#include "../debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include split modules */
#include "symbol_table_namespace_basic.c"
#include "symbol_table_namespace_nested.c"
