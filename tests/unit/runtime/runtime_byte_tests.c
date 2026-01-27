// tests/unit/runtime/runtime_byte_tests.c
// Tests for runtime byte array conversion functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Byte Array to String Tests (UTF-8)
 * ============================================================================ */

static void test_rt_byte_array_to_string_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create byte array from ASCII string */
    unsigned char *bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'H';
    bytes[1] = 'e';
    bytes[2] = 'l';
    bytes[3] = 'l';
    bytes[4] = 'o';

    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, "Hello") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Empty byte array */
    unsigned char *bytes = rt_array_alloc_byte(arena, 0, 0);
    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* NULL byte array */
    char *result = rt_byte_array_to_string(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_with_null_bytes(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Byte array with null byte in middle - stops at null due to strlen behavior */
    unsigned char *bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'a';
    bytes[1] = 'b';
    bytes[2] = 0;  /* null byte */
    bytes[3] = 'c';
    bytes[4] = 'd';

    char *result = rt_byte_array_to_string(arena, bytes);
    /* Result should be "ab" followed by null, then "cd" - but strlen will show "ab" */
    assert(result[0] == 'a');
    assert(result[1] == 'b');
    assert(result[2] == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_binary_data(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Binary data with high byte values */
    unsigned char *bytes = rt_array_alloc_byte(arena, 4, 0);
    bytes[0] = 0x41;  /* 'A' */
    bytes[1] = 0x42;  /* 'B' */
    bytes[2] = 0x43;  /* 'C' */
    bytes[3] = 0x44;  /* 'D' */

    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, "ABCD") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Byte Array to Latin-1 String Tests
 * ============================================================================ */

static void test_rt_byte_array_to_string_latin1_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* ASCII bytes - should be same as UTF-8 */
    unsigned char *bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'H';
    bytes[1] = 'e';
    bytes[2] = 'l';
    bytes[3] = 'l';
    bytes[4] = 'o';

    char *result = rt_byte_array_to_string_latin1(arena, bytes);
    assert(strcmp(result, "Hello") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_latin1_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 0, 0);
    char *result = rt_byte_array_to_string_latin1(arena, bytes);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_latin1_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_byte_array_to_string_latin1(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_latin1_high_bytes(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Latin-1 high bytes (0x80-0xFF) - should be UTF-8 encoded */
    unsigned char *bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 0xC0;  /* Latin-1 'À' */
    bytes[1] = 0xFF;  /* Latin-1 'ÿ' */

    char *result = rt_byte_array_to_string_latin1(arena, bytes);
    /* 0xC0 becomes UTF-8 0xC3 0x80, 0xFF becomes 0xC3 0xBF */
    assert((unsigned char)result[0] == 0xC3);
    assert((unsigned char)result[1] == 0x80);
    assert((unsigned char)result[2] == 0xC3);
    assert((unsigned char)result[3] == 0xBF);
    assert(result[4] == '\0');

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_string_latin1_mixed(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Mix of ASCII and high bytes */
    unsigned char *bytes = rt_array_alloc_byte(arena, 4, 0);
    bytes[0] = 'A';    /* ASCII */
    bytes[1] = 0xE9;   /* Latin-1 'é' */
    bytes[2] = 'B';    /* ASCII */
    bytes[3] = 0xFC;   /* Latin-1 'ü' */

    char *result = rt_byte_array_to_string_latin1(arena, bytes);
    /* 'A', then 0xC3 0xA9 for 'é', 'B', then 0xC3 0xBC for 'ü' */
    assert(result[0] == 'A');
    assert((unsigned char)result[1] == 0xC3);
    assert((unsigned char)result[2] == 0xA9);
    assert(result[3] == 'B');
    assert((unsigned char)result[4] == 0xC3);
    assert((unsigned char)result[5] == 0xBC);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Byte Array to Hex String Tests
 * ============================================================================ */

static void test_rt_byte_array_to_hex_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 4, 0);
    bytes[0] = 0xDE;
    bytes[1] = 0xAD;
    bytes[2] = 0xBE;
    bytes[3] = 0xEF;

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "deadbeef") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 0, 0);
    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_byte_array_to_hex(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_zeros(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 4, 0);
    bytes[0] = 0x00;
    bytes[1] = 0x00;
    bytes[2] = 0x00;
    bytes[3] = 0x00;

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "00000000") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_all_values(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 16, 0);
    for (int i = 0; i < 16; i++) {
        bytes[i] = (unsigned char)i;
    }

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "000102030405060708090a0b0c0d0e0f") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_single_byte(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 1, 0);
    bytes[0] = 0xFF;

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "ff") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_hex_leading_zero(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 0x0A;
    bytes[1] = 0x0B;

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "0a0b") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Byte Array to Base64 String Tests
 * ============================================================================ */

static void test_rt_byte_array_to_base64_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* "Man" -> "TWFu" */
    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 'M';
    bytes[1] = 'a';
    bytes[2] = 'n';

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TWFu") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 0, 0);
    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_byte_array_to_base64(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_one_byte(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* "M" -> "TQ==" */
    unsigned char *bytes = rt_array_alloc_byte(arena, 1, 0);
    bytes[0] = 'M';

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TQ==") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_two_bytes(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* "Ma" -> "TWE=" */
    unsigned char *bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 'M';
    bytes[1] = 'a';

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TWE=") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_hello(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* "Hello" -> "SGVsbG8=" */
    unsigned char *bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'H';
    bytes[1] = 'e';
    bytes[2] = 'l';
    bytes[3] = 'l';
    bytes[4] = 'o';

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "SGVsbG8=") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_binary(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Binary data with all bits set */
    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 0xFF;
    bytes[1] = 0xFF;
    bytes[2] = 0xFF;

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "////") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_zeros(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 0x00;
    bytes[1] = 0x00;
    bytes[2] = 0x00;

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "AAAA") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_byte_array_to_base64_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* "Hello, World!" -> "SGVsbG8sIFdvcmxkIQ==" */
    const char *input = "Hello, World!";
    size_t len = strlen(input);
    unsigned char *bytes = rt_array_alloc_byte(arena, (int)len, 0);
    for (size_t i = 0; i < len; i++) {
        bytes[i] = (unsigned char)input[i];
    }

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "SGVsbG8sIFdvcmxkIQ==") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String to Byte Array Tests
 * ============================================================================ */

static void test_rt_string_to_bytes_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, "Hello");
    assert(rt_array_length(bytes) == 5);
    assert(bytes[0] == 'H');
    assert(bytes[1] == 'e');
    assert(bytes[2] == 'l');
    assert(bytes[3] == 'l');
    assert(bytes[4] == 'o');

    rt_arena_destroy(arena);
}

