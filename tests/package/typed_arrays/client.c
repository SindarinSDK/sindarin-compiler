#include <assert.h>
#include <string.h>
#include "sn_array.h"
#include "sn_abi.h"
#include "ops.h"
extern SnAbiStatus typed_array_provide(SnAbiValue *, SnAbiValue *, int64_t, SnAbiValue **);
extern SnAbiStatus typed_array_ints_provide(SnAbiValue *, SnAbiValue *, SnAbiValue **);
static SnAbiValue *owner;
static int visited;
static void consume(void *header)
{
    assert(owner && header); sn_abi_v1_release(owner); owner = NULL; visited++;
    assert(((SnArray **) ((SnArray *)header)->data)[0]->len >= 2);
}
int main(void)
{
    SnAbiNativeArrayType shape = {SN_ABI_ARRAY_STRING, 2};
    SnAbiValue *out = (SnAbiValue *)(uintptr_t)1, *wrong = NULL;
    SnArray *original = sn_typed_new();
    assert(sn_abi_v1_native_array_adopt(original, shape, &owner) == 0);
    assert(typed_array_provide(owner, owner, 0, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_typed_calls() == 0);
    assert(sn_abi_v1_string_copy("wrong kind", &wrong) == 0);
    assert(typed_array_provide(wrong, owner, 0, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    sn_abi_v1_release(wrong); assert(sn_typed_calls() == 0);
    assert(typed_array_ints_provide(owner, owner, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    assert(typed_array_provide(owner, owner, 3, &out) == SN_ABI_OUT_OF_RANGE && out == (SnAbiValue *)(uintptr_t)1);
    assert(typed_array_provide(owner, owner, 2, &out) == SN_ABI_FOREIGN_ERROR && out == (SnAbiValue *)(uintptr_t)1);
    assert(sn_typed_calls() == 0 && sn_typed_dead_rows() == 0);
    assert(typed_array_provide(owner, owner, 1, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    assert(sn_typed_dead_rows() == 1 && sn_typed_dead_strings() == 2);
    sn_typed_set_callback(consume);
    assert(typed_array_provide(owner, owner, 0, &out) == 0 && !owner && visited == 1);
    assert(sn_typed_dead_rows() == 2 && sn_typed_dead_strings() == 5);
    SnArray *copy = NULL;
    assert(sn_abi_v1_native_array_take(&out, shape, &copy) == 0 && !out && copy != original);
    SnArray *row = ((SnArray **)copy->data)[0];
    assert(row->len == 3 && !strcmp(((char **)row->data)[0], "Mutable"));
    sn_array_free(copy); assert(sn_typed_dead_rows() == 3 && sn_typed_dead_strings() == 8);
    sn_typed_set_callback(NULL);
    assert(typed_array_provide(NULL, NULL, 0, &out) == 0 && !out);
    assert(sn_abi_v1_native_array_adopt(sn_typed_new(), shape, &owner) == 0);
    sn_typed_set_callback(consume); out = (SnAbiValue *)(uintptr_t)1;
    assert(typed_array_provide(owner, owner, 4, &out) == SN_ABI_OUT_OF_RANGE && !owner && visited == 2);
    assert(out == (SnAbiValue *)(uintptr_t)1);
    assert(sn_typed_dead_rows() == 5 && sn_typed_dead_strings() == 12);
    return 0;
}
