// tests/ast_tests_expr.c
// Expression-related AST tests

static void test_ast_create_binary_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *left = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *right = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *bin = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, loc);
    assert(bin != NULL);
    assert(bin->type == EXPR_BINARY);
    assert(bin->as.binary.left == left);
    assert(bin->as.binary.right == right);
    assert(bin->as.binary.operator == TOKEN_PLUS);
    assert(tokens_equal(bin->token, loc));
    assert(bin->expr_type == NULL);

    // Different operators
    Expr *bin_minus = ast_create_binary_expr(&arena, left, TOKEN_MINUS, right, loc);
    assert(bin_minus->as.binary.operator == TOKEN_MINUS);

    Expr *bin_mult = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, loc);
    assert(bin_mult->as.binary.operator == TOKEN_STAR);

    // NULL left/right
    assert(ast_create_binary_expr(&arena, NULL, TOKEN_PLUS, right, loc) == NULL);
    assert(ast_create_binary_expr(&arena, left, TOKEN_PLUS, NULL, loc) == NULL);
    assert(ast_create_binary_expr(&arena, NULL, TOKEN_PLUS, NULL, loc) == NULL);

    // NULL loc (code allows it, but token is const Token*)
    Expr *bin_null_loc = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, NULL);
    assert(bin_null_loc != NULL);
    assert(bin_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_unary_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 5}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *un = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, loc);
    assert(un != NULL);
    assert(un->type == EXPR_UNARY);
    assert(un->as.unary.operator == TOKEN_MINUS);
    assert(un->as.unary.operand == operand);
    assert(tokens_equal(un->token, loc));

    // Different operators
    Expr *un_not = ast_create_unary_expr(&arena, TOKEN_BANG, operand, loc);
    assert(un_not->as.unary.operator == TOKEN_BANG);

    // NULL operand
    assert(ast_create_unary_expr(&arena, TOKEN_MINUS, NULL, loc) == NULL);

    // NULL loc
    Expr *un_null_loc = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, NULL);
    assert(un_null_loc != NULL);
    assert(un_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_literal_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);

    // Int
    LiteralValue val_int = {.int_value = 42};
    Type *typ_int = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *lit_int = ast_create_literal_expr(&arena, val_int, typ_int, false, loc);
    assert(lit_int != NULL);
    assert(lit_int->type == EXPR_LITERAL);
    assert(lit_int->as.literal.value.int_value == 42);
    assert(lit_int->as.literal.type == typ_int);
    assert(lit_int->as.literal.is_interpolated == false);
    assert(tokens_equal(lit_int->token, loc));

    // Double
    LiteralValue val_double = {.double_value = 3.14};
    Type *typ_double = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Expr *lit_double = ast_create_literal_expr(&arena, val_double, typ_double, false, loc);
    assert(lit_double->as.literal.value.double_value == 3.14);
    assert(lit_double->as.literal.type == typ_double);

    // Char
    LiteralValue val_char = {.char_value = 'a'};
    Type *typ_char = ast_create_primitive_type(&arena, TYPE_CHAR);
    Expr *lit_char = ast_create_literal_expr(&arena, val_char, typ_char, false, loc);
    assert(lit_char->as.literal.value.char_value == 'a');

    // String
    LiteralValue val_string = {.string_value = "hello"};
    Type *typ_string = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *lit_string = ast_create_literal_expr(&arena, val_string, typ_string, false, loc);
    assert(strcmp(lit_string->as.literal.value.string_value, "hello") == 0);

    // Bool
    LiteralValue val_bool = {.bool_value = true};
    Type *typ_bool = ast_create_primitive_type(&arena, TYPE_BOOL);
    Expr *lit_bool = ast_create_literal_expr(&arena, val_bool, typ_bool, false, loc);
    assert(lit_bool->as.literal.value.bool_value == true);

    // Interpolated
    Expr *lit_interp = ast_create_literal_expr(&arena, val_int, typ_int, true, loc);
    assert(lit_interp->as.literal.is_interpolated == true);

    // NULL type (code requires type, but test if NULL)
    Expr *lit_null_type = ast_create_literal_expr(&arena, val_int, NULL, false, loc);
    assert(lit_null_type == NULL);

    // NULL loc
    Expr *lit_null_loc = ast_create_literal_expr(&arena, val_int, typ_int, false, NULL);
    assert(lit_null_loc != NULL);
    assert(lit_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_variable_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "varname");
    Token *loc = ast_clone_token(&arena, &name);
    Expr *var = ast_create_variable_expr(&arena, name, loc);
    assert(var != NULL);
    assert(var->type == EXPR_VARIABLE);
    assert(strcmp(var->as.variable.name.start, "varname") == 0);
    assert(var->as.variable.name.length == 7);
    assert(tokens_equal(var->token, loc));

    // Empty name (length 0)
    Token empty_name = create_dummy_token(&arena, "");
    Expr *var_empty = ast_create_variable_expr(&arena, empty_name, loc);
    assert(var_empty != NULL);
    assert(var_empty->as.variable.name.length == 0);

    // NULL loc
    Expr *var_null_loc = ast_create_variable_expr(&arena, name, NULL);
    assert(var_null_loc != NULL);
    assert(var_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_assign_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "x");
    Token *loc = ast_clone_token(&arena, &name);
    Expr *val = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 10}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *ass = ast_create_assign_expr(&arena, name, val, loc);
    assert(ass != NULL);
    assert(ass->type == EXPR_ASSIGN);
    assert(strcmp(ass->as.assign.name.start, "x") == 0);
    assert(ass->as.assign.value == val);
    assert(tokens_equal(ass->token, loc));

    // NULL value
    Expr *ass_null_val = ast_create_assign_expr(&arena, name, NULL, loc);
    assert(ass_null_val == NULL); // Code allows NULL value

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Expr *ass_empty = ast_create_assign_expr(&arena, empty_name, val, loc);
    assert(ass_empty != NULL);
    assert(ass_empty->as.assign.name.length == 0);

    // NULL loc
    Expr *ass_null_loc = ast_create_assign_expr(&arena, name, val, NULL);
    assert(ass_null_loc != NULL);
    assert(ass_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_call_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *callee = ast_create_variable_expr(&arena, create_dummy_token(&arena, "func"), loc);
    Expr *args[2];
    args[0] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    args[1] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *call = ast_create_call_expr(&arena, callee, args, 2, loc);
    assert(call != NULL);
    assert(call->type == EXPR_CALL);
    assert(call->as.call.callee == callee);
    assert(call->as.call.arg_count == 2);
    assert(call->as.call.arguments[0] == args[0]);
    assert(call->as.call.arguments[1] == args[1]);
    assert(tokens_equal(call->token, loc));

    // Empty args
    Expr *call_empty = ast_create_call_expr(&arena, callee, NULL, 0, loc);
    assert(call_empty != NULL);
    assert(call_empty->as.call.arg_count == 0);
    assert(call_empty->as.call.arguments == NULL);

    // NULL callee
    assert(ast_create_call_expr(&arena, NULL, args, 2, loc) == NULL);

    // NULL args with count > 0 (code sets arguments to passed, even if NULL)
    Expr *call_null_args = ast_create_call_expr(&arena, callee, NULL, 2, loc);
    assert(call_null_args != NULL);
    assert(call_null_args->as.call.arguments == NULL);
    assert(call_null_args->as.call.arg_count == 2);

    // NULL loc
    Expr *call_null_loc = ast_create_call_expr(&arena, callee, args, 2, NULL);
    assert(call_null_loc != NULL);
    assert(call_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_array_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *elems[3];
    elems[0] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    elems[1] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    elems[2] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 3}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *arr = ast_create_array_expr(&arena, elems, 3, loc);
    assert(arr != NULL);
    assert(arr->type == EXPR_ARRAY);
    assert(arr->as.array.element_count == 3);
    assert(arr->as.array.elements[0] == elems[0]);
    assert(arr->as.array.elements[1] == elems[1]);
    assert(arr->as.array.elements[2] == elems[2]);
    assert(tokens_equal(arr->token, loc));
    assert(arr->expr_type == NULL);

    // Empty array
    Expr *arr_empty = ast_create_array_expr(&arena, NULL, 0, loc);
    assert(arr_empty != NULL);
    assert(arr_empty->as.array.element_count == 0);
    assert(arr_empty->as.array.elements == NULL);

    // NULL elems with count > 0
    Expr *arr_null_elems = ast_create_array_expr(&arena, NULL, 3, loc);
    assert(arr_null_elems != NULL);
    assert(arr_null_elems->as.array.elements == NULL);
    assert(arr_null_elems->as.array.element_count == 3);

    // NULL loc
    Expr *arr_null_loc = ast_create_array_expr(&arena, elems, 3, NULL);
    assert(arr_null_loc != NULL);
    assert(arr_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_array_access_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *array = ast_create_variable_expr(&arena, create_dummy_token(&arena, "arr"), loc);
    Expr *index = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 0}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *access = ast_create_array_access_expr(&arena, array, index, loc);
    assert(access != NULL);
    assert(access->type == EXPR_ARRAY_ACCESS);
    assert(access->as.array_access.array == array);
    assert(access->as.array_access.index == index);
    assert(tokens_equal(access->token, loc));
    assert(access->expr_type == NULL);

    // NULL array or index
    assert(ast_create_array_access_expr(&arena, NULL, index, loc) == NULL);
    assert(ast_create_array_access_expr(&arena, array, NULL, loc) == NULL);
    assert(ast_create_array_access_expr(&arena, NULL, NULL, loc) == NULL);

    // NULL loc
    Expr *access_null_loc = ast_create_array_access_expr(&arena, array, index, NULL);
    assert(access_null_loc != NULL);
    assert(access_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_increment_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc);
    Expr *inc = ast_create_increment_expr(&arena, operand, loc);
    assert(inc != NULL);
    assert(inc->type == EXPR_INCREMENT);
    assert(inc->as.operand == operand);
    assert(tokens_equal(inc->token, loc));
    assert(inc->expr_type == NULL);

    // NULL operand
    assert(ast_create_increment_expr(&arena, NULL, loc) == NULL);

    // NULL loc
    Expr *inc_null_loc = ast_create_increment_expr(&arena, operand, NULL);
    assert(inc_null_loc != NULL);
    assert(inc_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_decrement_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc);
    Expr *dec = ast_create_decrement_expr(&arena, operand, loc);
    assert(dec != NULL);
    assert(dec->type == EXPR_DECREMENT);
    assert(dec->as.operand == operand);
    assert(tokens_equal(dec->token, loc));
    assert(dec->expr_type == NULL);

    // NULL operand
    assert(ast_create_decrement_expr(&arena, NULL, loc) == NULL);

    // NULL loc
    Expr *dec_null_loc = ast_create_decrement_expr(&arena, operand, NULL);
    assert(dec_null_loc != NULL);
    assert(dec_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_interpolated_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *parts[2];
    parts[0] = ast_create_literal_expr(&arena, (LiteralValue){.string_value = "hello "}, ast_create_primitive_type(&arena, TYPE_STRING), true, loc);
    parts[1] = ast_create_variable_expr(&arena, create_dummy_token(&arena, "name"), loc);
    char *fmts[2] = {NULL, NULL};
    Expr *interp = ast_create_interpolated_expr(&arena, parts, fmts, 2, loc);
    assert(interp != NULL);
    assert(interp->type == EXPR_INTERPOLATED);
    assert(interp->as.interpol.part_count == 2);
    assert(interp->as.interpol.parts[0] == parts[0]);
    assert(interp->as.interpol.parts[1] == parts[1]);
    assert(interp->as.interpol.format_specs[0] == NULL);
    assert(interp->as.interpol.format_specs[1] == NULL);
    assert(tokens_equal(interp->token, loc));
    assert(interp->expr_type == NULL);

    // Test with format specifiers
    char *fmts_with_spec[2] = {NULL, ".2f"};
    Expr *interp_fmt = ast_create_interpolated_expr(&arena, parts, fmts_with_spec, 2, loc);
    assert(interp_fmt != NULL);
    assert(interp_fmt->as.interpol.format_specs[0] == NULL);
    assert(strcmp(interp_fmt->as.interpol.format_specs[1], ".2f") == 0);

    // Empty parts
    Expr *interp_empty = ast_create_interpolated_expr(&arena, NULL, NULL, 0, loc);
    assert(interp_empty != NULL);
    assert(interp_empty->as.interpol.part_count == 0);
    assert(interp_empty->as.interpol.parts == NULL);

    // NULL parts with count > 0
    Expr *interp_null_parts = ast_create_interpolated_expr(&arena, NULL, NULL, 2, loc);
    assert(interp_null_parts != NULL);
    assert(interp_null_parts->as.interpol.parts == NULL);
    assert(interp_null_parts->as.interpol.part_count == 2);

    // NULL loc
    Expr *interp_null_loc = ast_create_interpolated_expr(&arena, parts, fmts, 2, NULL);
    assert(interp_null_loc != NULL);
    assert(interp_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_member_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Token obj_tok = create_dummy_token(&arena, "arr");
    Expr *obj = ast_create_variable_expr(&arena, obj_tok, loc);
    Token member_tok = create_dummy_token(&arena, "length");
    Expr *mem = ast_create_member_expr(&arena, obj, member_tok, loc);
    assert(mem != NULL);
    assert(mem->type == EXPR_MEMBER);
    assert(mem->as.member.object == obj);
    assert(strcmp(mem->as.member.member_name.start, "length") == 0);
    assert(mem->as.member.member_name.length == 6);
    assert(mem->as.member.member_name.line == 1);
    assert(mem->as.member.member_name.type == TOKEN_IDENTIFIER);
    assert(strcmp(mem->as.member.member_name.filename, "test.sn") == 0);
    assert(mem->expr_type == NULL);
    assert(tokens_equal(mem->token, loc));

    // Empty member name
    Token empty_member = create_dummy_token(&arena, "");
    Expr *mem_empty = ast_create_member_expr(&arena, obj, empty_member, loc);
    assert(mem_empty != NULL);
    assert(mem_empty->as.member.member_name.length == 0);
    assert(mem_empty->as.member.member_name.start != NULL);  // Allocated empty string

    // NULL object
    assert(ast_create_member_expr(&arena, NULL, member_tok, loc) == NULL);

    // NULL loc_token
    Expr *mem_null_loc = ast_create_member_expr(&arena, obj, member_tok, NULL);
    assert(mem_null_loc != NULL);
    assert(mem_null_loc->token == NULL);

    // Different token type for member (e.g., if parser uses different type)
    Token member_kw_tok = member_tok;
    member_kw_tok.type = TOKEN_FN;
    Expr *mem_kw = ast_create_member_expr(&arena, obj, member_kw_tok, loc);
    assert(mem_kw != NULL);
    assert(mem_kw->as.member.member_name.type == TOKEN_FN);

    cleanup_arena(&arena);
}

static void test_ast_create_comparison_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *left = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *right = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *comp = ast_create_comparison_expr(&arena, left, right, TOKEN_EQUAL_EQUAL, loc);
    assert(comp != NULL);
    assert(comp->type == EXPR_BINARY); // Since it's alias to binary
    assert(comp->as.binary.left == left);
    assert(comp->as.binary.right == right);
    assert(comp->as.binary.operator == TOKEN_EQUAL_EQUAL);

    // Different comparison types
    Expr *comp_gt = ast_create_comparison_expr(&arena, left, right, TOKEN_GREATER, loc);
    assert(comp_gt->as.binary.operator == TOKEN_GREATER);

    // NULL left/right
    assert(ast_create_comparison_expr(&arena, NULL, right, TOKEN_EQUAL_EQUAL, loc) == NULL);
    assert(ast_create_comparison_expr(&arena, left, NULL, TOKEN_EQUAL_EQUAL, loc) == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_array_slice_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *array = ast_create_variable_expr(&arena, create_dummy_token(&arena, "arr"), loc);
    Expr *start = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *end = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 3}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);

    // Full slice: arr[1..3]
    Expr *slice = ast_create_array_slice_expr(&arena, array, start, end, NULL, loc);
    assert(slice != NULL);
    assert(slice->type == EXPR_ARRAY_SLICE);
    assert(slice->as.array_slice.array == array);
    assert(slice->as.array_slice.start == start);
    assert(slice->as.array_slice.end == end);
    assert(slice->as.array_slice.step == NULL);
    assert(tokens_equal(slice->token, loc));
    assert(slice->expr_type == NULL);

    // Slice from start: arr[..3]
    Expr *slice_from_start = ast_create_array_slice_expr(&arena, array, NULL, end, NULL, loc);
    assert(slice_from_start != NULL);
    assert(slice_from_start->type == EXPR_ARRAY_SLICE);
    assert(slice_from_start->as.array_slice.array == array);
    assert(slice_from_start->as.array_slice.start == NULL);
    assert(slice_from_start->as.array_slice.end == end);

    // Slice to end: arr[1..]
    Expr *slice_to_end = ast_create_array_slice_expr(&arena, array, start, NULL, NULL, loc);
    assert(slice_to_end != NULL);
    assert(slice_to_end->type == EXPR_ARRAY_SLICE);
    assert(slice_to_end->as.array_slice.array == array);
    assert(slice_to_end->as.array_slice.start == start);
    assert(slice_to_end->as.array_slice.end == NULL);

    // Full copy: arr[..]
    Expr *slice_full = ast_create_array_slice_expr(&arena, array, NULL, NULL, NULL, loc);
    assert(slice_full != NULL);
    assert(slice_full->type == EXPR_ARRAY_SLICE);
    assert(slice_full->as.array_slice.array == array);
    assert(slice_full->as.array_slice.start == NULL);
    assert(slice_full->as.array_slice.end == NULL);

    // Slice with step: arr[0..10:2]
    Expr *step = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *slice_step = ast_create_array_slice_expr(&arena, array, start, end, step, loc);
    assert(slice_step != NULL);
    assert(slice_step->type == EXPR_ARRAY_SLICE);
    assert(slice_step->as.array_slice.step == step);

    // NULL array
    assert(ast_create_array_slice_expr(&arena, NULL, start, end, NULL, loc) == NULL);

    // NULL loc
    Expr *slice_null_loc = ast_create_array_slice_expr(&arena, array, start, end, NULL, NULL);
    assert(slice_null_loc != NULL);
    assert(slice_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_member_access_expr()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);

    /* Create a struct variable expression as the object */
    Token obj_tok = create_dummy_token(&arena, "point");
    Expr *obj = ast_create_variable_expr(&arena, obj_tok, loc);

    Token field_tok = create_dummy_token(&arena, "x");
    Expr *access = ast_create_member_access_expr(&arena, obj, field_tok, loc);

    /* Basic assertions */
    assert(access != NULL);
    assert(access->type == EXPR_MEMBER_ACCESS);
    assert(access->as.member_access.object == obj);
    assert(strcmp(access->as.member_access.field_name.start, "x") == 0);
    assert(access->as.member_access.field_name.length == 1);
    assert(tokens_equal(access->token, loc));
    assert(access->expr_type == NULL);

    /* Verify field_index is initialized to -1 (set during type checking) */
    assert(access->as.member_access.field_index == -1);

    /* Verify escape metadata fields are properly initialized */
    assert(access->as.member_access.escaped == false);
    assert(access->as.member_access.scope_depth == 0);

    /* Test with different field name */
    Token field_y_tok = create_dummy_token(&arena, "y");
    Expr *access_y = ast_create_member_access_expr(&arena, obj, field_y_tok, loc);
    assert(access_y != NULL);
    assert(strcmp(access_y->as.member_access.field_name.start, "y") == 0);
    assert(access_y->as.member_access.escaped == false);
    assert(access_y->as.member_access.scope_depth == 0);

    /* NULL object */
    assert(ast_create_member_access_expr(&arena, NULL, field_tok, loc) == NULL);

    /* NULL loc */
    Expr *access_null_loc = ast_create_member_access_expr(&arena, obj, field_tok, NULL);
    assert(access_null_loc != NULL);
    assert(access_null_loc->token == NULL);
    assert(access_null_loc->as.member_access.escaped == false);
    assert(access_null_loc->as.member_access.scope_depth == 0);

    /* Test that escape metadata can be modified after creation */
    access->as.member_access.escaped = true;
    access->as.member_access.scope_depth = 5;
    assert(access->as.member_access.escaped == true);
    assert(access->as.member_access.scope_depth == 5);

    cleanup_arena(&arena);
}

