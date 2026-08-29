#include <assert.h>
#include <json-c/json.h>

#include "../test_harness.h"
#include "cgen/gen_model_split.h"

static json_object *gen_model_split_test_model(void)
{
    json_object *model = json_object_new_object();
    json_object_object_add(model, "functions", json_object_new_array());
    json_object_object_add(model, "pragmas", json_object_new_array());
    return model;
}

static void test_gen_model_split_native_externs(void)
{
    json_object *model = gen_model_split_test_model();
    json_object *functions = NULL;
    assert(json_object_object_get_ex(model, "functions", &functions));

    json_object *native_fn = json_object_new_object();
    json_object_object_add(native_fn, "name", json_object_new_string("native_helper"));
    json_object_object_add(native_fn, "source_file", json_object_new_string("native.sn"));
    json_object_object_add(native_fn, "is_native", json_object_new_boolean(true));
    json_object_object_add(native_fn, "has_body", json_object_new_boolean(false));
    json_object_object_add(native_fn, "has_pragma_source", json_object_new_boolean(true));
    json_object_array_add(functions, native_fn);

    ModularModel *split = gen_model_split(model, "main.sn");
    assert(split != NULL);
    assert(split->impl_count == 2);

    json_object *header_functions = NULL;
    assert(json_object_object_get_ex(split->common_header, "functions", &header_functions));
    assert(json_object_array_length(header_functions) == 0);

    for (int i = 0; i < split->impl_count; i++)
    {
        json_object *native_externs = NULL;
        assert(json_object_object_get_ex(split->impl_models[i], "native_externs", &native_externs));
        assert(json_object_array_length(native_externs) == 1);

        json_object *native_copy = json_object_array_get_idx(native_externs, 0);
        assert(native_copy != native_fn);

        json_object *needs_forward_decl = NULL;
        assert(json_object_object_get_ex(native_copy, "needs_forward_decl", &needs_forward_decl));
        assert(json_object_get_boolean(needs_forward_decl));
    }

    modular_model_free(split);
    json_object_put(model);
}

void test_gen_model_split_main(void)
{
    TEST_SECTION("Modular Model Ownership");
    TEST_RUN("gen_model_split_native_externs", test_gen_model_split_native_externs);
}
