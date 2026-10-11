#include <assert.h>
#include <string.h>
#include "sn_array.h"
#include "sn_abi.h"
extern SnArray *sn_env_all(void);
extern void sn_env_set(char *, char *);

static SnArray *find(SnArray *rows, const char *name)
{
    for (long long i = 0; i < rows->len; i++) {
        SnArray *row = ((SnArray **)rows->data)[i];
        if (row && row->len == 2 && !strcmp(((char **)row->data)[0], name)) return row;
    }
    return NULL;
}
int main(void)
{
    sn_env_set("SN_ABI_TYPED_ARRAY_TEST", "canonical C environment");
    SnArray *original = sn_env_all();
    SnAbiValue *view = NULL, *copy = NULL;
    const SnAbiNativeArrayType shape = {SN_ABI_ARRAY_STRING,2};
    assert(sn_abi_v1_native_array_adopt(original, shape, &view) == 0);
    SnAbiValue *alias = sn_abi_v1_retain(view);
    SnArray *row = find(original, "SN_ABI_TYPED_ARRAY_TEST"), *data = NULL;
    assert(row && !strcmp(((char **)row->data)[1], "canonical C environment"));
    assert(sn_abi_v1_native_array_data(alias, shape, &data) == 0 && data == original);
    assert(sn_abi_v1_native_array_copy(view, &copy) == 0);
    assert(sn_abi_v1_native_array_data(copy, shape, &data) == 0 && data != original);
    SnArray *copied_row = find(data, "SN_ABI_TYPED_ARRAY_TEST");
    assert(copied_row && copied_row != row);
    assert(((char **)copied_row->data)[1] != ((char **)row->data)[1]);
    ((char **)row->data)[1][0] = 'C';
    assert(!strcmp(((char **)copied_row->data)[1], "canonical C environment"));
    sn_abi_v1_release(view); sn_abi_v1_release(alias);
    assert(!strcmp(((char **)copied_row->data)[1], "canonical C environment"));
    SnArray *again = sn_array_copy(data);
    sn_abi_v1_release(copy);
    row = find(again, "SN_ABI_TYPED_ARRAY_TEST");
    assert(row && !strcmp(((char **)row->data)[1], "canonical C environment"));
    sn_array_free(again);
    puts("SDK Environment typed array: pass");
    return 0;
}
