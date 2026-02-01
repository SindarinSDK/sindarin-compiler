// src/lexer/lexer_scan_keyword.c
// Keyword and Identifier Scanning

SnTokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, SnTokenType type)
{
    int lexeme_length = (int)(lexer->current - lexer->start);
    if (lexeme_length == start + length &&
        memcmp(lexer->start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

SnTokenType lexer_identifier_type(Lexer *lexer)
{
    switch (lexer->start[0])
    {
    case 'a':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 's':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_AS);
            case 'n':
                return lexer_check_keyword(lexer, 2, 1, "y", TOKEN_ANY);
            }
        }
        break;
    case 'b':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
            case 'r':
                return lexer_check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
            case 'y':
                return lexer_check_keyword(lexer, 2, 2, "te", TOKEN_BYTE);
            }
        }
        break;
    case 'c':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'h':
                return lexer_check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR);
            case 'o':
                return lexer_check_keyword(lexer, 2, 6, "ntinue", TOKEN_CONTINUE);
            }
        }
        break;
    case 'd':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 4, "uble", TOKEN_DOUBLE);
            }
        }
        break;
    case 'e':
        return lexer_check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 3, "lse", TOKEN_BOOL_LITERAL);
            case 'l':
                return lexer_check_keyword(lexer, 2, 3, "oat", TOKEN_FLOAT);
            case 'n':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_FN);
            case 'o':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 'i':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'f':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IF);
            case 'm':
                return lexer_check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
            case 'n':
                // Check for "in" (2 chars) vs "int" (3 chars) vs "int32" (5 chars)
                if (lexer->current - lexer->start == 2)
                {
                    return TOKEN_IN;
                }
                if (lexer->current - lexer->start == 5)
                {
                    return lexer_check_keyword(lexer, 2, 3, "t32", TOKEN_INT32);
                }
                return lexer_check_keyword(lexer, 2, 1, "t", TOKEN_INT);
            case 's':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IS);
            }
        }
        break;
    case 'l':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                // Check for "long" (4 chars) vs "lock" (4 chars)
                if (lexer->current - lexer->start == 4 && lexer->start[2] == 'c')
                {
                    return lexer_check_keyword(lexer, 2, 2, "ck", TOKEN_LOCK);
                }
                return lexer_check_keyword(lexer, 2, 2, "ng", TOKEN_LONG);
            }
        }
        break;
    case 'm':
        return lexer_check_keyword(lexer, 1, 4, "atch", TOKEN_MATCH);
    case 'n':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 4, "tive", TOKEN_NATIVE);
            case 'i':
                return lexer_check_keyword(lexer, 2, 1, "l", TOKEN_NIL);
            }
        }
        break;
    case 'p':
        return lexer_check_keyword(lexer, 1, 6, "rivate", TOKEN_PRIVATE);
    case 'r':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'e':
                // Check for "ref" (3 chars) vs "return" (6 chars)
                if (lexer->current - lexer->start == 3)
                {
                    return lexer_check_keyword(lexer, 2, 1, "f", TOKEN_REF);
                }
                return lexer_check_keyword(lexer, 2, 4, "turn", TOKEN_RETURN);
            }
        }
        break;
    case 's':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 't':
                // Check for "str" (3 chars) vs "struct" (6 chars) vs "static" (6 chars) vs "string" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    // "struct", "static", and "string" are 6 chars, check 3rd char
                    if (lexer->start[2] == 'r')
                    {
                        // "string" vs "struct" - check 4th char: 'i' vs 'u'
                        if (lexer->start[3] == 'i')
                        {
                            return lexer_check_keyword(lexer, 2, 4, "ring", TOKEN_STR);
                        }
                        return lexer_check_keyword(lexer, 2, 4, "ruct", TOKEN_STRUCT);
                    }
                    return lexer_check_keyword(lexer, 2, 4, "atic", TOKEN_STATIC);
                }
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_STR);
            case 'h':
                return lexer_check_keyword(lexer, 2, 4, "ared", TOKEN_SHARED);
            case 'i':
                return lexer_check_keyword(lexer, 2, 4, "zeof", TOKEN_SIZEOF);
            case 'y':
                return lexer_check_keyword(lexer, 2, 2, "nc", TOKEN_SYNC);
            }
        }
        break;
    case 't':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'r':
                return lexer_check_keyword(lexer, 2, 2, "ue", TOKEN_BOOL_LITERAL);
            case 'y':
                // Check for "type" (4 chars) vs "typeof" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    return lexer_check_keyword(lexer, 2, 4, "peof", TOKEN_TYPEOF);
                }
                return lexer_check_keyword(lexer, 2, 2, "pe", TOKEN_KEYWORD_TYPE);
            }
        }
        break;
    case 'o':
        return lexer_check_keyword(lexer, 1, 5, "paque", TOKEN_OPAQUE);
    case 'u':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'i':
                // Check for "uint" (4 chars) vs "uint32" (6 chars)
                if (lexer->current - lexer->start == 6)
                {
                    return lexer_check_keyword(lexer, 2, 4, "nt32", TOKEN_UINT32);
                }
                return lexer_check_keyword(lexer, 2, 2, "nt", TOKEN_UINT);
            }
        }
        break;
    case 'v':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                // Check for "val" (3 chars) vs "var" (3 chars)
                if (lexer->current - lexer->start == 3)
                {
                    if (lexer->start[2] == 'l')
                    {
                        return TOKEN_VAL;
                    }
                    else if (lexer->start[2] == 'r')
                    {
                        return TOKEN_VAR;
                    }
                }
                break;
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
            }
        }
        break;
    case 'w':
        return lexer_check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_scan_identifier(Lexer *lexer)
{
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_')
    {
        lexer_advance(lexer);
    }
    SnTokenType type = lexer_identifier_type(lexer);
    if (type == TOKEN_BOOL_LITERAL)
    {
        Token token = lexer_make_token(lexer, type);
        if (memcmp(lexer->start, "true", 4) == 0)
        {
            token_set_bool_literal(&token, 1);
        }
        else
        {
            token_set_bool_literal(&token, 0);
        }
        return token;
    }
    return lexer_make_token(lexer, type);
}
