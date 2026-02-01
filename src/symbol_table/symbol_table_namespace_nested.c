// symbol_table_namespace_nested.c
// Nested namespace operations

void symbol_table_add_nested_namespace(SymbolTable *table, Token parent_ns_name, Token nested_ns_name)
{
    char parent_str[256], nested_str[256];
    int parent_len = parent_ns_name.length < 255 ? parent_ns_name.length : 255;
    int nested_len = nested_ns_name.length < 255 ? nested_ns_name.length : 255;
    strncpy(parent_str, parent_ns_name.start, parent_len);
    parent_str[parent_len] = '\0';
    strncpy(nested_str, nested_ns_name.start, nested_len);
    nested_str[nested_len] = '\0';
    DEBUG_VERBOSE("Adding nested namespace '%s' to parent '%s'", nested_str, parent_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add nested namespace: NULL table or global scope");
        return;
    }

    /* Find the parent namespace symbol in global scope */
    Symbol *parent_ns = table->global_scope->symbols;
    while (parent_ns != NULL)
    {
        if (parent_ns->is_namespace &&
            parent_ns->name.length == parent_ns_name.length &&
            memcmp(parent_ns->name.start, parent_ns_name.start, parent_ns_name.length) == 0)
        {
            break;
        }
        parent_ns = parent_ns->next;
    }

    if (parent_ns == NULL)
    {
        DEBUG_ERROR("Parent namespace '%s' not found", parent_str);
        return;
    }

    /* Check if nested namespace already exists in parent */
    Symbol *existing = parent_ns->namespace_symbols;
    while (existing != NULL)
    {
        if (existing->name.length == nested_ns_name.length &&
            memcmp(existing->name.start, nested_ns_name.start, nested_ns_name.length) == 0)
        {
            DEBUG_VERBOSE("Nested namespace '%s' already exists in '%s'", nested_str, parent_str);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the nested namespace symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating nested namespace symbol");
        return;
    }

    /* Duplicate the namespace name */
    char *dup_name = arena_strndup(table->arena, nested_ns_name.start, nested_ns_name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating nested namespace name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = nested_ns_name.length;
    symbol->name.line = nested_ns_name.line;
    symbol->name.type = nested_ns_name.type;
    symbol->name.filename = nested_ns_name.filename;
    symbol->type = NULL;
    symbol->kind = SYMBOL_NAMESPACE;
    symbol->offset = 0;
    symbol->arena_depth = 0;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->declared_func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    symbol->is_native = false;
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->is_namespace = true;
    symbol->also_imported_directly = false;  /* May be set to true later if module also has direct import */
    symbol->namespace_name = dup_name;
    symbol->canonical_namespace_prefix = NULL;  /* NULL means this IS the canonical namespace */
    symbol->canonical_module_name = NULL;  /* Set during type checking from module path */
    symbol->imported_stmts = NULL;  /* Set during type checking to detect duplicate imports */
    symbol->namespace_symbols = NULL;

    /* Add to parent namespace's symbol list */
    symbol->next = parent_ns->namespace_symbols;
    parent_ns->namespace_symbols = symbol;

    DEBUG_VERBOSE("Nested namespace '%s' added to parent '%s'", nested_str, parent_str);
}

void symbol_table_add_function_to_nested_namespace(SymbolTable *table, Token parent_ns_name, Token nested_ns_name, Token symbol_name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod)
{
    char parent_str[256], nested_str[256], sym_str[256];
    int parent_len = parent_ns_name.length < 255 ? parent_ns_name.length : 255;
    int nested_len = nested_ns_name.length < 255 ? nested_ns_name.length : 255;
    int sym_len = symbol_name.length < 255 ? symbol_name.length : 255;
    strncpy(parent_str, parent_ns_name.start, parent_len);
    parent_str[parent_len] = '\0';
    strncpy(nested_str, nested_ns_name.start, nested_len);
    nested_str[nested_len] = '\0';
    strncpy(sym_str, symbol_name.start, sym_len);
    sym_str[sym_len] = '\0';
    DEBUG_VERBOSE("Adding function '%s' to nested namespace '%s.%s'", sym_str, parent_str, nested_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add function to nested namespace: NULL table or global scope");
        return;
    }

    /* Find the parent namespace */
    Symbol *parent_ns = table->global_scope->symbols;
    while (parent_ns != NULL)
    {
        if (parent_ns->is_namespace &&
            parent_ns->name.length == parent_ns_name.length &&
            memcmp(parent_ns->name.start, parent_ns_name.start, parent_ns_name.length) == 0)
        {
            break;
        }
        parent_ns = parent_ns->next;
    }

    if (parent_ns == NULL)
    {
        DEBUG_ERROR("Parent namespace '%s' not found", parent_str);
        return;
    }

    /* Find the nested namespace within the parent */
    Symbol *nested_ns = parent_ns->namespace_symbols;
    while (nested_ns != NULL)
    {
        if (nested_ns->is_namespace &&
            nested_ns->name.length == nested_ns_name.length &&
            memcmp(nested_ns->name.start, nested_ns_name.start, nested_ns_name.length) == 0)
        {
            break;
        }
        nested_ns = nested_ns->next;
    }

    if (nested_ns == NULL)
    {
        DEBUG_ERROR("Nested namespace '%s' not found in parent '%s'", nested_str, parent_str);
        return;
    }

    /* Check if symbol already exists in nested namespace */
    Symbol *existing = nested_ns->namespace_symbols;
    while (existing != NULL)
    {
        if (existing->name.length == symbol_name.length &&
            memcmp(existing->name.start, symbol_name.start, symbol_name.length) == 0)
        {
            DEBUG_VERBOSE("Function '%s' already exists in '%s.%s', updating", sym_str, parent_str, nested_str);
            existing->type = ast_clone_type(table->arena, type);
            existing->func_mod = func_mod;
            existing->declared_func_mod = declared_func_mod;
            existing->is_function = true;
            existing->is_native = (type != NULL && type->kind == TYPE_FUNCTION && type->as.function.is_native);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the new symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating function symbol for nested namespace");
        return;
    }

    char *dup_name = arena_strndup(table->arena, symbol_name.start, symbol_name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating function name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = symbol_name.length;
    symbol->name.line = symbol_name.line;
    symbol->name.type = symbol_name.type;
    symbol->name.filename = symbol_name.filename;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = SYMBOL_GLOBAL;
    symbol->offset = 0;
    symbol->arena_depth = 0;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = func_mod;
    symbol->declared_func_mod = declared_func_mod;
    symbol->is_function = true;
    symbol->is_native = (type != NULL && type->kind == TYPE_FUNCTION && type->as.function.is_native);
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;

    /* Add to nested namespace's symbol list */
    symbol->next = nested_ns->namespace_symbols;
    nested_ns->namespace_symbols = symbol;

    DEBUG_VERBOSE("Function '%s' added to nested namespace '%s.%s'", sym_str, parent_str, nested_str);
}

void symbol_table_add_symbol_to_nested_namespace(SymbolTable *table, Token parent_ns_name, Token nested_ns_name, Token symbol_name, Type *type, bool is_static)
{
    char parent_str[256], nested_str[256], sym_str[256];
    int parent_len = parent_ns_name.length < 255 ? parent_ns_name.length : 255;
    int nested_len = nested_ns_name.length < 255 ? nested_ns_name.length : 255;
    int sym_len = symbol_name.length < 255 ? symbol_name.length : 255;
    strncpy(parent_str, parent_ns_name.start, parent_len);
    parent_str[parent_len] = '\0';
    strncpy(nested_str, nested_ns_name.start, nested_len);
    nested_str[nested_len] = '\0';
    strncpy(sym_str, symbol_name.start, sym_len);
    sym_str[sym_len] = '\0';
    DEBUG_VERBOSE("Adding symbol '%s' to nested namespace '%s.%s' (is_static=%d)", sym_str, parent_str, nested_str, is_static);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add symbol to nested namespace: NULL table or global scope");
        return;
    }

    /* Find the parent namespace */
    Symbol *parent_ns = table->global_scope->symbols;
    while (parent_ns != NULL)
    {
        if (parent_ns->is_namespace &&
            parent_ns->name.length == parent_ns_name.length &&
            memcmp(parent_ns->name.start, parent_ns_name.start, parent_ns_name.length) == 0)
        {
            break;
        }
        parent_ns = parent_ns->next;
    }

    if (parent_ns == NULL)
    {
        DEBUG_ERROR("Parent namespace '%s' not found", parent_str);
        return;
    }

    /* Find the nested namespace within the parent */
    Symbol *nested_ns = parent_ns->namespace_symbols;
    while (nested_ns != NULL)
    {
        if (nested_ns->is_namespace &&
            nested_ns->name.length == nested_ns_name.length &&
            memcmp(nested_ns->name.start, nested_ns_name.start, nested_ns_name.length) == 0)
        {
            break;
        }
        nested_ns = nested_ns->next;
    }

    if (nested_ns == NULL)
    {
        DEBUG_ERROR("Nested namespace '%s' not found in parent '%s'", nested_str, parent_str);
        return;
    }

    /* Check if symbol already exists in nested namespace */
    Symbol *existing = nested_ns->namespace_symbols;
    while (existing != NULL)
    {
        if (existing->name.length == symbol_name.length &&
            memcmp(existing->name.start, symbol_name.start, symbol_name.length) == 0)
        {
            DEBUG_VERBOSE("Symbol '%s' already exists in '%s.%s', updating", sym_str, parent_str, nested_str);
            existing->type = ast_clone_type(table->arena, type);
            existing->is_static = is_static;
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the new symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating symbol for nested namespace");
        return;
    }

    char *dup_name = arena_strndup(table->arena, symbol_name.start, symbol_name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating symbol name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = symbol_name.length;
    symbol->name.line = symbol_name.line;
    symbol->name.type = symbol_name.type;
    symbol->name.filename = symbol_name.filename;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = SYMBOL_GLOBAL;
    symbol->offset = 0;
    symbol->arena_depth = 0;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->declared_func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    symbol->is_native = false;
    symbol->is_static = is_static;
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;

    /* Add to nested namespace's symbol list */
    symbol->next = nested_ns->namespace_symbols;
    nested_ns->namespace_symbols = symbol;

    DEBUG_VERBOSE("Symbol '%s' added to nested namespace '%s.%s'", sym_str, parent_str, nested_str);
}

Symbol *symbol_table_lookup_nested_namespace(SymbolTable *table, Token parent_ns_name, Token nested_ns_name)
{
    char parent_str[256], nested_str[256];
    int parent_len = parent_ns_name.length < 255 ? parent_ns_name.length : 255;
    int nested_len = nested_ns_name.length < 255 ? nested_ns_name.length : 255;
    strncpy(parent_str, parent_ns_name.start, parent_len);
    parent_str[parent_len] = '\0';
    strncpy(nested_str, nested_ns_name.start, nested_len);
    nested_str[nested_len] = '\0';
    DEBUG_VERBOSE("Looking up nested namespace '%s' in parent '%s'", nested_str, parent_str);

    if (table == NULL || table->global_scope == NULL)
    {
        return NULL;
    }

    /* Find the parent namespace */
    Symbol *parent_ns = table->global_scope->symbols;
    while (parent_ns != NULL)
    {
        if (parent_ns->is_namespace &&
            parent_ns->name.length == parent_ns_name.length &&
            memcmp(parent_ns->name.start, parent_ns_name.start, parent_ns_name.length) == 0)
        {
            break;
        }
        parent_ns = parent_ns->next;
    }

    if (parent_ns == NULL)
    {
        return NULL;
    }

    /* Find the nested namespace within the parent */
    Symbol *nested_ns = parent_ns->namespace_symbols;
    while (nested_ns != NULL)
    {
        if (nested_ns->is_namespace &&
            nested_ns->name.length == nested_ns_name.length &&
            memcmp(nested_ns->name.start, nested_ns_name.start, nested_ns_name.length) == 0)
        {
            return nested_ns;
        }
        nested_ns = nested_ns->next;
    }

    return NULL;
}

