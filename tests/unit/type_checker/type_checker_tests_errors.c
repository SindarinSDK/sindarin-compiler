// tests/type_checker_tests_errors.c
// Tests for enhanced type checker error messages with suggestions

#include "../type_checker/type_checker_util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test Levenshtein distance calculations */
static void test_levenshtein_distance_identical(void)
{
    int dist = levenshtein_distance("hello", 5, "hello", 5);
    assert(dist == 0);
}

static void test_levenshtein_distance_one_char_diff(void)
{
    /* "count" vs "coutn" - one transposition (distance 2 with standard edit ops) */
    int dist = levenshtein_distance("count", 5, "coutn", 5);
    assert(dist == 2);  /* c-o-u-n-t vs c-o-u-t-n requires 2 ops */

    /* "hello" vs "hallo" - one substitution */
    dist = levenshtein_distance("hello", 5, "hallo", 5);
    assert(dist == 1);
}

static void test_levenshtein_distance_insertion(void)
{
    /* "count" vs "counts" - one insertion */
    int dist = levenshtein_distance("count", 5, "counts", 6);
    assert(dist == 1);
}

static void test_levenshtein_distance_deletion(void)
{
    /* "counts" vs "count" - one deletion */
    int dist = levenshtein_distance("counts", 6, "count", 5);
    assert(dist == 1);
}

static void test_levenshtein_distance_empty(void)
{
    int dist = levenshtein_distance("", 0, "hello", 5);
    assert(dist == 5);

    dist = levenshtein_distance("hello", 5, "", 0);
    assert(dist == 5);

    dist = levenshtein_distance("", 0, "", 0);
    assert(dist == 0);
}

static void test_levenshtein_distance_completely_different(void)
{
    int dist = levenshtein_distance("abc", 3, "xyz", 3);
    assert(dist == 3);  /* 3 substitutions */
}

/* Test find_similar_symbol with a mock symbol table */
static void test_find_similar_symbol_basic(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add some symbols */
    Token count_tok;
    count_tok.type = TOKEN_IDENTIFIER;
    count_tok.start = "count";
    count_tok.length = 5;
    count_tok.line = 1;
    count_tok.filename = "test.sn";
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    symbol_table_add_symbol(&table, count_tok, int_type);

    Token value_tok;
    value_tok.type = TOKEN_IDENTIFIER;
    value_tok.start = "value";
    value_tok.length = 5;
    value_tok.line = 2;
    value_tok.filename = "test.sn";
    symbol_table_add_symbol(&table, value_tok, int_type);

    /* Search for similar to "coutn" (typo of "count") */
    const char *suggestion = find_similar_symbol(&table, "coutn", 5);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "count") == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_find_similar_symbol_no_match(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a symbol */
    Token tok;
    tok.type = TOKEN_IDENTIFIER;
    tok.start = "xyz";
    tok.length = 3;
    tok.line = 1;
    tok.filename = "test.sn";
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    symbol_table_add_symbol(&table, tok, int_type);

    /* Search for "abcd" - too different from "xyz" */
    const char *suggestion = find_similar_symbol(&table, "abcd", 4);
    assert(suggestion == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test find_similar_method */
static void test_find_similar_method_array(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Search for "pusj" (typo of "push") */
    const char *suggestion = find_similar_method(array_type, "pusj");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "push") == 0);

    /* Search for "lenth" (typo of "length") */
    suggestion = find_similar_method(array_type, "lenth");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "length") == 0);

    arena_free(&arena);
}

static void test_find_similar_method_string(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Search for "substr" (similar to "substring") */
    const char *suggestion = find_similar_method(string_type, "substr");
    /* Note: "substr" to "substring" has distance 3 (add 'ing'), so might not suggest */
    /* Let's try "substrin" (distance 1 from "substring") */
    suggestion = find_similar_method(string_type, "substrin");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "substring") == 0);

    /* Search for "trime" (typo of "trim") */
    suggestion = find_similar_method(string_type, "trime");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "trim") == 0);

    arena_free(&arena);
}

