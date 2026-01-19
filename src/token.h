#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum
{
    TOKEN_EOF,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NEWLINE,
    TOKEN_INT_LITERAL,
    TOKEN_LONG_LITERAL,
    TOKEN_BYTE_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_UINT_LITERAL,
    TOKEN_UINT32_LITERAL,
    TOKEN_INT32_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_INTERPOL_STRING,
    TOKEN_ARRAY_LITERAL,
    TOKEN_BOOL_LITERAL,
    TOKEN_IDENTIFIER,
    TOKEN_FN,
    TOKEN_VAR,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_IN,
    TOKEN_IMPORT,
    TOKEN_NIL,
    TOKEN_INT,
    TOKEN_INT32,
    TOKEN_UINT,
    TOKEN_UINT32,
    TOKEN_LONG,
    TOKEN_DOUBLE,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_STR,
    TOKEN_BOOL,
    TOKEN_BYTE,
    TOKEN_VOID,
    // Memory management keywords
    TOKEN_SHARED,
    TOKEN_PRIVATE,
    TOKEN_AS,
    TOKEN_VAL,
    TOKEN_REF,
    // Native interop keyword
    TOKEN_NATIVE,
    // Type declaration keywords
    TOKEN_KEYWORD_TYPE,
    TOKEN_OPAQUE,
    // Struct keyword
    TOKEN_STRUCT,
    // Static keyword (for static methods)
    TOKEN_STATIC,
    // Any type and type operators
    TOKEN_ANY,
    TOKEN_TYPEOF,
    TOKEN_IS,
    // Sizeof operator
    TOKEN_SIZEOF,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_MODULO,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_AMPERSAND,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    // Compound assignment operators
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH_EQUAL,
    TOKEN_MODULO_EQUAL,
    // Synchronization keywords
    TOKEN_SYNC,
    TOKEN_LOCK,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_RANGE,
    TOKEN_SPREAD,
    TOKEN_ARROW,
    TOKEN_PRAGMA,
    TOKEN_PRAGMA_INCLUDE,
    TOKEN_PRAGMA_LINK,
    TOKEN_PRAGMA_SOURCE,      /* #pragma source "file.c" */
    TOKEN_PRAGMA_PACK,        /* #pragma pack(1) or #pragma pack() */
    TOKEN_PRAGMA_ALIAS,       /* #pragma alias "c_name" for next declaration */
    TOKEN_ERROR
} SnTokenType;

typedef union
{
    int64_t int_value;
    double double_value;
    char char_value;
    const char *string_value;
    int bool_value;
} LiteralValue;

typedef struct
{
    SnTokenType type;
    const char *start;
    int length;
    int line;
    const char *filename;
    LiteralValue literal;
} Token;

void token_init(Token *token, SnTokenType type, const char *start, int length, int line, const char *filename);
void token_set_int_literal(Token *token, int64_t value);
void token_set_double_literal(Token *token, double value);
void token_set_char_literal(Token *token, char value);
void token_set_string_literal(Token *token, const char *value);
void token_set_array_literal(Token *token, const char *value);
void token_set_bool_literal(Token *token, int value);
const char *token_type_to_string(SnTokenType type);
void token_print(Token *token);

#endif