#ifndef SN_TEST_NATIVE_STRING_ARRAY_BRIDGE_H
#define SN_TEST_NATIVE_STRING_ARRAY_BRIDGE_H
#include "sn_abi.h"
struct SnArray *sn_test_native_strings_new(void);
void sn_test_native_strings_free(struct SnArray *array);
uint64_t sn_test_native_strings_destroyed(void);
SnAbiStatus sn_test_native_strings_length(SnAbiValue *view, uint64_t *out);
SnAbiStatus sn_test_native_strings_read(SnAbiValue *view, uint64_t index, SnAbiValue **out);
SnAbiStatus sn_test_native_strings_push(SnAbiValue *view, const char *text);
typedef SnAbiStatus (*SnTestArrayObserve)(SnAbiValue *, uintptr_t);
SnAbiStatus sn_test_native_strings_reenter(SnAbiValue *view, SnTestArrayObserve observe, uintptr_t context);
#endif
