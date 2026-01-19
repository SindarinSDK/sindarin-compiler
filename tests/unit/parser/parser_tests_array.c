// tests/parser_tests_array.c
// Array-related parser tests

static void test_array_declaration_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var arr:int[]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(strcmp(stmt->as.var_decl.name.start, "arr") == 0);
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);
    assert(stmt->as.var_decl.type->as.array.element_type->kind == TYPE_INT);
    assert(stmt->as.var_decl.initializer == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_literal_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var arr:int[] = {1, 2, 3}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(strcmp(stmt->as.var_decl.name.start, "arr") == 0);
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);
    assert(stmt->as.var_decl.type->as.array.element_type->kind == TYPE_INT);
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY);
    assert(stmt->as.var_decl.initializer->as.array.element_count == 3);
    assert(stmt->as.var_decl.initializer->as.array.elements[0]->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array.elements[0]->as.literal.value.int_value == 1);
    assert(stmt->as.var_decl.initializer->as.array.elements[1]->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array.elements[1]->as.literal.value.int_value == 2);
    assert(stmt->as.var_decl.initializer->as.array.elements[2]->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array.elements[2]->as.literal.value.int_value == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_member_access_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2}\n"
        "  print(arr.length)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "main") == 0);
    assert(fn->as.function.return_type->kind == TYPE_VOID);
    assert(fn->as.function.body_count == 2);

    Stmt *var_decl = fn->as.function.body[0];
    assert(var_decl->type == STMT_VAR_DECL);
    assert(var_decl->as.var_decl.initializer->type == EXPR_ARRAY);
    assert(var_decl->as.var_decl.initializer->as.array.element_count == 2);

    Stmt *print_stmt = fn->as.function.body[1];
    assert(print_stmt->type == STMT_EXPR);
    Expr *call = print_stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(strcmp(call->as.call.callee->as.variable.name.start, "print") == 0);
    assert(call->as.call.arg_count == 1);
    Expr *arg = call->as.call.arguments[0];
    assert(arg->type == EXPR_MEMBER);
    assert(arg->as.member.object->type == EXPR_VARIABLE);
    assert(strcmp(arg->as.member.object->as.variable.name.start, "arr") == 0);
    assert(strcmp(arg->as.member.member_name.start, "length") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_method_call_push_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1}\n"
        "  arr.push(2)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *push_stmt = fn->as.function.body[1];
    assert(push_stmt->type == STMT_EXPR);
    Expr *push_call = push_stmt->as.expression.expression;
    assert(push_call->type == EXPR_CALL);
    assert(push_call->as.call.arg_count == 1);
    assert(push_call->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(push_call->as.call.arguments[0]->as.literal.value.int_value == 2);
    Expr *callee = push_call->as.call.callee;
    assert(callee->type == EXPR_MEMBER);
    assert(callee->as.member.object->type == EXPR_VARIABLE);
    assert(strcmp(callee->as.member.object->as.variable.name.start, "arr") == 0);
    assert(strcmp(callee->as.member.member_name.start, "push") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_method_call_clear_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2}\n"
        "  arr.clear()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *clear_stmt = fn->as.function.body[1];
    assert(clear_stmt->type == STMT_EXPR);
    Expr *clear_call = clear_stmt->as.expression.expression;
    assert(clear_call->type == EXPR_CALL);
    assert(clear_call->as.call.arg_count == 0);
    Expr *callee = clear_call->as.call.callee;
    assert(callee->type == EXPR_MEMBER);
    assert(callee->as.member.object->type == EXPR_VARIABLE);
    assert(strcmp(callee->as.member.object->as.variable.name.start, "arr") == 0);
    assert(strcmp(callee->as.member.member_name.start, "clear") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_method_call_concat_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1}\n"
        "  arr.concat({2, 3})\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *concat_stmt = fn->as.function.body[1];
    assert(concat_stmt->type == STMT_EXPR);
    Expr *concat_call = concat_stmt->as.expression.expression;
    assert(concat_call->type == EXPR_CALL);
    assert(concat_call->as.call.arg_count == 1);
    Expr *arg = concat_call->as.call.arguments[0];
    assert(arg->type == EXPR_ARRAY);
    assert(arg->as.array.element_count == 2);
    assert(arg->as.array.elements[0]->as.literal.value.int_value == 2);
    assert(arg->as.array.elements[1]->as.literal.value.int_value == 3);
    Expr *callee = concat_call->as.call.callee;
    assert(callee->type == EXPR_MEMBER);
    assert(callee->as.member.object->type == EXPR_VARIABLE);
    assert(strcmp(callee->as.member.object->as.variable.name.start, "arr") == 0);
    assert(strcmp(callee->as.member.member_name.start, "concat") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_method_call_pop_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3}\n"
        "  var result:int = arr.pop()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *var_result = fn->as.function.body[1];
    assert(var_result->type == STMT_VAR_DECL);
    assert(strcmp(var_result->as.var_decl.name.start, "result") == 0);
    assert(var_result->as.var_decl.type->kind == TYPE_INT);
    Expr *initializer = var_result->as.var_decl.initializer;
    assert(initializer->type == EXPR_CALL);
    assert(initializer->as.call.arg_count == 0);
    Expr *callee = initializer->as.call.callee;
    assert(callee->type == EXPR_MEMBER);
    assert(callee->as.member.object->type == EXPR_VARIABLE);
    assert(strcmp(callee->as.member.object->as.variable.name.start, "arr") == 0);
    assert(strcmp(callee->as.member.member_name.start, "pop") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_print_and_interpolated_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2}\n"
        "  print(arr)\n"
        "  print($\"Arr: {arr} \")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 3);

    Stmt *print_arr = fn->as.function.body[1];
    assert(print_arr->type == STMT_EXPR);
    Expr *call1 = print_arr->as.expression.expression;
    assert(call1->type == EXPR_CALL);
    assert(call1->as.call.arg_count == 1);
    assert(call1->as.call.arguments[0]->type == EXPR_VARIABLE);
    assert(strcmp(call1->as.call.arguments[0]->as.variable.name.start, "arr") == 0);

    Stmt *print_interp = fn->as.function.body[2];
    assert(print_interp->type == STMT_EXPR);
    Expr *call2 = print_interp->as.expression.expression;
    assert(call2->type == EXPR_CALL);
    assert(call2->as.call.arg_count == 1);
    Expr *interp_arg = call2->as.call.arguments[0];
    assert(interp_arg->type == EXPR_INTERPOLATED);
    assert(interp_arg->as.interpol.part_count == 3);
    assert(interp_arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interp_arg->as.interpol.parts[0]->as.literal.value.string_value, "Arr: ") == 0);
    assert(interp_arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(interp_arg->as.interpol.parts[1]->as.variable.name.start, "arr") == 0);
    assert(interp_arg->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(interp_arg->as.interpol.parts[2]->as.literal.value.string_value, " ") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_print_and_interpolated_parsing_no_trailing_literal()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2}\n"
        "  print(arr)\n"
        "  print($\"Arr: {arr}\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 3);

    Stmt *print_arr = fn->as.function.body[1];
    assert(print_arr->type == STMT_EXPR);
    Expr *call1 = print_arr->as.expression.expression;
    assert(call1->type == EXPR_CALL);
    assert(call1->as.call.arg_count == 1);
    assert(call1->as.call.arguments[0]->type == EXPR_VARIABLE);
    assert(strcmp(call1->as.call.arguments[0]->as.variable.name.start, "arr") == 0);

    Stmt *print_interp = fn->as.function.body[2];
    assert(print_interp->type == STMT_EXPR);
    Expr *call2 = print_interp->as.expression.expression;
    assert(call2->type == EXPR_CALL);
    assert(call2->as.call.arg_count == 1);
    Expr *interp_arg = call2->as.call.arguments[0];
    assert(interp_arg->type == EXPR_INTERPOLATED);
    assert(interp_arg->as.interpol.part_count == 2);
    assert(interp_arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(interp_arg->as.interpol.parts[0]->as.literal.value.string_value, "Arr: ") == 0);
    assert(interp_arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(interp_arg->as.interpol.parts[1]->as.variable.name.start, "arr") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_function_params_and_return_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn declare_arr():int[] =>\n"
        "  var arr:int[] = {1, 2, 3}\n"
        "  return arr\n"
        "fn print_arr(arr:int[]):void =>\n"
        "  print(arr)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    Stmt *declare_fn = module->statements[0];
    assert(declare_fn->type == STMT_FUNCTION);
    assert(strcmp(declare_fn->as.function.name.start, "declare_arr") == 0);
    assert(declare_fn->as.function.param_count == 0);
    assert(declare_fn->as.function.return_type->kind == TYPE_ARRAY);
    assert(declare_fn->as.function.return_type->as.array.element_type->kind == TYPE_INT);
    assert(declare_fn->as.function.body_count == 2);
    Stmt *return_stmt = declare_fn->as.function.body[1];
    assert(return_stmt->type == STMT_RETURN);
    assert(return_stmt->as.return_stmt.value->type == EXPR_VARIABLE);
    assert(strcmp(return_stmt->as.return_stmt.value->as.variable.name.start, "arr") == 0);

    Stmt *print_fn = module->statements[1];
    assert(print_fn->type == STMT_FUNCTION);
    assert(strcmp(print_fn->as.function.name.start, "print_arr") == 0);
    assert(print_fn->as.function.param_count == 1);
    assert(strcmp(print_fn->as.function.params[0].name.start, "arr") == 0);
    assert(print_fn->as.function.params[0].type->kind == TYPE_ARRAY);
    assert(print_fn->as.function.params[0].type->as.array.element_type->kind == TYPE_INT);
    assert(print_fn->as.function.return_type->kind == TYPE_VOID);
    assert(print_fn->as.function.body_count == 1);
    Stmt *print_call = print_fn->as.function.body[0];
    assert(print_call->type == STMT_EXPR);
    Expr *call = print_call->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 1);
    assert(call->as.call.arguments[0]->type == EXPR_VARIABLE);
    assert(strcmp(call->as.call.arguments[0]->as.variable.name.start, "arr") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_access_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3}\n"
        "  print(arr[1])\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *print_stmt = fn->as.function.body[1];
    assert(print_stmt->type == STMT_EXPR);
    Expr *call = print_stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arg_count == 1);
    Expr *arg = call->as.call.arguments[0];
    assert(arg->type == EXPR_ARRAY_ACCESS);
    assert(arg->as.array_access.array->type == EXPR_VARIABLE);
    assert(strcmp(arg->as.array_access.array->as.variable.name.start, "arr") == 0);
    assert(arg->as.array_access.index->type == EXPR_LITERAL);
    assert(arg->as.array_access.index->as.literal.value.int_value == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_slice_full_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3, 4, 5}\n"
        "  var slice:int[] = arr[1..3]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *slice_decl = fn->as.function.body[1];
    assert(slice_decl->type == STMT_VAR_DECL);
    assert(strcmp(slice_decl->as.var_decl.name.start, "slice") == 0);
    Expr *initializer = slice_decl->as.var_decl.initializer;
    assert(initializer->type == EXPR_ARRAY_SLICE);
    assert(initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(initializer->as.array_slice.array->as.variable.name.start, "arr") == 0);
    assert(initializer->as.array_slice.start != NULL);
    assert(initializer->as.array_slice.start->type == EXPR_LITERAL);
    assert(initializer->as.array_slice.start->as.literal.value.int_value == 1);
    assert(initializer->as.array_slice.end != NULL);
    assert(initializer->as.array_slice.end->type == EXPR_LITERAL);
    assert(initializer->as.array_slice.end->as.literal.value.int_value == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_slice_from_start_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3, 4, 5}\n"
        "  var slice:int[] = arr[..3]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *slice_decl = fn->as.function.body[1];
    assert(slice_decl->type == STMT_VAR_DECL);
    Expr *initializer = slice_decl->as.var_decl.initializer;
    assert(initializer->type == EXPR_ARRAY_SLICE);
    assert(initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(initializer->as.array_slice.array->as.variable.name.start, "arr") == 0);
    assert(initializer->as.array_slice.start == NULL);
    assert(initializer->as.array_slice.end != NULL);
    assert(initializer->as.array_slice.end->type == EXPR_LITERAL);
    assert(initializer->as.array_slice.end->as.literal.value.int_value == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_slice_to_end_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3, 4, 5}\n"
        "  var slice:int[] = arr[2..]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *slice_decl = fn->as.function.body[1];
    assert(slice_decl->type == STMT_VAR_DECL);
    Expr *initializer = slice_decl->as.var_decl.initializer;
    assert(initializer->type == EXPR_ARRAY_SLICE);
    assert(initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(initializer->as.array_slice.array->as.variable.name.start, "arr") == 0);
    assert(initializer->as.array_slice.start != NULL);
    assert(initializer->as.array_slice.start->type == EXPR_LITERAL);
    assert(initializer->as.array_slice.start->as.literal.value.int_value == 2);
    assert(initializer->as.array_slice.end == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_slice_full_copy_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3}\n"
        "  var copy:int[] = arr[..]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);

    Stmt *slice_decl = fn->as.function.body[1];
    assert(slice_decl->type == STMT_VAR_DECL);
    Expr *initializer = slice_decl->as.var_decl.initializer;
    assert(initializer->type == EXPR_ARRAY_SLICE);
    assert(initializer->as.array_slice.array->type == EXPR_VARIABLE);
    assert(strcmp(initializer->as.array_slice.array->as.variable.name.start, "arr") == 0);
    assert(initializer->as.array_slice.start == NULL);
    assert(initializer->as.array_slice.end == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_array_element_assignment_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  var arr:int[] = {1, 2, 3}\n"
        "  arr[0] = 100\n"
        "  arr[-1] = 300\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 3);

    // First assignment: arr[0] = 100
    Stmt *assign1_stmt = fn->as.function.body[1];
    assert(assign1_stmt->type == STMT_EXPR);
    Expr *assign1 = assign1_stmt->as.expression.expression;
    assert(assign1->type == EXPR_INDEX_ASSIGN);
    assert(assign1->as.index_assign.array->type == EXPR_VARIABLE);
    assert(strcmp(assign1->as.index_assign.array->as.variable.name.start, "arr") == 0);
    assert(assign1->as.index_assign.index->type == EXPR_LITERAL);
    assert(assign1->as.index_assign.index->as.literal.value.int_value == 0);
    assert(assign1->as.index_assign.value->type == EXPR_LITERAL);
    assert(assign1->as.index_assign.value->as.literal.value.int_value == 100);

    // Second assignment: arr[-1] = 300
    Stmt *assign2_stmt = fn->as.function.body[2];
    assert(assign2_stmt->type == STMT_EXPR);
    Expr *assign2 = assign2_stmt->as.expression.expression;
    assert(assign2->type == EXPR_INDEX_ASSIGN);
    assert(assign2->as.index_assign.array->type == EXPR_VARIABLE);
    assert(strcmp(assign2->as.index_assign.array->as.variable.name.start, "arr") == 0);
    // Negative index is parsed as unary minus on literal
    assert(assign2->as.index_assign.index->type == EXPR_UNARY);
    assert(assign2->as.index_assign.value->type == EXPR_LITERAL);
    assert(assign2->as.index_assign.value->as.literal.value.int_value == 300);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_main()
{
    TEST_SECTION("Parser Array Tests");
    TEST_RUN("array_declaration_parsing", test_array_declaration_parsing);
    TEST_RUN("array_literal_parsing", test_array_literal_parsing);
    TEST_RUN("array_member_access_parsing", test_array_member_access_parsing);
    TEST_RUN("array_method_call_push_parsing", test_array_method_call_push_parsing);
    TEST_RUN("array_method_call_clear_parsing", test_array_method_call_clear_parsing);
    TEST_RUN("array_method_call_concat_parsing", test_array_method_call_concat_parsing);
    TEST_RUN("array_method_call_pop_parsing", test_array_method_call_pop_parsing);
    TEST_RUN("array_print_and_interpolated_parsing", test_array_print_and_interpolated_parsing);
    TEST_RUN("array_print_and_interpolated_no_trailing", test_array_print_and_interpolated_parsing_no_trailing_literal);
    TEST_RUN("array_function_params_and_return_parsing", test_array_function_params_and_return_parsing);
    TEST_RUN("array_access_parsing", test_array_access_parsing);
    TEST_RUN("array_element_assignment_parsing", test_array_element_assignment_parsing);
    TEST_RUN("array_slice_full_parsing", test_array_slice_full_parsing);
    TEST_RUN("array_slice_from_start_parsing", test_array_slice_from_start_parsing);
    TEST_RUN("array_slice_to_end_parsing", test_array_slice_to_end_parsing);
    TEST_RUN("array_slice_full_copy_parsing", test_array_slice_full_copy_parsing);
}
