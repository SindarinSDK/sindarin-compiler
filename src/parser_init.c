// parser_init.c
// Parser initialization with built-in function registration

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
    parser->arena = arena;
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table;
    parser->sized_array_pending = 0;
    parser->sized_array_size = NULL;
    parser->in_native_function = 0;
    parser->pack_alignment = 0;  /* 0 = default alignment, 1 = packed */
    parser->pending_alias = NULL;  /* No pending alias initially */
    parser->pending_serializable = false;  /* No pending @serializable initially */
    parser->pending_comments = NULL;  /* No pending comments initially */
    parser->pending_comment_count = 0;
    parser->pending_comment_capacity = 0;
    parser->continuation_indent_depth = 0;  /* No pending continuation dedents */

    Token print_token;
    print_token.start = arena_strdup(arena, "print");
    print_token.length = 5;
    print_token.type = TOKEN_IDENTIFIER;
    print_token.line = 0;
    print_token.filename = arena_strdup(arena, "<built-in>");

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), NULL, 0);
    print_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), NULL, 0);
    to_string_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);

    // Array built-in: len(arr) -> int (works on arrays and strings)
    Token len_token;
    len_token.start = arena_strdup(arena, "len");
    len_token.length = 3;
    len_token.type = TOKEN_IDENTIFIER;
    len_token.line = 0;
    len_token.filename = arena_strdup(arena, "<built-in>");
    Type *len_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_INT), NULL, 0);
    len_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, len_token, len_type, SYMBOL_GLOBAL);

    // Console I/O convenience functions
    // readLine() -> str
    Token readLine_token;
    readLine_token.start = arena_strdup(arena, "readLine");
    readLine_token.length = 8;
    readLine_token.type = TOKEN_IDENTIFIER;
    readLine_token.line = 0;
    readLine_token.filename = arena_strdup(arena, "<built-in>");
    Type *readLine_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), NULL, 0);
    symbol_table_add_symbol_with_kind(parser->symbol_table, readLine_token, readLine_type, SYMBOL_GLOBAL);

    // println(any) -> void
    Token println_token;
    println_token.start = arena_strdup(arena, "println");
    println_token.length = 7;
    println_token.type = TOKEN_IDENTIFIER;
    println_token.line = 0;
    println_token.filename = arena_strdup(arena, "<built-in>");
    Type *println_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), NULL, 0);
    println_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, println_token, println_type, SYMBOL_GLOBAL);

    // printErr(any) -> void
    Token printErr_token;
    printErr_token.start = arena_strdup(arena, "printErr");
    printErr_token.length = 8;
    printErr_token.type = TOKEN_IDENTIFIER;
    printErr_token.line = 0;
    printErr_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErr_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), NULL, 0);
    printErr_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErr_token, printErr_type, SYMBOL_GLOBAL);

    // printErrLn(any) -> void
    Token printErrLn_token;
    printErrLn_token.start = arena_strdup(arena, "printErrLn");
    printErrLn_token.length = 10;
    printErrLn_token.type = TOKEN_IDENTIFIER;
    printErrLn_token.line = 0;
    printErrLn_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErrLn_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), NULL, 0);
    printErrLn_type->as.function.is_variadic = true;
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErrLn_token, printErrLn_type, SYMBOL_GLOBAL);

    // exit(code: int) -> void
    Token exit_token;
    exit_token.start = arena_strdup(arena, "exit");
    exit_token.length = 4;
    exit_token.type = TOKEN_IDENTIFIER;
    exit_token.line = 0;
    exit_token.filename = arena_strdup(arena, "<built-in>");
    Type **exit_params = arena_alloc(arena, sizeof(Type *));
    exit_params[0] = ast_create_primitive_type(arena, TYPE_INT);
    Type *exit_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), exit_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, exit_token, exit_type, SYMBOL_GLOBAL);

    // assert(condition: bool, message: str) -> void
    Token assert_token;
    assert_token.start = arena_strdup(arena, "assert");
    assert_token.length = 6;
    assert_token.type = TOKEN_IDENTIFIER;
    assert_token.line = 0;
    assert_token.filename = arena_strdup(arena, "<built-in>");
    Type **assert_params = arena_alloc(arena, sizeof(Type *) * 2);
    assert_params[0] = ast_create_primitive_type(arena, TYPE_BOOL);
    assert_params[1] = ast_create_primitive_type(arena, TYPE_STRING);
    Type *assert_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), assert_params, 2);
    symbol_table_add_symbol_with_kind(parser->symbol_table, assert_token, assert_type, SYMBOL_GLOBAL);

    // ---- Built-in struct types for reflection: FieldInfo, TypeInfo ----

    // FieldInfo: { name: str, typeName: str, typeId: int }
    {
        StructField *fi_fields = arena_alloc(arena, sizeof(StructField) * 3);
        fi_fields[0] = (StructField){ .name = "name", .type = ast_create_primitive_type(arena, TYPE_STRING), .offset = 0, .default_value = NULL, .c_alias = NULL };
        fi_fields[1] = (StructField){ .name = "typeName", .type = ast_create_primitive_type(arena, TYPE_STRING), .offset = 0, .default_value = NULL, .c_alias = NULL };
        fi_fields[2] = (StructField){ .name = "typeId", .type = ast_create_primitive_type(arena, TYPE_INT), .offset = 0, .default_value = NULL, .c_alias = NULL };

        Type *fieldinfo_type = ast_create_struct_type(arena, "FieldInfo", fi_fields, 3, NULL, 0, false, false, false, NULL);

        Token fi_token;
        fi_token.start = arena_strdup(arena, "FieldInfo");
        fi_token.length = 9;
        fi_token.type = TOKEN_IDENTIFIER;
        fi_token.line = 0;
        fi_token.filename = arena_strdup(arena, "<built-in>");
        symbol_table_add_type(parser->symbol_table, fi_token, fieldinfo_type);

        // TypeInfo: { name: str, fields: [FieldInfo], fieldCount: int, typeId: int }
        StructField *ti_fields = arena_alloc(arena, sizeof(StructField) * 4);
        ti_fields[0] = (StructField){ .name = "name", .type = ast_create_primitive_type(arena, TYPE_STRING), .offset = 0, .default_value = NULL, .c_alias = NULL };
        ti_fields[1] = (StructField){ .name = "fields", .type = ast_create_array_type(arena, fieldinfo_type), .offset = 0, .default_value = NULL, .c_alias = NULL };
        ti_fields[2] = (StructField){ .name = "fieldCount", .type = ast_create_primitive_type(arena, TYPE_INT), .offset = 0, .default_value = NULL, .c_alias = NULL };
        ti_fields[3] = (StructField){ .name = "typeId", .type = ast_create_primitive_type(arena, TYPE_INT), .offset = 0, .default_value = NULL, .c_alias = NULL };

        Type *typeinfo_type = ast_create_struct_type(arena, "TypeInfo", ti_fields, 4, NULL, 0, false, false, false, NULL);

        Token ti_token;
        ti_token.start = arena_strdup(arena, "TypeInfo");
        ti_token.length = 8;
        ti_token.type = TOKEN_IDENTIFIER;
        ti_token.line = 0;
        ti_token.filename = arena_strdup(arena, "<built-in>");
        symbol_table_add_type(parser->symbol_table, ti_token, typeinfo_type);
    }

    // ---- Built-in struct types for serialization: Encoder, Decoder ----

    {
        /* Encoder: native struct with vtable-based dispatch.
         * At the Sindarin level, it's an opaque ref type with methods.
         * The actual C definition is in sn_serial.h. */
        Token enc_tok;
        enc_tok.start = arena_strdup(arena, "Encoder");
        enc_tok.length = 7;
        enc_tok.type = TOKEN_IDENTIFIER;
        enc_tok.line = 0;
        enc_tok.filename = arena_strdup(arena, "<built-in>");

        /* Encoder methods (14 total) — all native, dispatched via vtable macros in sn_serial.h */
        int enc_method_count = 14;
        StructMethod *enc_methods = arena_alloc(arena, sizeof(StructMethod) * enc_method_count);
        memset(enc_methods, 0, sizeof(StructMethod) * enc_method_count);

        /* Helper types */
        Type *t_void = ast_create_primitive_type(arena, TYPE_VOID);
        Type *t_str = ast_create_primitive_type(arena, TYPE_STRING);
        Type *t_int = ast_create_primitive_type(arena, TYPE_INT);
        Type *t_double = ast_create_primitive_type(arena, TYPE_DOUBLE);
        Type *t_bool = ast_create_primitive_type(arena, TYPE_BOOL);

        /* We need a forward reference to the Encoder type for methods that return Encoder.
         * Create the type first, then fill in the methods. */
        Type *encoder_type = ast_create_struct_type(arena, "Encoder", NULL, 0,
                                                     NULL, 0, true, false, true, NULL);

        /* writeStr(key: str, value: str): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 2);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            p[1] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[0] = (StructMethod){ .name = "writeStr", .params = p, .param_count = 2,
                .return_type = t_void, .is_native = true, .name_token = { .start = "writeStr", .length = 8 } };
        }
        /* writeInt(key: str, value: int): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 2);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            p[1] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[1] = (StructMethod){ .name = "writeInt", .params = p, .param_count = 2,
                .return_type = t_void, .is_native = true, .name_token = { .start = "writeInt", .length = 8 } };
        }
        /* writeDouble(key: str, value: double): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 2);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            p[1] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_double, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[2] = (StructMethod){ .name = "writeDouble", .params = p, .param_count = 2,
                .return_type = t_void, .is_native = true, .name_token = { .start = "writeDouble", .length = 11 } };
        }
        /* writeBool(key: str, value: bool): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 2);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            p[1] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_bool, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[3] = (StructMethod){ .name = "writeBool", .params = p, .param_count = 2,
                .return_type = t_void, .is_native = true, .name_token = { .start = "writeBool", .length = 9 } };
        }
        /* writeNull(key: str): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[4] = (StructMethod){ .name = "writeNull", .params = p, .param_count = 1,
                .return_type = t_void, .is_native = true, .name_token = { .start = "writeNull", .length = 9 } };
        }
        /* beginObject(key: str): Encoder */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[5] = (StructMethod){ .name = "beginObject", .params = p, .param_count = 1,
                .return_type = encoder_type, .is_native = true, .name_token = { .start = "beginObject", .length = 11 } };
        }
        /* beginArray(key: str): Encoder */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[6] = (StructMethod){ .name = "beginArray", .params = p, .param_count = 1,
                .return_type = encoder_type, .is_native = true, .name_token = { .start = "beginArray", .length = 10 } };
        }
        /* end(): void */
        {
            enc_methods[7] = (StructMethod){ .name = "end", .params = NULL, .param_count = 0,
                .return_type = t_void, .is_native = true, .name_token = { .start = "end", .length = 3 } };
        }
        /* appendStr(value: str): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[8] = (StructMethod){ .name = "appendStr", .params = p, .param_count = 1,
                .return_type = t_void, .is_native = true, .name_token = { .start = "appendStr", .length = 9 } };
        }
        /* appendInt(value: int): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[9] = (StructMethod){ .name = "appendInt", .params = p, .param_count = 1,
                .return_type = t_void, .is_native = true, .name_token = { .start = "appendInt", .length = 9 } };
        }
        /* appendDouble(value: double): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_double, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[10] = (StructMethod){ .name = "appendDouble", .params = p, .param_count = 1,
                .return_type = t_void, .is_native = true, .name_token = { .start = "appendDouble", .length = 12 } };
        }
        /* appendBool(value: bool): void */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "value", .length = 5 }, .type = t_bool, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            enc_methods[11] = (StructMethod){ .name = "appendBool", .params = p, .param_count = 1,
                .return_type = t_void, .is_native = true, .name_token = { .start = "appendBool", .length = 10 } };
        }
        /* appendObject(): Encoder */
        {
            enc_methods[12] = (StructMethod){ .name = "appendObject", .params = NULL, .param_count = 0,
                .return_type = encoder_type, .is_native = true, .name_token = { .start = "appendObject", .length = 12 } };
        }
        /* result(): str */
        {
            enc_methods[13] = (StructMethod){ .name = "result", .params = NULL, .param_count = 0,
                .return_type = t_str, .is_native = true, .name_token = { .start = "result", .length = 6 } };
        }

        /* Update encoder_type with methods */
        encoder_type->as.struct_type.methods = enc_methods;
        encoder_type->as.struct_type.method_count = enc_method_count;

        symbol_table_add_type(parser->symbol_table, enc_tok, encoder_type);

        /* Decoder: native struct with vtable-based dispatch */
        Token dec_tok;
        dec_tok.start = arena_strdup(arena, "Decoder");
        dec_tok.length = 7;
        dec_tok.type = TOKEN_IDENTIFIER;
        dec_tok.line = 0;
        dec_tok.filename = arena_strdup(arena, "<built-in>");

        int dec_method_count = 13;
        StructMethod *dec_methods = arena_alloc(arena, sizeof(StructMethod) * dec_method_count);
        memset(dec_methods, 0, sizeof(StructMethod) * dec_method_count);

        Type *decoder_type = ast_create_struct_type(arena, "Decoder", NULL, 0,
                                                     NULL, 0, true, false, true, NULL);

        /* readStr(key: str): str */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[0] = (StructMethod){ .name = "readStr", .params = p, .param_count = 1,
                .return_type = t_str, .is_native = true, .name_token = { .start = "readStr", .length = 7 } };
        }
        /* readInt(key: str): int */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[1] = (StructMethod){ .name = "readInt", .params = p, .param_count = 1,
                .return_type = t_int, .is_native = true, .name_token = { .start = "readInt", .length = 7 } };
        }
        /* readDouble(key: str): double */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[2] = (StructMethod){ .name = "readDouble", .params = p, .param_count = 1,
                .return_type = t_double, .is_native = true, .name_token = { .start = "readDouble", .length = 10 } };
        }
        /* readBool(key: str): bool */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[3] = (StructMethod){ .name = "readBool", .params = p, .param_count = 1,
                .return_type = t_bool, .is_native = true, .name_token = { .start = "readBool", .length = 8 } };
        }
        /* hasKey(key: str): bool */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[4] = (StructMethod){ .name = "hasKey", .params = p, .param_count = 1,
                .return_type = t_bool, .is_native = true, .name_token = { .start = "hasKey", .length = 6 } };
        }
        /* readObject(key: str): Decoder */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[5] = (StructMethod){ .name = "readObject", .params = p, .param_count = 1,
                .return_type = decoder_type, .is_native = true, .name_token = { .start = "readObject", .length = 10 } };
        }
        /* readArray(key: str): Decoder */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "key", .length = 3 }, .type = t_str, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[6] = (StructMethod){ .name = "readArray", .params = p, .param_count = 1,
                .return_type = decoder_type, .is_native = true, .name_token = { .start = "readArray", .length = 9 } };
        }
        /* length(): int */
        {
            dec_methods[7] = (StructMethod){ .name = "length", .params = NULL, .param_count = 0,
                .return_type = t_int, .is_native = true, .name_token = { .start = "length", .length = 6 } };
        }
        /* at(index: int): Decoder */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "index", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[8] = (StructMethod){ .name = "at", .params = p, .param_count = 1,
                .return_type = decoder_type, .is_native = true, .name_token = { .start = "at", .length = 2 } };
        }
        /* atStr(index: int): str */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "index", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[9] = (StructMethod){ .name = "atStr", .params = p, .param_count = 1,
                .return_type = t_str, .is_native = true, .name_token = { .start = "atStr", .length = 5 } };
        }
        /* atInt(index: int): int */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "index", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[10] = (StructMethod){ .name = "atInt", .params = p, .param_count = 1,
                .return_type = t_int, .is_native = true, .name_token = { .start = "atInt", .length = 5 } };
        }
        /* atDouble(index: int): double */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "index", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[11] = (StructMethod){ .name = "atDouble", .params = p, .param_count = 1,
                .return_type = t_double, .is_native = true, .name_token = { .start = "atDouble", .length = 8 } };
        }
        /* atBool(index: int): bool */
        {
            Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
            p[0] = (Parameter){ .name = { .start = "index", .length = 5 }, .type = t_int, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
            dec_methods[12] = (StructMethod){ .name = "atBool", .params = p, .param_count = 1,
                .return_type = t_bool, .is_native = true, .name_token = { .start = "atBool", .length = 6 } };
        }

        /* Update decoder_type with methods */
        decoder_type->as.struct_type.methods = dec_methods;
        decoder_type->as.struct_type.method_count = dec_method_count;

        symbol_table_add_type(parser->symbol_table, dec_tok, decoder_type);
    }

    // ---- Built-in standard interfaces (with method signatures for constraint checking) ----

    Type *iface_int = ast_create_primitive_type(arena, TYPE_INT);
    Type *iface_str = ast_create_primitive_type(arena, TYPE_STRING);
    Type *iface_bool = ast_create_primitive_type(arena, TYPE_BOOL);
    Type *t_self = ast_create_opaque_type(arena, "Self");

    // Comparable — compare(other: Self): int
    {
        Token comparable_tok;
        comparable_tok.start = arena_strdup(arena, "Comparable");
        comparable_tok.length = 10;
        comparable_tok.type = TOKEN_IDENTIFIER;
        comparable_tok.line = 0;
        comparable_tok.filename = arena_strdup(arena, "<built-in>");

        StructMethod *methods = arena_alloc(arena, sizeof(StructMethod) * 1);
        memset(methods, 0, sizeof(StructMethod) * 1);
        Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
        p[0] = (Parameter){ .name = { .start = "other", .length = 5 }, .type = t_self, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
        methods[0] = (StructMethod){ .name = "compare", .params = p, .param_count = 1,
            .return_type = iface_int, .name_token = { .start = "compare", .length = 7 } };

        Type *comparable_type = ast_create_interface_type(arena, "Comparable", methods, 1);
        symbol_table_add_type(parser->symbol_table, comparable_tok, comparable_type);
    }

    // Hashable — hash(): int, equals(other: Self): bool
    {
        Token hashable_tok;
        hashable_tok.start = arena_strdup(arena, "Hashable");
        hashable_tok.length = 8;
        hashable_tok.type = TOKEN_IDENTIFIER;
        hashable_tok.line = 0;
        hashable_tok.filename = arena_strdup(arena, "<built-in>");

        StructMethod *methods = arena_alloc(arena, sizeof(StructMethod) * 2);
        memset(methods, 0, sizeof(StructMethod) * 2);
        methods[0] = (StructMethod){ .name = "hash", .params = NULL, .param_count = 0,
            .return_type = iface_int, .name_token = { .start = "hash", .length = 4 } };
        Parameter *p = arena_alloc(arena, sizeof(Parameter) * 1);
        p[0] = (Parameter){ .name = { .start = "other", .length = 5 }, .type = t_self, .mem_qualifier = MEM_DEFAULT, .sync_modifier = SYNC_NONE };
        methods[1] = (StructMethod){ .name = "equals", .params = p, .param_count = 1,
            .return_type = iface_bool, .name_token = { .start = "equals", .length = 6 } };

        Type *hashable_type = ast_create_interface_type(arena, "Hashable", methods, 2);
        symbol_table_add_type(parser->symbol_table, hashable_tok, hashable_type);
    }

    // Stringable — toString(): str
    {
        Token stringable_tok;
        stringable_tok.start = arena_strdup(arena, "Stringable");
        stringable_tok.length = 10;
        stringable_tok.type = TOKEN_IDENTIFIER;
        stringable_tok.line = 0;
        stringable_tok.filename = arena_strdup(arena, "<built-in>");

        StructMethod *methods = arena_alloc(arena, sizeof(StructMethod) * 1);
        memset(methods, 0, sizeof(StructMethod) * 1);
        methods[0] = (StructMethod){ .name = "toString", .params = NULL, .param_count = 0,
            .return_type = iface_str, .name_token = { .start = "toString", .length = 8 } };

        Type *stringable_type = ast_create_interface_type(arena, "Stringable", methods, 1);
        symbol_table_add_type(parser->symbol_table, stringable_tok, stringable_type);
    }

    // Copyable — copy(): Self
    {
        Token copyable_tok;
        copyable_tok.start = arena_strdup(arena, "Copyable");
        copyable_tok.length = 8;
        copyable_tok.type = TOKEN_IDENTIFIER;
        copyable_tok.line = 0;
        copyable_tok.filename = arena_strdup(arena, "<built-in>");

        StructMethod *methods = arena_alloc(arena, sizeof(StructMethod) * 1);
        memset(methods, 0, sizeof(StructMethod) * 1);
        methods[0] = (StructMethod){ .name = "copy", .params = NULL, .param_count = 0,
            .return_type = t_self, .name_token = { .start = "copy", .length = 4 } };

        Type *copyable_type = ast_create_interface_type(arena, "Copyable", methods, 1);
        symbol_table_add_type(parser->symbol_table, copyable_tok, copyable_type);
    }

    // Note: Other array operations (push, pop, rev, rem, ins) are now method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->import_ctx = NULL;
}

void parser_cleanup(Parser *parser)
{
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        return NULL;
    }

    return module;
}
