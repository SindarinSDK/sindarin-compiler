#include "sn_abi.h"
#include <stddef.h>
/* This declaration contains C types only; no private Go layout crosses the ABI. */
extern void sn_test_go_destroy(void *resource, uintptr_t context);
extern void sn_test_go_mutation_destroy(void *resource, uintptr_t context);
SnAbiStatus sn_test_go_mutation_resource(uintptr_t context, SnAbiValue **out)
{
    return sn_abi_v1_resource_new(NULL, sn_test_go_mutation_destroy, context, out);
}
SnAbiStatus sn_test_go_resource(uintptr_t context, SnAbiValue **out)
{
    return sn_abi_v1_resource_new(NULL, sn_test_go_destroy, context, out);
}
SnAbiStatus sn_test_go_typed_resource(uintptr_t context, SnAbiValue **out)
{
    return sn_abi_v1_resource_new_typed("pkg.GoResource@1", NULL, sn_test_go_destroy, context, out);
}
