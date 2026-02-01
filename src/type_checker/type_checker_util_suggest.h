#ifndef TYPE_CHECKER_UTIL_SUGGEST_H
#define TYPE_CHECKER_UTIL_SUGGEST_H

#include "type_checker_util.h"

/* Compute Levenshtein distance between two strings.
 * Uses O(n) space. */
int levenshtein_distance(const char *s1, int len1, const char *s2, int len2);

/* Find a similar symbol name in the symbol table.
 * Returns NULL if no good match found (distance > 2).
 * Uses a static buffer to return the result. */
const char *find_similar_symbol(SymbolTable *table, const char *name, int name_len);

/* Find a similar method name for a given type.
 * Returns NULL if no good match found. */
const char *find_similar_method(Type *type, const char *method_name);

/* Error reporting with suggestions */
void undefined_variable_error(Token *token, SymbolTable *table);
void undefined_variable_error_for_assign(Token *token, SymbolTable *table);
void invalid_member_error(Token *token, Type *object_type, const char *member_name);
void argument_count_error(Token *token, const char *func_name, int expected, int actual);
void argument_type_error(Token *token, const char *func_name, int arg_index, Type *expected, Type *actual);

#endif /* TYPE_CHECKER_UTIL_SUGGEST_H */