static void test_find_similar_method_no_match(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* INT type has no methods */
    const char *suggestion = find_similar_method(int_type, "push");
    assert(suggestion == NULL);

    arena_free(&arena);
}

/* Test various typo patterns for "did you mean" suggestions */
static void test_find_similar_symbol_typo_patterns(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Add several symbols */
    Token tok1;
    tok1.type = TOKEN_IDENTIFIER;
    tok1.start = "counter";
    tok1.length = 7;
    tok1.line = 1;
    tok1.filename = "test.sn";
    symbol_table_add_symbol(&table, tok1, int_type);

    Token tok2;
    tok2.type = TOKEN_IDENTIFIER;
    tok2.start = "index";
    tok2.length = 5;
    tok2.line = 2;
    tok2.filename = "test.sn";
    symbol_table_add_symbol(&table, tok2, int_type);

    Token tok3;
    tok3.type = TOKEN_IDENTIFIER;
    tok3.start = "total";
    tok3.length = 5;
    tok3.line = 3;
    tok3.filename = "test.sn";
    symbol_table_add_symbol(&table, tok3, int_type);

    /* Test: transposition typo "ocunter" -> "counter" */
    const char *suggestion = find_similar_symbol(&table, "ocunter", 7);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "counter") == 0);

    /* Test: missing character "inde" -> "index" */
    suggestion = find_similar_symbol(&table, "inde", 4);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "index") == 0);

    /* Test: extra character "totall" -> "total" */
    suggestion = find_similar_symbol(&table, "totall", 6);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "total") == 0);

    /* Test: substitution typo "tatal" -> "total" */
    suggestion = find_similar_symbol(&table, "tatal", 5);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "total") == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test case sensitivity in symbol lookup */
static void test_find_similar_symbol_case_sensitivity(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_IDENTIFIER;
    tok.start = "Counter";
    tok.length = 7;
    tok.line = 1;
    tok.filename = "test.sn";
    symbol_table_add_symbol(&table, tok, int_type);

    /* "counter" vs "Counter" - one character difference (case) */
    const char *suggestion = find_similar_symbol(&table, "counter", 7);
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "Counter") == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test method suggestions for arrays */
static void test_find_similar_method_array_typos(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* "pus" -> "push" (missing char) */
    const char *suggestion = find_similar_method(array_type, "pus");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "push") == 0);

    /* "clera" -> "clear" (transposition) */
    suggestion = find_similar_method(array_type, "clera");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "clear") == 0);

    /* "concatt" -> "concat" (extra char) */
    suggestion = find_similar_method(array_type, "concatt");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "concat") == 0);

    /* "revrese" -> "reverse" (transposition) */
    suggestion = find_similar_method(array_type, "revrese");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "reverse") == 0);

    arena_free(&arena);
}

/* Test method suggestions for strings */
static void test_find_similar_method_string_typos(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* "indexof" -> "indexOf" (case difference) */
    const char *suggestion = find_similar_method(string_type, "indexof");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "indexOf") == 0);

    /* "splt" -> "split" (missing char) */
    suggestion = find_similar_method(string_type, "splt");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "split") == 0);

    /* "toLower" exists, test "tolower" */
    suggestion = find_similar_method(string_type, "tolower");
    assert(suggestion != NULL);
    assert(strcmp(suggestion, "toLower") == 0);

    arena_free(&arena);
}