static void test_rt_string_to_bytes_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, "");
    assert(rt_array_length(bytes) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_string_to_bytes_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, NULL);
    assert(rt_array_length(bytes) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_string_to_bytes_special_chars(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, "\t\n\r");
    assert(rt_array_length(bytes) == 3);
    assert(bytes[0] == '\t');
    assert(bytes[1] == '\n');
    assert(bytes[2] == '\r');

    rt_arena_destroy(arena);
}

static void test_rt_string_to_bytes_numbers(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, "12345");
    assert(rt_array_length(bytes) == 5);
    assert(bytes[0] == '1');
    assert(bytes[1] == '2');
    assert(bytes[2] == '3');
    assert(bytes[3] == '4');
    assert(bytes[4] == '5');

    rt_arena_destroy(arena);
}

static void test_rt_string_to_bytes_single_char(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *bytes = rt_string_to_bytes(arena, "X");
    assert(rt_array_length(bytes) == 1);
    assert(bytes[0] == 'X');

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Round-Trip Tests
 * ============================================================================ */

static void test_byte_string_roundtrip(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* String -> Bytes -> String */
    const char *original = "Hello, World!";
    unsigned char *bytes = rt_string_to_bytes(arena, original);
    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, original) == 0);

    rt_arena_destroy(arena);
}

static void test_byte_string_roundtrip_ascii(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* All printable ASCII characters */
    char original[95 + 1];
    for (int i = 0; i < 95; i++) {
        original[i] = (char)(32 + i);  /* ' ' to '~' */
    }
    original[95] = '\0';

    unsigned char *bytes = rt_string_to_bytes(arena, original);
    assert(rt_array_length(bytes) == 95);

    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, original) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_byte_array_large(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create large byte array */
    int size = 10000;
    unsigned char *bytes = rt_array_alloc_byte(arena, size, 0);
    for (int i = 0; i < size; i++) {
        bytes[i] = (unsigned char)(i % 256);
    }

    /* Convert to hex (20000 chars) */
    char *hex = rt_byte_array_to_hex(arena, bytes);
    assert(strlen(hex) == (size_t)(size * 2));

    /* Convert to base64 */
    char *b64 = rt_byte_array_to_base64(arena, bytes);
    assert(b64 != NULL);
    assert(strlen(b64) > 0);

    rt_arena_destroy(arena);
}

