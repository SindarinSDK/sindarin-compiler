#include "type_checker/type_checker_util.h"
#include "diagnostic.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int had_type_error = 0;
static int native_context_depth = 0;

void type_checker_reset_error(void)
{
    had_type_error = 0;
}

int type_checker_had_error(void)
{
    return had_type_error;
}

void type_checker_set_error(void)
{
    had_type_error = 1;
}

const char *type_name(Type *type)
{
    if (type == NULL) return "unknown";
    switch (type->kind) {
        case TYPE_INT:         return "int";
        case TYPE_INT32:       return "int32";
        case TYPE_UINT:        return "uint";
        case TYPE_UINT32:      return "uint32";
        case TYPE_LONG:        return "long";
        case TYPE_DOUBLE:      return "double";
        case TYPE_FLOAT:       return "float";
        case TYPE_CHAR:        return "char";
        case TYPE_STRING:      return "str";
        case TYPE_BOOL:        return "bool";
        case TYPE_BYTE:        return "byte";
        case TYPE_VOID:        return "void";
        case TYPE_NIL:         return "nil";
        case TYPE_ANY:         return "any";
        case TYPE_ARRAY:       return "array";
        case TYPE_FUNCTION:    return "function";
        case TYPE_POINTER:      return "pointer";
        default:                return "unknown";
    }
}

void type_error(Token *token, const char *msg)
{
    diagnostic_error_at(token, "%s", msg);
    had_type_error = 1;
}

void type_error_with_suggestion(Token *token, const char *msg, const char *suggestion)
{
    diagnostic_error_with_suggestion(token, suggestion, "%s", msg);
    had_type_error = 1;
}

void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context)
{
    diagnostic_error_at(token, "type mismatch in %s: expected '%s', got '%s'",
                        context, type_name(expected), type_name(actual));
    had_type_error = 1;
}

bool is_numeric_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_INT32 ||
                           type->kind == TYPE_UINT || type->kind == TYPE_UINT32 ||
                           type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE ||
                           type->kind == TYPE_FLOAT || type->kind == TYPE_BYTE ||
                           type->kind == TYPE_CHAR);
    DEBUG_VERBOSE("Checking if type is numeric: %s", result ? "true" : "false");
    return result;
}

bool is_comparison_operator(SnTokenType op)
{
    bool result = op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS ||
                  op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
    DEBUG_VERBOSE("Checking if operator is comparison: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_arithmetic_operator(SnTokenType op)
{
    bool result = op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
    DEBUG_VERBOSE("Checking if operator is arithmetic: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_printable_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_INT32 ||
                           type->kind == TYPE_UINT || type->kind == TYPE_UINT32 ||
                           type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE ||
                           type->kind == TYPE_FLOAT || type->kind == TYPE_CHAR ||
                           type->kind == TYPE_STRING || type->kind == TYPE_BOOL ||
                           type->kind == TYPE_BYTE || type->kind == TYPE_ARRAY ||
                           type->kind == TYPE_ANY || type->kind == TYPE_STRUCT);
    DEBUG_VERBOSE("Checking if type is printable: %s", result ? "true" : "false");
    return result;
}

/* Check if a type can be passed as a variadic argument.
 * Per spec: primitives, str, and pointer types are allowed.
 * Arrays cannot be passed as variadic arguments. */
bool is_variadic_compatible_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_INT ||
                  type->kind == TYPE_INT32 ||
                  type->kind == TYPE_UINT ||
                  type->kind == TYPE_UINT32 ||
                  type->kind == TYPE_LONG ||
                  type->kind == TYPE_DOUBLE ||
                  type->kind == TYPE_FLOAT ||
                  type->kind == TYPE_CHAR ||
                  type->kind == TYPE_BOOL ||
                  type->kind == TYPE_BYTE ||
                  type->kind == TYPE_STRING ||
                  type->kind == TYPE_POINTER;
    DEBUG_VERBOSE("Checking if type is variadic-compatible: %s", result ? "true" : "false");
    return result;
}

bool is_primitive_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_INT ||
                  type->kind == TYPE_INT32 ||
                  type->kind == TYPE_UINT ||
                  type->kind == TYPE_UINT32 ||
                  type->kind == TYPE_LONG ||
                  type->kind == TYPE_DOUBLE ||
                  type->kind == TYPE_FLOAT ||
                  type->kind == TYPE_CHAR ||
                  type->kind == TYPE_BOOL ||
                  type->kind == TYPE_BYTE ||
                  type->kind == TYPE_VOID;
    DEBUG_VERBOSE("Checking if type is primitive: %s", result ? "true" : "false");
    return result;
}

bool is_reference_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_STRING ||
                  type->kind == TYPE_ARRAY ||
                  type->kind == TYPE_FUNCTION;
    DEBUG_VERBOSE("Checking if type is reference: %s", result ? "true" : "false");
    return result;
}

/* Forward declaration for recursive struct field checking */
static bool struct_has_only_primitives(Type *struct_type);

