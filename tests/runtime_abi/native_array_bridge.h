#ifndef SN_TEST_NATIVE_ARRAY_BRIDGE_H
#define SN_TEST_NATIVE_ARRAY_BRIDGE_H
#include "sn_abi.h"
struct SnArray *sn_test_typed_array_new(void);
void sn_test_typed_array_free(struct SnArray *array);
uint64_t sn_test_typed_array_length(SnAbiValue *view);
SnAbiStatus sn_test_typed_array_reenter(SnAbiValue *view, SnAbiStatus (*callback)(SnAbiValue *, uintptr_t), uintptr_t context);
uint64_t sn_test_typed_array_destroyed(void);
#endif
