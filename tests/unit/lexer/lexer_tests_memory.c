// tests/lexer_tests_memory.c
// Lexer tests for memory management keywords (shared, private, as, val, ref)

#include "../test_harness.h"

static void test_lexer_keyword_shared(void)
{
    DEBUG_INFO("Starting test_lexer_keyword_shared");

    const char *source = "shared";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_SHARED);
    assert(t1.length == 6);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keyword_shared");
}

static void test_lexer_keyword_private(void)
{
    DEBUG_INFO("Starting test_lexer_keyword_private");

    const char *source = "private";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_PRIVATE);
    assert(t1.length == 7);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keyword_private");
}

static void test_lexer_keyword_as(void)
{
    DEBUG_INFO("Starting test_lexer_keyword_as");

    const char *source = "as";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_AS);
    assert(t1.length == 2);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keyword_as");
}

static void test_lexer_keyword_val(void)
{
    DEBUG_INFO("Starting test_lexer_keyword_val");

    const char *source = "val";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAL);
    assert(t1.length == 3);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keyword_val");
}

static void test_lexer_keyword_ref(void)
{
    DEBUG_INFO("Starting test_lexer_keyword_ref");

    const char *source = "ref";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_REF);
    assert(t1.length == 3);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keyword_ref");
}

static void test_lexer_memory_keywords_combined(void)
{
    DEBUG_INFO("Starting test_lexer_memory_keywords_combined");

    const char *source = "shared private as val ref";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_SHARED);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_PRIVATE);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_AS);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_VAL);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_REF);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_memory_keywords_combined");
}

static void test_lexer_as_val_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_as_val_syntax");

    const char *source = "x as val";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_AS);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_VAL);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_as_val_syntax");
}

static void test_lexer_as_ref_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_as_ref_syntax");

    const char *source = "x: int as ref";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_COLON);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_INT);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_AS);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_REF);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_as_ref_syntax");
}

static void test_lexer_shared_function_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_shared_function_syntax");

    const char *source = "fn foo() shared: void =>";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_FN);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_LEFT_PAREN);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RIGHT_PAREN);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_SHARED);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_COLON);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_VOID);

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_ARROW);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_shared_function_syntax");
}

static void test_lexer_private_block_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_private_block_syntax");

    const char *source = "private =>";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_PRIVATE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_ARROW);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_private_block_syntax");
}

static void test_lexer_val_var_distinction(void)
{
    DEBUG_INFO("Starting test_lexer_val_var_distinction");

    const char *source = "val var value variable";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAL);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_VAR);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);  // "value" is not a keyword

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_IDENTIFIER);  // "variable" is not a keyword

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_val_var_distinction");
}

static void test_lexer_ref_return_distinction(void)
{
    DEBUG_INFO("Starting test_lexer_ref_return_distinction");

    const char *source = "ref return reference";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_REF);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_RETURN);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);  // "reference" is not a keyword

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_ref_return_distinction");
}

static void test_lexer_shared_str_distinction(void)
{
    DEBUG_INFO("Starting test_lexer_shared_str_distinction");

    const char *source = "shared str share string";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_SHARED);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_STR);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);  // "share" is not a keyword

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_IDENTIFIER);  // "string" is not a keyword

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_shared_str_distinction");
}

static void test_lexer_import_as_namespace_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_import_as_namespace_syntax");

    const char *source = "import \"math_utils\" as math";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IMPORT);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_STRING_LITERAL);
    assert(strcmp(t2.literal.string_value, "math_utils") == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_AS);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_IDENTIFIER);
    assert(t4.length == 4);
    assert(strncmp(t4.start, "math", 4) == 0);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_import_as_namespace_syntax");
}

static void test_lexer_as_identifier_prefix(void)
{
    DEBUG_INFO("Starting test_lexer_as_identifier_prefix");

    const char *source = "as assert assign async";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_AS);  // "as" is the keyword

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);  // "assert" is an identifier

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);  // "assign" is an identifier

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_IDENTIFIER);  // "async" is an identifier

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_as_identifier_prefix");
}

void test_lexer_memory_main(void)
{
    TEST_SECTION("Lexer Memory Keywords Tests");
    TEST_RUN("lexer_keyword_shared", test_lexer_keyword_shared);
    TEST_RUN("lexer_keyword_private", test_lexer_keyword_private);
    TEST_RUN("lexer_keyword_as", test_lexer_keyword_as);
    TEST_RUN("lexer_keyword_val", test_lexer_keyword_val);
    TEST_RUN("lexer_keyword_ref", test_lexer_keyword_ref);
    TEST_RUN("lexer_memory_keywords_combined", test_lexer_memory_keywords_combined);
    TEST_RUN("lexer_as_val_syntax", test_lexer_as_val_syntax);
    TEST_RUN("lexer_as_ref_syntax", test_lexer_as_ref_syntax);
    TEST_RUN("lexer_shared_function_syntax", test_lexer_shared_function_syntax);
    TEST_RUN("lexer_private_block_syntax", test_lexer_private_block_syntax);
    TEST_RUN("lexer_val_var_distinction", test_lexer_val_var_distinction);
    TEST_RUN("lexer_ref_return_distinction", test_lexer_ref_return_distinction);
    TEST_RUN("lexer_shared_str_distinction", test_lexer_shared_str_distinction);
    TEST_RUN("lexer_import_as_namespace_syntax", test_lexer_import_as_namespace_syntax);
    TEST_RUN("lexer_as_identifier_prefix", test_lexer_as_identifier_prefix);
}