/* Test that very different names don't get suggestions */
static void test_find_similar_symbol_too_different(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_IDENTIFIER;
    tok.start = "abc";
    tok.length = 3;
    tok.line = 1;
    tok.filename = "test.sn";
    symbol_table_add_symbol(&table, tok, int_type);

    /* "xyz" is completely different from "abc" - should not suggest */
    const char *suggestion = find_similar_symbol(&table, "xyz", 3);
    assert(suggestion == NULL);

    /* "abcdefghij" is too different in length */
    suggestion = find_similar_symbol(&table, "abcdefghij", 10);
    assert(suggestion == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Levenshtein distance edge cases */
static void test_levenshtein_distance_edge_cases(void)
{
    /* Same string with different cases */
    int dist = levenshtein_distance("Hello", 5, "hello", 5);
    assert(dist == 1);  /* One substitution (H -> h) */

    /* One character strings */
    dist = levenshtein_distance("a", 1, "b", 1);
    assert(dist == 1);

    /* Single character vs empty */
    dist = levenshtein_distance("a", 1, "", 0);
    assert(dist == 1);

    /* Repeated characters */
    dist = levenshtein_distance("aaa", 3, "aaaa", 4);
    assert(dist == 1);  /* One insertion */

    /* Complete replacement needed */
    dist = levenshtein_distance("cat", 3, "dog", 3);
    assert(dist == 3);  /* Three substitutions */
}

/* Test find_similar_method with NULL type */
static void test_find_similar_method_null_type(void)
{
    const char *suggestion = find_similar_method(NULL, "push");
    assert(suggestion == NULL);
}

/* Test find_similar_symbol with empty table */
static void test_find_similar_symbol_empty_table(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    const char *suggestion = find_similar_symbol(&table, "anything", 8);
    assert(suggestion == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that exact matches are not suggested (distance must be > 0) */
static void test_find_similar_symbol_exact_match(void)
{
    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_IDENTIFIER;
    tok.start = "count";
    tok.length = 5;
    tok.line = 1;
    tok.filename = "test.sn";
    symbol_table_add_symbol(&table, tok, int_type);

    /* Searching for "count" when "count" exists - should not suggest itself */
    const char *suggestion = find_similar_symbol(&table, "count", 5);
    assert(suggestion == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_errors_main(void)
{
    TEST_SECTION("Type Checker Errors");

    /* Core Levenshtein distance tests */
    TEST_RUN("levenshtein_distance_identical", test_levenshtein_distance_identical);
    TEST_RUN("levenshtein_distance_one_char_diff", test_levenshtein_distance_one_char_diff);
    TEST_RUN("levenshtein_distance_insertion", test_levenshtein_distance_insertion);
    TEST_RUN("levenshtein_distance_deletion", test_levenshtein_distance_deletion);
    TEST_RUN("levenshtein_distance_empty", test_levenshtein_distance_empty);
    TEST_RUN("levenshtein_distance_completely_different", test_levenshtein_distance_completely_different);
    TEST_RUN("levenshtein_distance_edge_cases", test_levenshtein_distance_edge_cases);

    /* Symbol suggestion tests */
    TEST_RUN("find_similar_symbol_basic", test_find_similar_symbol_basic);
    TEST_RUN("find_similar_symbol_no_match", test_find_similar_symbol_no_match);
    TEST_RUN("find_similar_symbol_typo_patterns", test_find_similar_symbol_typo_patterns);
    TEST_RUN("find_similar_symbol_case_sensitivity", test_find_similar_symbol_case_sensitivity);
    TEST_RUN("find_similar_symbol_too_different", test_find_similar_symbol_too_different);
    TEST_RUN("find_similar_symbol_empty_table", test_find_similar_symbol_empty_table);
    TEST_RUN("find_similar_symbol_exact_match", test_find_similar_symbol_exact_match);

    /* Method suggestion tests */
    TEST_RUN("find_similar_method_array", test_find_similar_method_array);
    TEST_RUN("find_similar_method_string", test_find_similar_method_string);
    TEST_RUN("find_similar_method_no_match", test_find_similar_method_no_match);
    TEST_RUN("find_similar_method_array_typos", test_find_similar_method_array_typos);
    TEST_RUN("find_similar_method_string_typos", test_find_similar_method_string_typos);
    TEST_RUN("find_similar_method_null_type", test_find_similar_method_null_type);
}