/* Check if a type can escape from a private block/function.
 * Primitives can always escape.
 * Structs can escape only if they contain only primitive fields (recursively checked).
 * Arrays, strings, pointers, and other heap types cannot escape. */
bool can_escape_private(Type *type)
{
    if (type == NULL) return false;

    /* Primitive types can always escape */
    if (is_primitive_type(type))
    {
        return true;
    }

    /* Struct types can escape only if they contain only primitives */
    if (type->kind == TYPE_STRUCT)
    {
        return struct_has_only_primitives(type);
    }

    /* All other types (arrays, strings, pointers, etc.) cannot escape */
    return false;
}

/* Helper function to check if a struct contains only primitive fields.
 * Recursively checks nested struct types.
 * Returns true if all fields are primitives or primitive-only structs.
 * Returns false if any field is a heap type (str, array, pointer, etc.). */
static bool struct_has_only_primitives(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return false;
    }

    /* Native structs may contain pointers and cannot escape private blocks */
    if (struct_type->as.struct_type.is_native)
    {
        DEBUG_VERBOSE("Struct '%s' is native (contains pointers) - cannot escape private",
                      struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
        return false;
    }

    /* Check each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;

        if (field_type == NULL)
        {
            return false;
        }

        /* Primitive fields are OK */
        if (is_primitive_type(field_type))
        {
            continue;
        }

        /* Nested struct fields - recursively check */
        if (field_type->kind == TYPE_STRUCT)
        {
            if (!struct_has_only_primitives(field_type))
            {
                DEBUG_VERBOSE("Struct field '%s' contains non-primitive nested struct - cannot escape private",
                              field->name ? field->name : "unknown");
                return false;
            }
            continue;
        }

        /* String fields - heap allocated, cannot escape */
        if (field_type->kind == TYPE_STRING)
        {
            DEBUG_VERBOSE("Struct field '%s' is str type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Array fields - heap allocated (dynamic) or contain heap data */
        if (field_type->kind == TYPE_ARRAY)
        {
            DEBUG_VERBOSE("Struct field '%s' is array type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Pointer fields - point to heap/external memory */
        if (field_type->kind == TYPE_POINTER)
        {
            DEBUG_VERBOSE("Struct field '%s' is pointer type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Function fields (closures) - contain heap references */
        if (field_type->kind == TYPE_FUNCTION)
        {
            DEBUG_VERBOSE("Struct field '%s' is function type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Opaque types - external C pointers */
        if (field_type->kind == TYPE_OPAQUE)
        {
            DEBUG_VERBOSE("Struct field '%s' is opaque type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Any type - may contain heap references */
        if (field_type->kind == TYPE_ANY)
        {
            DEBUG_VERBOSE("Struct field '%s' is any type - cannot escape private",
                          field->name ? field->name : "unknown");
            return false;
        }

        /* Unknown type kind - conservative default: cannot escape */
        DEBUG_VERBOSE("Struct field '%s' has unknown type kind %d - cannot escape private",
                      field->name ? field->name : "unknown", field_type->kind);
        return false;
    }

    /* All fields are primitives or primitive-only structs */
    DEBUG_VERBOSE("Struct '%s' contains only primitives - can escape private",
                  struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
    return true;
}

/* Helper to find and describe the first non-primitive field in a struct.
 * Returns a static buffer with the description, or NULL if all fields are primitives. */
static const char *find_blocking_struct_field(Type *struct_type)
{
    static char reason_buffer[256];

    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return NULL;
    }

    /* Native structs may contain pointers */
    if (struct_type->as.struct_type.is_native)
    {
        snprintf(reason_buffer, sizeof(reason_buffer),
                 "struct '%s' is a native struct (may contain pointers)",
                 struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "anonymous");
        return reason_buffer;
    }

    /* Check each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;
        const char *field_name = field->name ? field->name : "unknown";

        if (field_type == NULL) continue;

        if (is_primitive_type(field_type)) continue;

        /* Nested struct - recursively check */
        if (field_type->kind == TYPE_STRUCT)
        {
            const char *nested_reason = find_blocking_struct_field(field_type);
            if (nested_reason != NULL)
            {
                /* Copy nested_reason first since it points to the same static buffer
                 * we're about to write to */
                char nested_copy[256];
                strncpy(nested_copy, nested_reason, sizeof(nested_copy) - 1);
                nested_copy[sizeof(nested_copy) - 1] = '\0';

                snprintf(reason_buffer, sizeof(reason_buffer),
                         "field '%s' contains %s", field_name, nested_copy);
                return reason_buffer;
            }
            continue;
        }

        /* Describe the blocking field type */
        const char *type_desc = NULL;
        switch (field_type->kind)
        {
            case TYPE_STRING:
                type_desc = "str (heap-allocated string)";
                break;
            case TYPE_ARRAY:
                type_desc = "array (heap-allocated)";
                break;
            case TYPE_POINTER:
                type_desc = "pointer";
                break;
            case TYPE_FUNCTION:
                type_desc = "function (closure)";
                break;
            case TYPE_OPAQUE:
                type_desc = "opaque type";
                break;
            case TYPE_ANY:
                type_desc = "any type";
                break;
            default:
                type_desc = "non-primitive type";
                break;
        }

        snprintf(reason_buffer, sizeof(reason_buffer),
                 "field '%s' is %s", field_name, type_desc);
        return reason_buffer;
    }

    return NULL;
}

const char *get_private_escape_block_reason(Type *type)
{
    static char reason_buffer[512];

    if (type == NULL) return "unknown type";

    /* Primitive types can escape */
    if (is_primitive_type(type))
    {
        return NULL;
    }

    /* Struct types - check for heap fields */
    if (type->kind == TYPE_STRUCT)
    {
        const char *field_reason = find_blocking_struct_field(type);
        if (field_reason != NULL)
        {
            snprintf(reason_buffer, sizeof(reason_buffer),
                     "struct '%s' contains heap data: %s",
                     type->as.struct_type.name ? type->as.struct_type.name : "anonymous",
                     field_reason);
            return reason_buffer;
        }
        return NULL; /* Struct with only primitives can escape */
    }

    /* Non-struct, non-primitive types */
    switch (type->kind)
    {
        case TYPE_STRING:
            return "str type contains heap-allocated string data";
        case TYPE_ARRAY:
            return "array type is heap-allocated";
        case TYPE_POINTER:
            return "pointer type references external memory";
        case TYPE_FUNCTION:
            return "function type (closure) contains heap references";
        case TYPE_OPAQUE:
            return "opaque type references external C memory";
        case TYPE_ANY:
            return "any type may contain heap references";
        default:
            return "type cannot escape private block";
    }
}

void memory_context_init(MemoryContext *ctx)
{
    ctx->in_private_block = false;
    ctx->in_private_function = false;
    ctx->private_depth = 0;
    ctx->scope_depth = 0;
}

void memory_context_enter_private(MemoryContext *ctx)
{
    ctx->in_private_block = true;
    ctx->private_depth++;
}

void memory_context_exit_private(MemoryContext *ctx)
{
    ctx->private_depth--;
    if (ctx->private_depth <= 0)
    {
        ctx->in_private_block = false;
        ctx->private_depth = 0;
    }
}

bool memory_context_is_private(MemoryContext *ctx)
{
    return ctx->in_private_block || ctx->in_private_function;
}

void memory_context_enter_scope(MemoryContext *ctx)
{
    if (ctx != NULL)
    {
        ctx->scope_depth++;
        DEBUG_VERBOSE("Entering scope in memory context (depth: %d)", ctx->scope_depth);
    }
}

void memory_context_exit_scope(MemoryContext *ctx)
{
    if (ctx != NULL && ctx->scope_depth > 0)
    {
        ctx->scope_depth--;
        DEBUG_VERBOSE("Exiting scope in memory context (depth: %d)", ctx->scope_depth);
    }
}

int memory_context_get_scope_depth(MemoryContext *ctx)
{
    if (ctx == NULL)
    {
        return 0;
    }
    return ctx->scope_depth;
}

bool can_promote_numeric(Type *from, Type *to)
{
    if (from == NULL || to == NULL) return false;
    /* int can promote to double or long */
    if (from->kind == TYPE_INT && (to->kind == TYPE_DOUBLE || to->kind == TYPE_LONG))
        return true;
    /* long can promote to double */
    if (from->kind == TYPE_LONG && to->kind == TYPE_DOUBLE)
        return true;
    /* float can promote to double */
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_DOUBLE)
        return true;
    /* int32 can promote to float or double */
    if (from->kind == TYPE_INT32 && (to->kind == TYPE_FLOAT || to->kind == TYPE_DOUBLE))
        return true;
    /* uint32 can promote to float or double */
    if (from->kind == TYPE_UINT32 && (to->kind == TYPE_FLOAT || to->kind == TYPE_DOUBLE))
        return true;
    /* uint can promote to double or long */
    if (from->kind == TYPE_UINT && (to->kind == TYPE_DOUBLE || to->kind == TYPE_LONG))
        return true;
    return false;
}

Type *get_promoted_type(Arena *arena, Type *left, Type *right)
{
    if (left == NULL || right == NULL) return NULL;

    /* Check for numeric type promotion FIRST (before ast_type_equals check) */
    /* because ast_type_equals treats compatible types as equal */
    if (is_numeric_type(left) && is_numeric_type(right))
    {
        /* double is the widest numeric type */
        if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE)
            return ast_create_primitive_type(arena, TYPE_DOUBLE);
        /* float is wider than integer types */
        if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT)
            return ast_create_primitive_type(arena, TYPE_FLOAT);
        /* long is wider than int/uint */
        if (left->kind == TYPE_LONG || right->kind == TYPE_LONG)
            return ast_create_primitive_type(arena, TYPE_LONG);
        /* If both are the exact same type, return it */
        if (left->kind == right->kind)
            return left;
        /* int32 and uint32 stay as their own types (fixed-width) */
        if ((left->kind == TYPE_INT32 && right->kind == TYPE_INT32) ||
            (left->kind == TYPE_UINT32 && right->kind == TYPE_UINT32))
            return left;
        /* int and uint stay as their own types */
        if ((left->kind == TYPE_INT && right->kind == TYPE_INT) ||
            (left->kind == TYPE_UINT && right->kind == TYPE_UINT))
            return left;
        /* int promotes to int32/uint32 when mixed */
        if (left->kind == TYPE_INT32 && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_INT32)
            return right;
        if (left->kind == TYPE_UINT32 && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_UINT32)
            return right;
        /* int promotes to uint when mixed */
        if (left->kind == TYPE_UINT && right->kind == TYPE_INT)
            return left;
        if (left->kind == TYPE_INT && right->kind == TYPE_UINT)
            return right;
        /* byte and char promote to int in arithmetic operations */
        if (left->kind == TYPE_BYTE || left->kind == TYPE_CHAR ||
            right->kind == TYPE_BYTE || right->kind == TYPE_CHAR)
            return ast_create_primitive_type(arena, TYPE_INT);
        /* Mixed int/uint - no automatic promotion for other cases */
        return NULL;
    }

    /* If both are the same type, return it */
    if (ast_type_equals(left, right))
        return left;

    /* No valid promotion */
    return NULL;
}

/* ============================================================================
 * String Similarity Helpers
 * ============================================================================ */

/* Minimum of three integers */
static int min3(int a, int b, int c)
{
    int min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

/*
 * Compute Levenshtein distance between two strings.
 * Uses O(n) space by only keeping two rows of the DP table.
 */
int levenshtein_distance(const char *s1, int len1, const char *s2, int len2)
{
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    /* Use stack allocation for small strings, heap for larger */
    int *prev_row;
    int *curr_row;
    int stack_prev[64];
    int stack_curr[64];
    int *heap_prev = NULL;
    int *heap_curr = NULL;

    if (len2 + 1 <= 64)
    {
        prev_row = stack_prev;
        curr_row = stack_curr;
    }
    else
    {
        heap_prev = malloc((len2 + 1) * sizeof(int));
        heap_curr = malloc((len2 + 1) * sizeof(int));
        if (!heap_prev || !heap_curr)
        {
            free(heap_prev);
            free(heap_curr);
            return len1 > len2 ? len1 : len2; /* Fallback: max length */
        }
        prev_row = heap_prev;
        curr_row = heap_curr;
    }

    /* Initialize first row */
    for (int j = 0; j <= len2; j++)
    {
        prev_row[j] = j;
    }

    /* Fill in the DP table */
    for (int i = 1; i <= len1; i++)
    {
        curr_row[0] = i;
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            curr_row[j] = min3(
                prev_row[j] + 1,      /* deletion */
                curr_row[j - 1] + 1,  /* insertion */
                prev_row[j - 1] + cost /* substitution */
            );
        }
        /* Swap rows */
        int *temp = prev_row;
        prev_row = curr_row;
        curr_row = temp;
    }

    int result = prev_row[len2];
    free(heap_prev);
    free(heap_curr);
    return result;
}

/*
 * Find a similar symbol name in the symbol table.
 * Returns NULL if no good match found (distance > 2 or no symbols).
 * Uses a static buffer to return the result.
 */
const char *find_similar_symbol(SymbolTable *table, const char *name, int name_len)
{
    static char best_match[128];
    int best_distance = 3; /* Only suggest if distance <= 2 */
    Scope *scope = table->current;

    best_match[0] = '\0';

    while (scope != NULL)
    {
        Symbol *sym = scope->symbols;
        while (sym != NULL)
        {
            int sym_len = sym->name.length;
            /* Skip if lengths are too different */
            if (abs(sym_len - name_len) <= 2)
            {
                int dist = levenshtein_distance(name, name_len, sym->name.start, sym_len);
                if (dist < best_distance && dist > 0) /* dist > 0 to avoid exact matches */
                {
                    best_distance = dist;
                    int copy_len = sym_len < 127 ? sym_len : 127;
                    memcpy(best_match, sym->name.start, copy_len);
                    best_match[copy_len] = '\0';
                }
            }
            sym = sym->next;
        }
        scope = scope->enclosing;
    }

    return best_match[0] != '\0' ? best_match : NULL;
}

/* Known array methods for suggestions */
static const char *array_methods[] = {
    "push", "pop", "clear", "concat", "indexOf", "contains",
    "clone", "join", "reverse", "insert", "remove", "length",
    NULL
};

/* Known string methods for suggestions */
static const char *string_methods[] = {
    "substring", "indexOf", "split", "trim", "toUpper", "toLower",
    "startsWith", "endsWith", "contains", "replace", "charAt", "length",
    "append",
    NULL
};

/*
 * Find a similar method name for a given type.
 * Returns NULL if no good match found.
 */
const char *find_similar_method(Type *type, const char *method_name)
{
    const char **methods = NULL;

    if (type == NULL) return NULL;

    if (type->kind == TYPE_ARRAY)
    {
        methods = array_methods;
    }
    else if (type->kind == TYPE_STRING)
    {
        methods = string_methods;
    }
    else
    {
        return NULL;
    }

    int name_len = (int)strlen(method_name);
    int best_distance = 3; /* Only suggest if distance <= 2 */
    const char *best_match = NULL;

    for (int i = 0; methods[i] != NULL; i++)
    {
        int method_len = (int)strlen(methods[i]);
        if (abs(method_len - name_len) <= 2)
        {
            int dist = levenshtein_distance(method_name, name_len, methods[i], method_len);
            if (dist < best_distance && dist > 0)
            {
                best_distance = dist;
                best_match = methods[i];
            }
        }
    }

    return best_match;
}

/* ============================================================================
 * Enhanced Error Reporting Functions
 * ============================================================================ */

void undefined_variable_error(Token *token, SymbolTable *table)
{
    char msg_buffer[256];
    const char *var_name_start = token->start;
    int var_name_len = token->length;

    /* Create the variable name as a null-terminated string for the message */
    char var_name[128];
    int copy_len = var_name_len < 127 ? var_name_len : 127;
    memcpy(var_name, var_name_start, copy_len);
    var_name[copy_len] = '\0';

    snprintf(msg_buffer, sizeof(msg_buffer), "Undefined variable '%s'", var_name);

    const char *suggestion = find_similar_symbol(table, var_name_start, var_name_len);
    type_error_with_suggestion(token, msg_buffer, suggestion);
}

void undefined_variable_error_for_assign(Token *token, SymbolTable *table)
{
    char msg_buffer[256];
    const char *var_name_start = token->start;
    int var_name_len = token->length;

    char var_name[128];
    int copy_len = var_name_len < 127 ? var_name_len : 127;
    memcpy(var_name, var_name_start, copy_len);
    var_name[copy_len] = '\0';

    snprintf(msg_buffer, sizeof(msg_buffer), "Cannot assign to undefined variable '%s'", var_name);

    const char *suggestion = find_similar_symbol(table, var_name_start, var_name_len);
    type_error_with_suggestion(token, msg_buffer, suggestion);
}

void invalid_member_error(Token *token, Type *object_type, const char *member_name)
{
    char msg_buffer[256];
    snprintf(msg_buffer, sizeof(msg_buffer), "Type '%s' has no member '%s'",
             type_name(object_type), member_name);

    const char *suggestion = find_similar_method(object_type, member_name);
    type_error_with_suggestion(token, msg_buffer, suggestion);
}

void argument_count_error(Token *token, const char *func_name, int expected, int actual)
{
    diagnostic_error_at(token, "function '%s' expects %d argument(s), got %d",
                        func_name, expected, actual);
    had_type_error = 1;
}

void argument_type_error(Token *token, const char *func_name, int arg_index, Type *expected, Type *actual)
{
    diagnostic_error_at(token, "argument %d of '%s': expected '%s', got '%s'",
                        arg_index + 1, func_name, type_name(expected), type_name(actual));
    had_type_error = 1;
}

/* ============================================================================
 * Native Function Context Tracking
 * ============================================================================ */

void native_context_enter(void)
{
    native_context_depth++;
    DEBUG_VERBOSE("Entering native function context (depth: %d)", native_context_depth);
}

void native_context_exit(void)
{
    if (native_context_depth > 0)
    {
        native_context_depth--;
        DEBUG_VERBOSE("Exiting native function context (depth: %d)", native_context_depth);
    }
}

bool native_context_is_active(void)
{
    return native_context_depth > 0;
}

/* ============================================================================
 * Method Context Tracking (for struct methods with pointer 'self')
 * ============================================================================ */

static int method_context_depth = 0;

void method_context_enter(void)
{
    method_context_depth++;
    DEBUG_VERBOSE("Entering method context (depth: %d)", method_context_depth);
}

void method_context_exit(void)
{
    if (method_context_depth > 0)
    {
        method_context_depth--;
        DEBUG_VERBOSE("Exiting method context (depth: %d)", method_context_depth);
    }
}

bool method_context_is_active(void)
{
    return method_context_depth > 0;
}

/* ============================================================================
 * 'as val' Operand Context Tracking
 * ============================================================================ */

static int as_val_context_depth = 0;

void as_val_context_enter(void)
{
    as_val_context_depth++;
    DEBUG_VERBOSE("Entering 'as val' context (depth: %d)", as_val_context_depth);
}

void as_val_context_exit(void)
{
    if (as_val_context_depth > 0)
    {
        as_val_context_depth--;
        DEBUG_VERBOSE("Exiting 'as val' context (depth: %d)", as_val_context_depth);
    }
}

bool as_val_context_is_active(void)
{
    return as_val_context_depth > 0;
}

/* ============================================================================
 * C-Compatible Type Checking
 * ============================================================================ */

bool is_c_compatible_type(Type *type)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
        /* Primitive types - all C compatible */
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
        case TYPE_CHAR:
        case TYPE_BYTE:
        case TYPE_BOOL:
        case TYPE_VOID:
        /* Interop types - explicitly C compatible */
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_UINT:
            return true;

        /* Pointer types - C compatible */
        case TYPE_POINTER:
            return true;

        /* Opaque types - represent C types like FILE* */
        case TYPE_OPAQUE:
            return true;

        /* Native function types (callback types) - C compatible function pointers */
        case TYPE_FUNCTION:
            return type->as.function.is_native;

        /* Non-C-compatible Sindarin types */
        case TYPE_STRING:  /* Sindarin strings are managed */
        case TYPE_ARRAY:   /* Sindarin arrays have metadata (length, etc.) */
        case TYPE_NIL:
        case TYPE_ANY:
        default:
            return false;
    }
}

/* ============================================================================
 * Struct Field Type Validation
 * ============================================================================ */

bool is_valid_field_type(Type *type, SymbolTable *table)
{
    if (type == NULL)
    {
        return false;
    }

    switch (type->kind)
    {
        /* Primitive types - always valid */
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT:
        case TYPE_UINT32:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
        case TYPE_CHAR:
        case TYPE_STRING:
        case TYPE_BOOL:
        case TYPE_BYTE:
        case TYPE_VOID:
            return true;

        /* Built-in reference types - always valid */
        case TYPE_ANY:
            return true;

        /* Pointer types - valid (checked for native struct separately) */
        case TYPE_POINTER:
            /* Recursively check base type is valid */
            return is_valid_field_type(type->as.pointer.base_type, table);

        /* Array types - check element type is valid */
        case TYPE_ARRAY:
            return is_valid_field_type(type->as.array.element_type, table);

        /* Opaque types - always valid (represents external C types) */
        case TYPE_OPAQUE:
            return true;

        /* Struct types - need to verify the struct is defined */
        case TYPE_STRUCT:
        {
            if (type->as.struct_type.name == NULL)
            {
                return false;
            }
            /* If this is a forward reference (fields == NULL and field_count == 0),
             * verify the struct actually exists in the symbol table */
            if (type->as.struct_type.fields == NULL && type->as.struct_type.field_count == 0)
            {
                if (table == NULL)
                {
                    return false; /* Can't verify without symbol table */
                }
                Token lookup_tok;
                lookup_tok.start = type->as.struct_type.name;
                lookup_tok.length = strlen(type->as.struct_type.name);
                Symbol *sym = symbol_table_lookup_type(table, lookup_tok);
                if (sym == NULL || sym->type == NULL || sym->type->kind != TYPE_STRUCT)
                {
                    return false; /* Struct not defined */
                }
            }
            return true;
        }

        /* Function types (closures) - valid as fields */
        case TYPE_FUNCTION:
            return true;

        /* TYPE_NIL is not a valid field type */
        case TYPE_NIL:
        default:
            return false;
    }
}

/* ============================================================================
 * Circular Dependency Detection for Structs
 * ============================================================================ */

/* Maximum depth for circular dependency detection to prevent stack overflow */
#define MAX_CYCLE_DEPTH 64

/* Helper struct to track visited struct names during cycle detection */
typedef struct {
    const char *names[MAX_CYCLE_DEPTH];
    int count;
} VisitedStructs;

/* Check if a struct name is already in the visited set */
static bool is_struct_visited(VisitedStructs *visited, const char *name)
{
    if (name == NULL) return false;
    for (int i = 0; i < visited->count; i++)
    {
        if (visited->names[i] != NULL && strcmp(visited->names[i], name) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Add a struct name to the visited set. Returns false if full. */
static bool add_visited_struct(VisitedStructs *visited, const char *name)
{
    if (visited->count >= MAX_CYCLE_DEPTH || name == NULL)
    {
        return false;
    }
    visited->names[visited->count++] = name;
    return true;
}

/* Build the dependency chain string from visited structs */
static void build_chain_string(VisitedStructs *visited, const char *cycle_start,
                                char *chain_out, int chain_size)
{
    int offset = 0;
    bool found_start = false;

    for (int i = 0; i < visited->count && offset < chain_size - 1; i++)
    {
        if (!found_start && strcmp(visited->names[i], cycle_start) == 0)
        {
            found_start = true;
        }
        if (found_start)
        {
            int written = snprintf(chain_out + offset, chain_size - offset,
                                    "%s%s", (offset > 0) ? " -> " : "", visited->names[i]);
            if (written > 0) offset += written;
        }
    }
    /* Add the cycle-completing reference back to the start */
    if (offset < chain_size - 1)
    {
        snprintf(chain_out + offset, chain_size - offset, " -> %s", cycle_start);
    }
}

/* Recursive helper to detect circular dependencies in struct field types.
 * The table parameter is used to resolve forward-referenced struct types.
 * Returns true if a cycle is found, false otherwise. */
static bool detect_cycle_in_type(Type *type, SymbolTable *table, VisitedStructs *visited,
                                  char *chain_out, int chain_size)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
        case TYPE_STRUCT:
        {
            const char *struct_name = type->as.struct_type.name;
            if (struct_name == NULL) return false;

            /* Check if we've already visited this struct (cycle detected) */
            if (is_struct_visited(visited, struct_name))
            {
                build_chain_string(visited, struct_name, chain_out, chain_size);
                return true;
            }

            /* Add this struct to visited set */
            if (!add_visited_struct(visited, struct_name))
            {
                /* Too deep, treat as no cycle to avoid stack issues */
                return false;
            }

            /* If this is a forward reference (field_count == 0 and fields == NULL),
             * look up the actual struct type from the symbol table */
            Type *resolved_type = type;
            if (type->as.struct_type.field_count == 0 && type->as.struct_type.fields == NULL && table != NULL)
            {
                /* Create a token for lookup */
                Token lookup_tok;
                lookup_tok.start = struct_name;
                lookup_tok.length = strlen(struct_name);

                Symbol *sym = symbol_table_lookup_type(table, lookup_tok);
                if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
                {
                    resolved_type = sym->type;
                }
            }

            /* Check all fields for cycles */
            for (int i = 0; i < resolved_type->as.struct_type.field_count; i++)
            {
                StructField *field = &resolved_type->as.struct_type.fields[i];
                if (detect_cycle_in_type(field->type, table, visited, chain_out, chain_size))
                {
                    return true;
                }
            }

            /* Remove this struct from visited set (backtrack) */
            visited->count--;
            return false;
        }

        case TYPE_ARRAY:
            /* Arrays of structs can also cause circular dependencies */
            return detect_cycle_in_type(type->as.array.element_type, table, visited, chain_out, chain_size);

        case TYPE_POINTER:
            /* Pointers break cycles - a pointer to a struct is fine */
            return false;

        default:
            /* Other types cannot cause struct circular dependencies */
            return false;
    }
}

bool detect_struct_circular_dependency(Type *struct_type, SymbolTable *table, char *chain_out, int chain_size)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return false;
    }

    if (chain_out != NULL && chain_size > 0)
    {
        chain_out[0] = '\0';
    }

    VisitedStructs visited;
    visited.count = 0;

    /* Start with the root struct */
    const char *root_name = struct_type->as.struct_type.name;
    if (root_name == NULL) return false;

    if (!add_visited_struct(&visited, root_name))
    {
        return false;
    }

    /* Check all fields for cycles */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (detect_cycle_in_type(field->type, table, &visited, chain_out, chain_size))
        {
            return true;
        }
    }

    return false;
}

Type *resolve_struct_forward_reference(Type *type, SymbolTable *table)
{
    if (type == NULL || type->kind != TYPE_STRUCT || table == NULL)
    {
        return type;
    }

    /* Check if this is a forward reference (0 fields and NULL fields pointer) */
    if (type->as.struct_type.field_count == 0 && type->as.struct_type.fields == NULL)
    {
        const char *struct_name = type->as.struct_type.name;
        if (struct_name == NULL)
        {
            return type;
        }

        /* Look up the complete struct definition */
        Token lookup_tok;
        lookup_tok.start = struct_name;
        lookup_tok.length = strlen(struct_name);

        Symbol *sym = symbol_table_lookup_type(table, lookup_tok);
        if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_STRUCT)
        {
            /* Found the complete type - use it instead */
            return sym->type;
        }
    }

    return type;
}