static void test_byte_array_repeated(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Multiple conversions */
    for (int iter = 0; iter < 100; iter++) {
        unsigned char *bytes = rt_array_alloc_byte(arena, 10, 0);
        for (int i = 0; i < 10; i++) {
            bytes[i] = (unsigned char)(iter + i);
        }
        char *hex = rt_byte_array_to_hex(arena, bytes);
        assert(strlen(hex) == 20);
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_byte_main(void)
{
    TEST_SECTION("Runtime Byte Array - To String (UTF-8)");
    TEST_RUN("rt_byte_array_to_string_basic", test_rt_byte_array_to_string_basic);
    TEST_RUN("rt_byte_array_to_string_empty", test_rt_byte_array_to_string_empty);
    TEST_RUN("rt_byte_array_to_string_null", test_rt_byte_array_to_string_null);
    TEST_RUN("rt_byte_array_to_string_with_null_bytes", test_rt_byte_array_to_string_with_null_bytes);
    TEST_RUN("rt_byte_array_to_string_binary_data", test_rt_byte_array_to_string_binary_data);

    TEST_SECTION("Runtime Byte Array - To Latin-1 String");
    TEST_RUN("rt_byte_array_to_string_latin1_basic", test_rt_byte_array_to_string_latin1_basic);
    TEST_RUN("rt_byte_array_to_string_latin1_empty", test_rt_byte_array_to_string_latin1_empty);
    TEST_RUN("rt_byte_array_to_string_latin1_null", test_rt_byte_array_to_string_latin1_null);
    TEST_RUN("rt_byte_array_to_string_latin1_high_bytes", test_rt_byte_array_to_string_latin1_high_bytes);
    TEST_RUN("rt_byte_array_to_string_latin1_mixed", test_rt_byte_array_to_string_latin1_mixed);

    TEST_SECTION("Runtime Byte Array - To Hex");
    TEST_RUN("rt_byte_array_to_hex_basic", test_rt_byte_array_to_hex_basic);
    TEST_RUN("rt_byte_array_to_hex_empty", test_rt_byte_array_to_hex_empty);
    TEST_RUN("rt_byte_array_to_hex_null", test_rt_byte_array_to_hex_null);
    TEST_RUN("rt_byte_array_to_hex_zeros", test_rt_byte_array_to_hex_zeros);
    TEST_RUN("rt_byte_array_to_hex_all_values", test_rt_byte_array_to_hex_all_values);
    TEST_RUN("rt_byte_array_to_hex_single_byte", test_rt_byte_array_to_hex_single_byte);
    TEST_RUN("rt_byte_array_to_hex_leading_zero", test_rt_byte_array_to_hex_leading_zero);

    TEST_SECTION("Runtime Byte Array - To Base64");
    TEST_RUN("rt_byte_array_to_base64_basic", test_rt_byte_array_to_base64_basic);
    TEST_RUN("rt_byte_array_to_base64_empty", test_rt_byte_array_to_base64_empty);
    TEST_RUN("rt_byte_array_to_base64_null", test_rt_byte_array_to_base64_null);
    TEST_RUN("rt_byte_array_to_base64_one_byte", test_rt_byte_array_to_base64_one_byte);
    TEST_RUN("rt_byte_array_to_base64_two_bytes", test_rt_byte_array_to_base64_two_bytes);
    TEST_RUN("rt_byte_array_to_base64_hello", test_rt_byte_array_to_base64_hello);
    TEST_RUN("rt_byte_array_to_base64_binary", test_rt_byte_array_to_base64_binary);
    TEST_RUN("rt_byte_array_to_base64_zeros", test_rt_byte_array_to_base64_zeros);
    TEST_RUN("rt_byte_array_to_base64_long", test_rt_byte_array_to_base64_long);

    TEST_SECTION("Runtime Byte Array - String to Bytes");
    TEST_RUN("rt_string_to_bytes_basic", test_rt_string_to_bytes_basic);
    TEST_RUN("rt_string_to_bytes_empty", test_rt_string_to_bytes_empty);
    TEST_RUN("rt_string_to_bytes_null", test_rt_string_to_bytes_null);
    TEST_RUN("rt_string_to_bytes_special_chars", test_rt_string_to_bytes_special_chars);
    TEST_RUN("rt_string_to_bytes_numbers", test_rt_string_to_bytes_numbers);
    TEST_RUN("rt_string_to_bytes_single_char", test_rt_string_to_bytes_single_char);

    TEST_SECTION("Runtime Byte Array - Round Trip");
    TEST_RUN("byte_string_roundtrip", test_byte_string_roundtrip);
    TEST_RUN("byte_string_roundtrip_ascii", test_byte_string_roundtrip_ascii);

    TEST_SECTION("Runtime Byte Array - Stress Tests");
    TEST_RUN("byte_array_large", test_byte_array_large);
    TEST_RUN("byte_array_repeated", test_byte_array_repeated);
}
