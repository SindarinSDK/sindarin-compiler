#include "sn_array.h"
#include "sn_abi.h"
#include "ops.h"
static uint64_t dead_rows, dead_strings, calls;
static void (*callback)(void *);
static void release_string(void *slot) { dead_strings++; free(*(char **)slot); }
static void release_row(void *slot) { dead_rows++; sn_array_free(*(SnArray **)slot); }
void *sn_typed_new(void)
{
    SnArray *root = sn_array_new(sizeof(SnArray *), 1);
    SnArray *row = sn_array_new(sizeof(char *), 1);
    root->elem_tag = SN_TAG_ARRAY; root->elem_release = release_row;
    row->elem_tag = SN_TAG_STRING; row->elem_release = release_string;
    char *text = strdup("mutable");
    sn_array_push(row, &text); sn_array_push(root, &row);
    return root;
}
void *sn_typed_copy(void *left, void *right, int64_t mode)
{
    calls++;
    if (left != right) abort();
    if (!left) return NULL;
    SnArray *root = left, *row = ((SnArray **)root->data)[0];
    ((char **)row->data)[0][0] = 'M';
    char *tail = strdup("tail"); sn_array_push(row, &tail);
    SnAbiNativeArrayType shape = {SN_ABI_ARRAY_STRING, 2};
    SnAbiValue *view = NULL, *copy = NULL; SnArray *result = NULL;
    if (sn_abi_v1_native_array_borrow(root, shape, &view) ||
        sn_abi_v1_native_array_copy(view, &copy) ||
        sn_abi_v1_native_array_take(&copy, shape, &result)) abort();
    sn_abi_v1_release(view);
    if (mode == 1) result->elem_tag = SN_TAG_INT;
    return result;
}
void sn_typed_free(void *value) { sn_array_free(value); }
void *sn_typed_ints(void *left, void *right)
{
    if (left != right) abort();
    if (!left) return NULL;
    SnArray *root = left, *row = ((SnArray **)root->data)[0];
    ((int32_t *)row->data)[0] = 42;
    int32_t added = 99; sn_array_push(row, &added);
    SnAbiNativeArrayType shape = {SN_ABI_ARRAY_INT32, 2};
    SnAbiValue *view = NULL, *copy = NULL; SnArray *result = NULL;
    if (sn_abi_v1_native_array_borrow(root, shape, &view) ||
        sn_abi_v1_native_array_copy(view, &copy) ||
        sn_abi_v1_native_array_take(&copy, shape, &result)) abort();
    sn_abi_v1_release(view); return result;
}
void sn_typed_observe(void *value)
{
    if (!value) return;
    SnArray *root = value, *row = ((SnArray **)root->data)[0];
    if (row->len < 2 || strcmp(((char **)row->data)[0], "Mutable") ||
        strcmp(((char **)row->data)[row->len-1], "tail")) abort();
    if (callback) callback(value);
    if (strcmp(((char **)row->data)[0], "Mutable")) abort();
}
void sn_typed_set_callback(void (*value)(void *)) { callback = value; }
uint64_t sn_typed_dead_rows(void) { return dead_rows; }
uint64_t sn_typed_dead_strings(void) { return dead_strings; }
uint64_t sn_typed_calls(void) { return calls; }
