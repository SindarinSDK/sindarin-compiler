// tests/unit/lexer/lexer_tests_keywords.c
// Tests for keyword tokenization

/* ============================================================================
 * Keyword Recognition Tests
 * ============================================================================ */

static void test_lexer_keyword_if(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "if");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_else(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "else");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ELSE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_for(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "for");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FOR);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_while(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "while");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_WHILE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_return(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "return");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RETURN);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_fn(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "fn");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FN);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_var(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_nil(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "nil");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NIL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_match(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "match");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MATCH);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_break(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "break");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BREAK);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_continue(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "continue");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CONTINUE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_in(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "in");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IN);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_kw_as(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "as");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_AS);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_kw_val(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "val");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_kw_ref(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "ref");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_REF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_native(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "native");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NATIVE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_struct(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "struct");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRUCT);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_import(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "import");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IMPORT);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_static(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "static");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STATIC);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_typeof(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "typeof");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_TYPEOF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_is(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "is");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IS);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_sizeof(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "sizeof");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SIZEOF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_kw_shared(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "shared");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SHARED);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_kw_private(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "private");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PRIVATE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_sync(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "sync");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SYNC);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_lock(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "lock");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LOCK);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Type Keyword Tests
 * ============================================================================ */

static void test_lexer_type_int(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "int");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_long(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "long");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LONG);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_double(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "double");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_str(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "str");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STR);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_bool(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "bool");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BOOL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_char(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "char");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_byte(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "byte");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BYTE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_void(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "void");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VOID);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_any(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "any");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ANY);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_type_float(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "float");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FLOAT);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Keyword Context Tests
 * ============================================================================ */

static void test_lexer_keyword_with_space_after(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "if ");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_before_paren(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "if(");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "if else");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ELSE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_after_number(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "123 if");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_type_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var x: int");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_fn_declaration_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "fn foo(): void");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VOID);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_for_loop_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "for i in range");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FOR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER); // range is not a keyword
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_return_value_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "return 42");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RETURN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_match_expr_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "match x");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MATCH);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_struct_def_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "struct Point");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRUCT);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_native_fn_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "native fn puts");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NATIVE);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_as_val_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x as val");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_AS);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_as_ref_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x as ref");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_AS);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_REF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_break_continue_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "break continue");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BREAK);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CONTINUE);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_any_type_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var x: any");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ANY);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_array_type_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var arr: [int]");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACKET);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_BRACKET);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_nil_check_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x == nil");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL_EQUAL);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NIL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_is_type_check_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x is int");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IS);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_sizeof_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "sizeof int");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SIZEOF);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    cleanup_lexer_test(&arena, &lexer);
}

void test_lexer_keywords_main(void)
{
    TEST_SECTION("Lexer Keywords");

    // Basic keywords
    TEST_RUN("keyword_if", test_lexer_keyword_if);
    TEST_RUN("keyword_else", test_lexer_keyword_else);
    TEST_RUN("keyword_for", test_lexer_keyword_for);
    TEST_RUN("keyword_while", test_lexer_keyword_while);
    TEST_RUN("keyword_return", test_lexer_keyword_return);
    TEST_RUN("keyword_fn", test_lexer_keyword_fn);
    TEST_RUN("keyword_var", test_lexer_keyword_var);
    TEST_RUN("keyword_nil", test_lexer_keyword_nil);
    TEST_RUN("keyword_match", test_lexer_keyword_match);
    TEST_RUN("keyword_break", test_lexer_keyword_break);
    TEST_RUN("keyword_continue", test_lexer_keyword_continue);
    TEST_RUN("keyword_in", test_lexer_keyword_in);
    TEST_RUN("keyword_as", test_lexer_kw_as);
    TEST_RUN("keyword_val", test_lexer_kw_val);
    TEST_RUN("keyword_ref", test_lexer_kw_ref);
    TEST_RUN("keyword_native", test_lexer_keyword_native);
    TEST_RUN("keyword_struct", test_lexer_keyword_struct);
    TEST_RUN("keyword_import", test_lexer_keyword_import);
    TEST_RUN("keyword_static", test_lexer_keyword_static);
    TEST_RUN("keyword_typeof", test_lexer_keyword_typeof);
    TEST_RUN("keyword_is", test_lexer_keyword_is);
    TEST_RUN("keyword_sizeof", test_lexer_keyword_sizeof);
    TEST_RUN("keyword_shared", test_lexer_kw_shared);
    TEST_RUN("keyword_private", test_lexer_kw_private);
    TEST_RUN("keyword_sync", test_lexer_keyword_sync);
    TEST_RUN("keyword_lock", test_lexer_keyword_lock);

    // Type keywords
    TEST_RUN("type_int", test_lexer_type_int);
    TEST_RUN("type_long", test_lexer_type_long);
    TEST_RUN("type_double", test_lexer_type_double);
    TEST_RUN("type_str", test_lexer_type_str);
    TEST_RUN("type_bool", test_lexer_type_bool);
    TEST_RUN("type_char", test_lexer_type_char);
    TEST_RUN("type_byte", test_lexer_type_byte);
    TEST_RUN("type_void", test_lexer_type_void);
    TEST_RUN("type_any", test_lexer_type_any);
    TEST_RUN("type_float", test_lexer_type_float);

    // Context tests
    TEST_RUN("keyword_with_space_after", test_lexer_keyword_with_space_after);
    TEST_RUN("keyword_before_paren", test_lexer_keyword_before_paren);
    TEST_RUN("keyword_sequence", test_lexer_keyword_sequence);
    TEST_RUN("keyword_after_number", test_lexer_keyword_after_number);
    TEST_RUN("keyword_type_sequence", test_lexer_keyword_type_sequence);
    TEST_RUN("fn_declaration_sequence", test_lexer_fn_declaration_sequence);
    TEST_RUN("for_loop_sequence", test_lexer_for_loop_sequence);
    TEST_RUN("return_value_sequence", test_lexer_return_value_sequence);
    TEST_RUN("match_expr_sequence", test_lexer_match_expr_sequence);
    TEST_RUN("struct_def_sequence", test_lexer_struct_def_sequence);
    TEST_RUN("native_fn_sequence", test_lexer_native_fn_sequence);
    TEST_RUN("as_val_sequence", test_lexer_as_val_sequence);
    TEST_RUN("as_ref_sequence", test_lexer_as_ref_sequence);
    TEST_RUN("break_continue_sequence", test_lexer_break_continue_sequence);
    TEST_RUN("any_type_sequence", test_lexer_any_type_sequence);
    TEST_RUN("array_type_sequence", test_lexer_array_type_sequence);
    TEST_RUN("nil_check_sequence", test_lexer_nil_check_sequence);
    TEST_RUN("is_type_check_sequence", test_lexer_is_type_check_sequence);
    TEST_RUN("sizeof_sequence", test_lexer_sizeof_sequence);
}
