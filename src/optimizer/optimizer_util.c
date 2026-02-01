#include "optimizer/optimizer_util.h"
#include "optimizer.h"
#include <stdlib.h>
#include <string.h>

/* Include split modules */
#include "optimizer_util_literal.c"
#include "optimizer_util_vars.c"
#include "optimizer_util_dead.c"
