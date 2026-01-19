// tests/code_gen_tests_util.c
// Helper functions and basic code gen tests

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"
#include "../file.h"
#include "../test_utils.h"

static const char *test_output_path = "test_output.c";
static const char *expected_output_path = "expected_output.c";

/* Use shared header from test_utils.h */
const char *get_expected(Arena *arena, const char *expected)
{
    return build_expected_output(arena, expected);
}

void create_expected_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    if (content)
    {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, file);
        assert(written == len);
    }
    fclose(file);
}

void remove_test_file(const char *path)
{
    remove(path);
}

/* Normalize line endings by removing \r characters (for cross-platform comparison) */
static char *normalize_line_endings(Arena *arena, const char *str)
{
    if (str == NULL) return NULL;

    size_t len = strlen(str);
    char *result = arena_alloc(arena, len + 1);
    if (result == NULL) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] != '\r') {
            result[j++] = str[i];
        }
    }
    result[j] = '\0';
    return result;
}

void compare_output_files(const char *actual_path, const char *expected_path)
{
    DEBUG_VERBOSE("Entering compare_output_files with actual_path=%s, expected_path=%s", actual_path, expected_path);

    Arena read_arena;
    DEBUG_VERBOSE("Initializing arena with size=1MB");
    arena_init(&read_arena, 1024 * 1024);

    DEBUG_VERBOSE("Reading actual file: %s", actual_path);
    char *actual_raw = file_read(&read_arena, actual_path);
    DEBUG_VERBOSE("Actual file contents: %s", actual_raw ? actual_raw : "NULL");

    DEBUG_VERBOSE("Reading expected file: %s", expected_path);
    char *expected_raw = file_read(&read_arena, expected_path);
    DEBUG_VERBOSE("Expected file contents: %s", expected_raw ? expected_raw : "NULL");

    DEBUG_VERBOSE("Checking if file contents are non-null");
    assert(actual_raw != NULL && expected_raw != NULL);

    /* Normalize line endings for cross-platform comparison */
    char *actual = normalize_line_endings(&read_arena, actual_raw);
    char *expected = normalize_line_endings(&read_arena, expected_raw);

    DEBUG_VERBOSE("Comparing file contents");
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "=== FILE COMPARISON FAILED ===\n");
        fprintf(stderr, "Actual length: %zu, Expected length: %zu\n", strlen(actual), strlen(expected));
        // Find first difference
        size_t i = 0;
        while (actual[i] && expected[i] && actual[i] == expected[i]) i++;
        fprintf(stderr, "First difference at position %zu:\n", i);
        fprintf(stderr, "Actual:   0x%02x '%c'\n", (unsigned char)actual[i], actual[i] > 31 ? actual[i] : '?');
        fprintf(stderr, "Expected: 0x%02x '%c'\n", (unsigned char)expected[i], expected[i] > 31 ? expected[i] : '?');
        // Show context
        size_t start = i > 50 ? i - 50 : 0;
        fprintf(stderr, "Context around difference (actual):\n%.100s\n", actual + start);
        fprintf(stderr, "Context around difference (expected):\n%.100s\n", expected + start);
    }
    assert(strcmp(actual, expected) == 0);

    DEBUG_VERBOSE("Freeing arena");
    arena_free(&read_arena);
}

void setup_basic_token(Token *token, SnTokenType type, const char *lexeme)
{
    token_init(token, type, lexeme, (int)strlen(lexeme), 1, "test.sn");
}

static void test_code_gen_cleanup_null_output(void)
{
    DEBUG_INFO("Starting test_code_gen_cleanup_null_output");

    Arena arena;
    arena_init(&arena, 1024);
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    gen.output = NULL; // Simulate

    code_gen_cleanup(&gen); // Should do nothing

    symbol_table_cleanup(&sym_table);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_cleanup_null_output");
}

static void test_code_gen_headers_and_externs(void)
{
    DEBUG_INFO("Starting test_code_gen_headers_and_externs");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");
    code_gen_module(&gen, &module);

    // Expected with full headers and externs + dummy main with arena
    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_headers_and_externs");
}

void test_code_gen_util_main(void)
{
    TEST_SECTION("Code Gen Util Tests");
    TEST_RUN("code_gen_cleanup_null_output", test_code_gen_cleanup_null_output);
    TEST_RUN("code_gen_headers_and_externs", test_code_gen_headers_and_externs);
}