void test_ast_expr_main()
{
    TEST_SECTION("AST Expression Tests");
    TEST_RUN("ast_create_binary_expr", test_ast_create_binary_expr);
    TEST_RUN("ast_create_unary_expr", test_ast_create_unary_expr);
    TEST_RUN("ast_create_literal_expr", test_ast_create_literal_expr);
    TEST_RUN("ast_create_variable_expr", test_ast_create_variable_expr);
    TEST_RUN("ast_create_assign_expr", test_ast_create_assign_expr);
    TEST_RUN("ast_create_call_expr", test_ast_create_call_expr);
    TEST_RUN("ast_create_array_expr", test_ast_create_array_expr);
    TEST_RUN("ast_create_array_access_expr", test_ast_create_array_access_expr);
    TEST_RUN("ast_create_increment_expr", test_ast_create_increment_expr);
    TEST_RUN("ast_create_decrement_expr", test_ast_create_decrement_expr);
    TEST_RUN("ast_create_interpolated_expr", test_ast_create_interpolated_expr);
    TEST_RUN("ast_create_member_expr", test_ast_create_member_expr);
    TEST_RUN("ast_create_comparison_expr", test_ast_create_comparison_expr);
    TEST_RUN("ast_create_array_slice_expr", test_ast_create_array_slice_expr);
    TEST_RUN("ast_create_member_access_expr", test_ast_create_member_access_expr);
}
