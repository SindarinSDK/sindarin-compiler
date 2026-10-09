#include "sn_abi.h"
#include <stddef.h>
/* This declaration contains C types only; no private Go layout crosses the ABI. */
extern void sn_test_go_destroy(void *resource, uintptr_t context);
SnAbiStatus sn_test_go_resource(uintptr_t context, SnAbiValue **out)
{
    return sn_abi_v1_resource_new(NULL, sn_test_go_destroy, context, out);
}