void get_module_symbols(Module *imported_module, SymbolTable *table,
                        Token ***symbols_out, Type ***types_out, int *count_out)
{
    /* Initialize outputs to empty/NULL */
    *symbols_out = NULL;
    *types_out = NULL;
    *count_out = 0;

    if (imported_module == NULL || imported_module->statements == NULL || table == NULL)
    {
        return;
    }

    Arena *arena = table->arena;
    int capacity = 8;
    int count = 0;
    Token **symbols = arena_alloc(arena, sizeof(Token *) * capacity);
    Type **types = arena_alloc(arena, sizeof(Type *) * capacity);

    if (symbols == NULL || types == NULL)
    {
        DEBUG_ERROR("Failed to allocate memory for module symbols");
        return;
    }

    /* Walk through module statements and extract function definitions */
    for (int i = 0; i < imported_module->count; i++)
    {
        Stmt *stmt = imported_module->statements[i];
        if (stmt == NULL)
            continue;

        if (stmt->type == STMT_FUNCTION)
        {
            FunctionStmt *func = &stmt->as.function;

            /* Grow arrays if needed */
            if (count >= capacity)
            {
                int new_capacity = capacity * 2;
                Token **new_symbols = arena_alloc(arena, sizeof(Token *) * new_capacity);
                Type **new_types = arena_alloc(arena, sizeof(Type *) * new_capacity);
                if (new_symbols == NULL || new_types == NULL)
                {
                    DEBUG_ERROR("Failed to grow module symbols arrays");
                    *symbols_out = symbols;
                    *types_out = types;
                    *count_out = count;
                    return;
                }
                memcpy(new_symbols, symbols, sizeof(Token *) * count);
                memcpy(new_types, types, sizeof(Type *) * count);
                symbols = new_symbols;
                types = new_types;
                capacity = new_capacity;
            }

            /* Store symbol name token */
            Token *name_token = arena_alloc(arena, sizeof(Token));
            if (name_token == NULL)
            {
                DEBUG_ERROR("Failed to allocate token for function name");
                continue;
            }
            *name_token = func->name;
            symbols[count] = name_token;

            /* Build function type */
            Type **param_types = NULL;
            if (func->param_count > 0)
            {
                param_types = arena_alloc(arena, sizeof(Type *) * func->param_count);
                if (param_types == NULL)
                {
                    DEBUG_ERROR("Failed to allocate param types");
                    continue;
                }
                for (int j = 0; j < func->param_count; j++)
                {
                    Type *param_type = func->params[j].type;
                    if (param_type == NULL)
                    {
                        param_type = ast_create_primitive_type(arena, TYPE_NIL);
                    }
                    param_types[j] = param_type;
                }
            }

            Type *func_type = ast_create_function_type(arena, func->return_type, param_types, func->param_count);
            /* Carry over variadic, native, and has_body flags from function statement */
            func_type->as.function.is_variadic = func->is_variadic;
            func_type->as.function.is_native = func->is_native;
            func_type->as.function.has_body = (func->body != NULL);

            /* Store parameter memory qualifiers if any non-default exist */
            if (func->param_count > 0)
            {
                bool has_non_default_qual = false;
                for (int j = 0; j < func->param_count; j++)
                {
                    if (func->params[j].mem_qualifier != MEM_DEFAULT)
                    {
                        has_non_default_qual = true;
                        break;
                    }
                }
                if (has_non_default_qual)
                {
                    func_type->as.function.param_mem_quals = arena_alloc(arena,
                        sizeof(MemoryQualifier) * func->param_count);
                    if (func_type->as.function.param_mem_quals != NULL)
                    {
                        for (int j = 0; j < func->param_count; j++)
                        {
                            func_type->as.function.param_mem_quals[j] = func->params[j].mem_qualifier;
                        }
                    }
                }
            }

            types[count] = func_type;
            count++;
        }
    }

    *symbols_out = symbols;
    *types_out = types;
    *count_out = count;
}

