#ifndef SN_TEST_NATIVE_RECORD_ARRAY_H
#define SN_TEST_NATIVE_RECORD_ARRAY_H
#include "sn_abi.h"
struct SnArray *sn_test_record_array_new(void);
void sn_test_record_array_free(struct SnArray *array);
uint64_t sn_test_record_array_destroyed(void);
int64_t sn_test_record_array_value(SnAbiValue *value);
SnAbiStatus sn_test_record_array_reenter(SnAbiValue *value, SnAbiStatus (*callback)(SnAbiValue *, uintptr_t), uintptr_t context);
#endif
