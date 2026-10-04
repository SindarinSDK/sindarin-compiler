/* Included by both expression model builders. Resolved native methods have the
 * same owned-result contract as native functions, including static methods. */
static json_object *wrap_native_method_borrow(json_object *call, Expr *expr)
{
    StructMethod *method = NULL;
    bool has_receiver = false;
    if (expr->type == EXPR_METHOD_CALL) {
        method = expr->as.method_call.method;
        has_receiver = !expr->as.method_call.is_static;
    } else if (expr->type == EXPR_STATIC_CALL) {
        method = expr->as.static_call.resolved_method;
    }
    if (!method || !method->is_native) return call;
    json_object *type = NULL, *name = NULL, *ref = NULL, *args = NULL;
    if (!json_object_object_get_ex(call, "type", &type) ||
        !json_object_object_get_ex(type, "name", &name) ||
        !json_object_object_get_ex(type, "pass_self_by_ref", &ref) ||
        !json_object_get_boolean(ref) || !json_object_object_get_ex(call, "args", &args)) return call;
    const char *result_name = json_object_get_string(name);
    json_object *checks = json_object_new_array();
    if (has_receiver) {
        json_object *receiver = NULL, *receiver_type = NULL, *receiver_name = NULL;
        if (json_object_object_get_ex(call, "object", &receiver) &&
            json_object_object_get_ex(receiver, "type", &receiver_type) &&
            json_object_object_get_ex(receiver_type, "name", &receiver_name) &&
            strcmp(json_object_get_string(receiver_name), result_name) == 0) {
            json_object *check = json_object_new_object();
            json_object_object_add(check, "type_name", json_object_new_string(result_name));
            json_object_object_add(check, "is_receiver", json_object_new_boolean(true));
            json_object_object_add(check, "ptr_expr", json_object_get(receiver));
            json_object_array_add(checks, check);
        }
    }
    for (int i = 0; i < method->param_count && i < (int)json_object_array_length(args); i++) {
        Type *param = method->params[i].type;
        if (param && param->kind == TYPE_STRUCT && param->as.struct_type.pass_self_by_ref &&
            param->as.struct_type.name && strcmp(param->as.struct_type.name, result_name) == 0) {
            json_object *check = json_object_new_object();
            json_object_object_add(check, "type_name", json_object_new_string(result_name));
            json_object_object_add(check, "arg_index", json_object_new_int(i));
            json_object_object_add(check, "ptr_expr", json_object_get(json_object_array_get_idx(args, i)));
            json_object_array_add(checks, check);
        }
    }
    if (!json_object_array_length(checks)) { json_object_put(checks); return call; }
    json_object *wrapper = json_object_new_object();
    json_object_object_add(wrapper, "kind", json_object_new_string("borrow_inferred_call"));
    json_object_object_add(wrapper, "result_type_name", json_object_new_string(result_name));
    json_object_object_add(wrapper, "borrow_check_args", checks);
    json_object_object_add(wrapper, "inner_call", call);
    json_object_object_add(wrapper, "type", json_object_get(type));
    return wrapper;
}