/* ============================================================================
 * Struct Layout Calculation
 * ============================================================================
 * These functions compute C-compatible memory layout for struct types.
 * Uses natural alignment rules for cross-platform compatibility.
 * ============================================================================ */

size_t get_type_alignment(Type *type)
{
    if (type == NULL) return 1;

    switch (type->kind)
    {
        /* 1-byte alignment */
        case TYPE_BYTE:
        case TYPE_BOOL:
        case TYPE_CHAR:
            return 1;

        /* 4-byte alignment */
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_FLOAT:
            return 4;

        /* 8-byte alignment */
        case TYPE_INT:
        case TYPE_UINT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_POINTER:
        case TYPE_STRING:
        case TYPE_ARRAY:
        case TYPE_OPAQUE:
        case TYPE_FUNCTION:
            return 8;

        /* Struct types - return computed alignment */
        case TYPE_STRUCT:
            return type->as.struct_type.alignment;

        /* Any type needs 8-byte alignment */
        case TYPE_ANY:
            return 8;

        /* Void and nil have no alignment requirement */
        case TYPE_VOID:
        case TYPE_NIL:
            return 1;

        default:
            return 1;
    }
}

/* Helper: align a value to the next multiple of alignment */
static size_t align_to(size_t value, size_t alignment)
{
    if (alignment == 0) return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

void calculate_struct_layout(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return;
    }

    bool is_packed = struct_type->as.struct_type.is_packed;
    size_t current_offset = 0;
    size_t max_alignment = 1;

    /* Process each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;

        /* Get field size and alignment */
        size_t field_size = get_type_size(field_type);
        size_t field_alignment = get_type_alignment(field_type);

        /* Ensure minimum alignment of 1 */
        if (field_alignment == 0) field_alignment = 1;

        /* For packed structs, use alignment of 1 (no padding) */
        if (is_packed)
        {
            field_alignment = 1;
        }

        /* Align current offset to field's alignment */
        current_offset = align_to(current_offset, field_alignment);

        /* Set field offset */
        field->offset = current_offset;

        /* Advance offset by field size */
        current_offset += field_size;

        /* Track maximum alignment */
        if (field_alignment > max_alignment)
        {
            max_alignment = field_alignment;
        }
    }

    /* For packed structs, no trailing padding needed - alignment is 1 */
    size_t total_size;
    if (is_packed)
    {
        total_size = current_offset;
        max_alignment = 1;
    }
    else
    {
        /* Add trailing padding to make struct size a multiple of its alignment */
        total_size = align_to(current_offset, max_alignment);
    }

    /* Store computed values */
    struct_type->as.struct_type.size = total_size;
    struct_type->as.struct_type.alignment = max_alignment;
}
