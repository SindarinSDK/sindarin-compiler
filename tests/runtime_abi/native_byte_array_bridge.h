#ifndef TEST_NATIVE_BYTE_ARRAY_BRIDGE_H
#define TEST_NATIVE_BYTE_ARRAY_BRIDGE_H
#include "sn_abi.h"
struct SnArray *sn_test_native_bytes_new(void);
void sn_test_native_bytes_free(struct SnArray *array);
uint64_t sn_test_native_bytes_destroyed(void);
void sn_test_native_bytes_copy_consume(SnAbiValue *value);
SnAbiStatus sn_test_native_bytes_push(SnAbiValue *value, uint8_t byte);
SnAbiStatus sn_test_native_bytes_reenter(SnAbiValue *value,
    SnAbiStatus (*observe)(SnAbiValue *, uintptr_t), uintptr_t context);
#endif
