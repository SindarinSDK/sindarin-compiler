// tests/ast_tests.c
// AST tests - main entry point

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../ast.h"
#include "../debug.h"
#include "../token.h"
#include "../test_harness.h"

// Helper functions
static void setup_arena(Arena *arena)
{
    arena_init(arena, 4096);
}

static void cleanup_arena(Arena *arena)
{
    arena_free(arena);
}

static int tokens_equal(const Token *a, const Token *b)
{
    if (a == NULL && b == NULL)
        return 1;
    if (a == NULL || b == NULL)
        return 0;
    return a->type == b->type &&
           a->length == b->length &&
           a->line == b->line &&
           strcmp(a->start, b->start) == 0 &&
           strcmp(a->filename, b->filename) == 0;
}

static Token create_dummy_token(Arena *arena, const char *lexeme)
{
    Token t;
    t.type = TOKEN_IDENTIFIER;
    t.start = arena_strdup(arena, lexeme);
    t.length = strlen(lexeme);
    t.line = 1;
    t.filename = "test.sn";
    return t;
}

#include "ast_tests_type.c"
#include "ast_tests_expr.c"
#include "ast_tests_stmt.c"
#include "ast_tests_util.c"

void test_ast_main()
{
    test_ast_type_main();
    test_ast_expr_main();
    test_ast_stmt_main();
    test_ast_util_main();
}
