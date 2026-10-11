#include "native_array_bridge.h"
#include "sn_array.h"
static uint64_t destroyed;
static void release_string(void *slot) { destroyed++; free(*(char **)slot); }
static void release_child(void *slot) { sn_array_free(*(SnArray **)slot); }
SnArray *sn_test_typed_array_new(void)
{
    SnArray *root = sn_array_new(sizeof(SnArray *), 1), *child = sn_array_new(sizeof(char *), 1);
    root->elem_tag = SN_TAG_ARRAY; root->elem_release = release_child;
    child->elem_tag = SN_TAG_STRING; child->elem_release = release_string;
    char *text = strdup("first"); sn_array_push(child, &text); sn_array_push(root, &child);
    return root;
}
void sn_test_typed_array_free(SnArray *array) { sn_array_free(array); }
uint64_t sn_test_typed_array_length(SnAbiValue *view)
{
    SnArray *root = NULL;
    if (sn_abi_v1_native_array_data(view, (SnAbiNativeArrayType){SN_ABI_ARRAY_STRING, 2}, &root) || !root) return UINT64_MAX;
    return (uint64_t)((SnArray **)root->data)[0]->len;
}
SnAbiStatus sn_test_typed_array_reenter(SnAbiValue *view, SnAbiStatus (*callback)(SnAbiValue *, uintptr_t), uintptr_t context)
{
    if (!view || !callback) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *guard = sn_abi_v1_retain(view);
    SnAbiStatus status = SN_ABI_OK;
    for (int i = 0; !status && i < 64; i++) {
        SnArray *root = NULL;
        status = sn_abi_v1_native_array_data(view, (SnAbiNativeArrayType){SN_ABI_ARRAY_STRING, 2}, &root);
        if (status) break;
        char *text = strdup("C update"); sn_array_push(((SnArray **)root->data)[0], &text);
        status = callback(view, context);
    }
    sn_abi_v1_release(guard);
    return status;
}
uint64_t sn_test_typed_array_destroyed(void) { return destroyed; }
