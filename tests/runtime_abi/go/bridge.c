#include "sn_abi.h"
#include <stddef.h>
#include "native_string_array_bridge.h"
extern SnAbiStatus sn_test_go_native_array_observe(SnAbiValue *, uintptr_t);
SnAbiStatus sn_test_go_native_array_reenter(SnAbiValue *view, uintptr_t context)
{
    return sn_test_native_strings_reenter(view, sn_test_go_native_array_observe, context);
}
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
#include "native_byte_array_bridge.h"
extern SnAbiStatus sn_test_go_native_bytes_observe(SnAbiValue *, uintptr_t);
SnAbiStatus sn_test_go_native_bytes_reenter(SnAbiValue *value, uintptr_t context)
{
    return sn_test_native_bytes_reenter(value, sn_test_go_native_bytes_observe, context);
}
#include "native_array_bridge.h"
extern SnAbiStatus sn_test_go_typed_array_observe(SnAbiValue *, uintptr_t);
SnAbiStatus sn_test_go_typed_array_reenter(SnAbiValue *value, uintptr_t context)
{
    return sn_test_typed_array_reenter(value, sn_test_go_typed_array_observe, context);
}
