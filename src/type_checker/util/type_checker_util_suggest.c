#include "type_checker_util_suggest.h"
#include "diagnostic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimum of three integers */
static int min3(int a, int b, int c)
{
    int min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

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
    type_checker_set_error();
}

void argument_type_error(Token *token, const char *func_name, int arg_index, Type *expected, Type *actual)
{
    diagnostic_error_at(token, "argument %d of '%s': expected '%s', got '%s'",
                        arg_index + 1, func_name, type_name(expected), type_name(actual));
    type_checker_set_error();
}
