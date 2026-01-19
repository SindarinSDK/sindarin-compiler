#include "code_gen.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_stmt.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "arena.h"

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file)
{
    DEBUG_VERBOSE("Entering code_gen_init");
    gen->arena = arena;
    gen->label_count = 0;
    gen->symbol_table = symbol_table;
    gen->output = fopen(output_file, "w");
    gen->current_function = NULL;
    gen->current_return_type = NULL;
    gen->temp_count = 0;
    gen->for_continue_label = NULL;

    /* Initialize arena context fields */
    gen->arena_depth = 0;
    gen->in_shared_context = false;
    gen->in_private_context = false;
    gen->current_arena_var = NULL;
    gen->current_func_modifier = FUNC_DEFAULT;

    /* Initialize loop arena fields */
    gen->loop_arena_var = NULL;
    gen->loop_cleanup_label = NULL;

    /* Initialize loop arena stack for nested loops */
    gen->loop_arena_stack = NULL;
    gen->loop_cleanup_stack = NULL;
    gen->loop_arena_depth = 0;
    gen->loop_arena_capacity = 0;

    /* Initialize loop counter tracking for optimization */
    gen->loop_counter_names = NULL;
    gen->loop_counter_count = 0;
    gen->loop_counter_capacity = 0;

    /* Initialize private block arena stack */
    gen->arena_stack = NULL;
    gen->arena_stack_depth = 0;
    gen->arena_stack_capacity = 0;

    /* Initialize lambda fields */
    gen->lambda_count = 0;
    gen->lambda_forward_decls = arena_strdup(arena, "");
    gen->lambda_definitions = arena_strdup(arena, "");
    gen->enclosing_lambdas = NULL;
    gen->enclosing_lambda_count = 0;
    gen->enclosing_lambda_capacity = 0;

    /* Initialize thread wrapper support */
    gen->thread_wrapper_count = 0;

    /* Initialize named function wrapper support */
    gen->wrapper_count = 0;

    /* Initialize buffering fields */
    gen->function_definitions = arena_strdup(arena, "");
    gen->buffering_functions = false;

    /* Initialize optimization settings */
    gen->arithmetic_mode = ARITH_CHECKED;  /* Default to checked arithmetic */

    /* Initialize tail call optimization state */
    gen->in_tail_call_function = false;
    gen->tail_call_fn = NULL;

    /* Initialize captured primitive tracking */
    gen->captured_primitives = NULL;
    gen->captured_prim_ptrs = NULL;
    gen->captured_prim_count = 0;
    gen->captured_prim_capacity = 0;

    /* Initialize pragma tracking */
    gen->pragma_includes = NULL;
    gen->pragma_include_count = 0;
    gen->pragma_include_capacity = 0;
    gen->pragma_links = NULL;
    gen->pragma_link_count = 0;
    gen->pragma_link_capacity = 0;
    gen->pragma_sources = NULL;
    gen->pragma_source_count = 0;
    gen->pragma_source_capacity = 0;

    /* Initialize interceptor thunk support */
    gen->thunk_count = 0;
    gen->thunk_forward_decls = arena_strdup(arena, "");
    gen->thunk_definitions = arena_strdup(arena, "");

    /* Initialize array compound literal context flag */
    gen->in_array_compound_literal = false;

    /* Initialize recursive lambda support */
    gen->current_decl_var_name = NULL;
    gen->recursive_lambda_id = -1;

    if (gen->output == NULL)
    {
        exit(1);
    }
}

void code_gen_cleanup(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_cleanup");
    if (gen->output != NULL)
    {
        fclose(gen->output);
    }
    gen->current_function = NULL;
}

int code_gen_new_label(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_new_label");
    return gen->label_count++;
}

static void code_gen_headers(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_headers");
    indented_fprintf(gen, 0, "#include <stdlib.h>\n");
    indented_fprintf(gen, 0, "#include <string.h>\n");
    indented_fprintf(gen, 0, "#include <stdio.h>\n");
    indented_fprintf(gen, 0, "#include <stdbool.h>\n");
    indented_fprintf(gen, 0, "#include <stdint.h>\n");  /* For int32_t, uint32_t, uint64_t */
    indented_fprintf(gen, 0, "#include <limits.h>\n");
    indented_fprintf(gen, 0, "#include <setjmp.h>\n");  /* For thread panic handling */
    /* pthread.h is included via runtime.h -> runtime_thread.h (handles Windows compatibility) */
    /* Include runtime.h for inline function definitions (comparisons, array_length, etc.) */
    indented_fprintf(gen, 0, "#include \"runtime.h\"\n");
    /* Undefine Windows min/max macros to avoid name collisions with user functions */
    indented_fprintf(gen, 0, "#ifdef _WIN32\n");
    indented_fprintf(gen, 0, "#undef min\n");
    indented_fprintf(gen, 0, "#undef max\n");
    indented_fprintf(gen, 0, "#endif\n\n");
}

static void code_gen_externs(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_externs");

    /* Runtime arena operations - declare first since other functions use RtArena */
    indented_fprintf(gen, 0, "/* Runtime arena operations */\n");
    indented_fprintf(gen, 0, "typedef struct RtArena RtArena;\n");
    indented_fprintf(gen, 0, "extern RtArena *rt_arena_create(RtArena *parent);\n");
    indented_fprintf(gen, 0, "extern void rt_arena_destroy(RtArena *arena);\n");
    indented_fprintf(gen, 0, "extern void *rt_arena_alloc(RtArena *arena, size_t size);\n\n");

    /* Generic closure type for lambdas */
    indented_fprintf(gen, 0, "/* Closure type for lambdas */\n");
    indented_fprintf(gen, 0, "typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;\n\n");

    indented_fprintf(gen, 0, "/* Runtime string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_str_concat(RtArena *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_length(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_substring(RtArena *, const char *, long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_str_indexOf(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split(RtArena *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_trim(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toUpper(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toLower(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_startsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_endsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_contains(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_charAt(const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_long(long long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_double(double);\n");
    indented_fprintf(gen, 0, "extern void rt_print_char(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_string(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_bool(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_byte(unsigned char);\n\n");

    indented_fprintf(gen, 0, "/* Runtime type conversions */\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_long(RtArena *, long long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_double(RtArena *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_char(RtArena *, char);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_bool(RtArena *, int);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_byte(RtArena *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_string(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_void(RtArena *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_pointer(RtArena *, void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime format specifier functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_format_long(RtArena *, long long, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_format_double(RtArena *, double, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_format_string(RtArena *, const char *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime long arithmetic (comparisons are static inline in runtime.h) */\n");
    indented_fprintf(gen, 0, "extern long long rt_add_long(long long, long long);\n");
    indented_fprintf(gen, 0, "extern long long rt_sub_long(long long, long long);\n");
    indented_fprintf(gen, 0, "extern long long rt_mul_long(long long, long long);\n");
    indented_fprintf(gen, 0, "extern long long rt_div_long(long long, long long);\n");
    indented_fprintf(gen, 0, "extern long long rt_mod_long(long long, long long);\n");
    indented_fprintf(gen, 0, "extern long long rt_neg_long(long long);\n");
    /* rt_eq_long, rt_ne_long, etc. are static inline in runtime.h */
    indented_fprintf(gen, 0, "extern long long rt_post_inc_long(long long *);\n");
    indented_fprintf(gen, 0, "extern long long rt_post_dec_long(long long *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime double arithmetic (comparisons are static inline in runtime.h) */\n");
    indented_fprintf(gen, 0, "extern double rt_add_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_sub_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_mul_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_div_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_neg_double(double);\n\n");
    /* rt_eq_double, rt_ne_double, etc. are static inline in runtime.h */

    /* rt_not_bool, rt_eq_string, etc. are declared in runtime.h */

    indented_fprintf(gen, 0, "/* Runtime array operations */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_push_long(RtArena *, long long *, long long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_string(RtArena *, char **, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern void **rt_array_push_ptr(RtArena *, void **, void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_long(long long *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_double(double *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_char(char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_bool(int *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_byte(unsigned char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clear */\n");
    indented_fprintf(gen, 0, "extern void rt_array_clear(void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array pop functions */\n");
    indented_fprintf(gen, 0, "extern long long rt_array_pop_long(long long *);\n");
    indented_fprintf(gen, 0, "extern double rt_array_pop_double(double *);\n");
    indented_fprintf(gen, 0, "extern char rt_array_pop_char(char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_pop_bool(int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char rt_array_pop_byte(unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_pop_string(char **);\n");
    indented_fprintf(gen, 0, "extern void *rt_array_pop_ptr(void **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array concat functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_concat_long(RtArena *, long long *, long long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_concat_double(RtArena *, double *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_concat_char(RtArena *, char *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_concat_string(RtArena *, char **, char **);\n");
    indented_fprintf(gen, 0, "extern void **rt_array_concat_ptr(RtArena *, void **, void **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array slice functions (start, end, step) */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_slice_long(RtArena *, long long *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array reverse functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_rev_long(RtArena *, long long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rev_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rev_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rev_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rev_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array remove functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_rem_long(RtArena *, long long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rem_double(RtArena *, double *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rem_char(RtArena *, char *, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rem_bool(RtArena *, int *, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rem_string(RtArena *, char **, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array insert functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_ins_long(RtArena *, long long *, long long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array push (copy) functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_push_copy_long(RtArena *, long long *, long long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array indexOf functions */\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_long(long long *, long long);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_byte(unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array contains functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_long(long long *, long long);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_byte(unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clone functions */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_clone_long(RtArena *, long long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_clone_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_clone_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_clone_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_clone_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array join functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_long(RtArena *, long long *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_double(RtArena *, double *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_char(RtArena *, char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array create from static data */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_create_long(RtArena *, size_t, const long long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array equality functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_long(long long *, long long *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_double(double *, double *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_char(char *, char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_bool(int *, int *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_byte(unsigned char *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_string(char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime range creation */\n");
    indented_fprintf(gen, 0, "extern long long *rt_array_range(RtArena *, long long, long long);\n\n");

    indented_fprintf(gen, 0, "/* Standard streams (Stdin, Stdout, Stderr) */\n");
    indented_fprintf(gen, 0, "extern char *rt_stdin_read_line(RtArena *);\n");
    indented_fprintf(gen, 0, "extern long rt_stdin_read_char(void);\n");
    indented_fprintf(gen, 0, "extern char *rt_stdin_read_word(RtArena *);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_has_chars(void);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_has_lines(void);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_is_eof(void);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_write(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_write_line(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_flush(void);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_write(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_write_line(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_flush(void);\n\n");

    indented_fprintf(gen, 0, "/* Global convenience functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_read_line(RtArena *);\n");
    indented_fprintf(gen, 0, "extern void rt_println(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_err(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_err_ln(const char *);\n\n");

    indented_fprintf(gen, 0, "/* Byte array extension methods */\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_string(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_string_to_bytes(RtArena *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* String splitting methods */\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split_whitespace(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split_lines(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_is_blank(const char *);\n\n");

    indented_fprintf(gen, 0, "/* Mutable string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_string_with_capacity(RtArena *, size_t);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_from(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_ensure_mutable(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_append(char *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Environment operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_env_get(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_env_names(RtArena *);\n\n");
}

/* Generate C typedef for a native callback type declaration.
 * For type X = native fn(a: int, b: double): void
 * Generates: typedef void (*X)(long, double);
 */
static void code_gen_native_callback_typedef(CodeGen *gen, TypeDeclStmt *type_decl)
{
    Type *type = type_decl->type;

    /* Only handle native function types */
    if (type->kind != TYPE_FUNCTION || !type->as.function.is_native)
    {
        return;
    }

    /* Get the type alias name */
    char *type_name = get_var_name(gen->arena, type_decl->name);

    /* Get the C return type */
    const char *ret_c = get_c_type(gen->arena, type->as.function.return_type);

    /* Generate: typedef ret_type (*TypeName)(param_types); */
    indented_fprintf(gen, 0, "typedef %s (*%s)(", ret_c, type_name);

    /* Generate parameter types */
    for (int i = 0; i < type->as.function.param_count; i++)
    {
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        const char *param_c = get_c_type(gen->arena, type->as.function.param_types[i]);
        fprintf(gen->output, "%s", param_c);
    }

    /* Handle empty parameter list */
    if (type->as.function.param_count == 0)
    {
        fprintf(gen->output, "void");
    }

    fprintf(gen->output, ");\n");
}

static void code_gen_forward_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);

    // Skip main - it doesn't need a forward declaration
    if (strcmp(fn_name, "main") == 0)
    {
        return;
    }

    /* Native functions without a body are external C declarations.
     * We don't generate any forward declaration - they must be provided via
     * #pragma include or linked via #pragma link. */
    if (fn->is_native && fn->body_count == 0)
    {
        return;
    }

    /* New arena model: ALL non-main Sindarin functions receive __caller_arena__ as first param.
     * This is true regardless of whether they are shared, default, or private.
     * The modifier only affects how the local arena is set up inside the function. */
    const char *ret_c = get_c_type(gen->arena, fn->return_type);
    indented_fprintf(gen, 0, "%s %s(", ret_c, fn_name);

    /* All non-main functions receive caller's arena as first parameter */
    fprintf(gen->output, "RtArena *");
    if (fn->param_count > 0)
    {
        fprintf(gen->output, ", ");
    }

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        /* 'as ref' primitive and struct parameters become pointer types */
        bool is_ref_param = false;
        if (fn->params[i].mem_qualifier == MEM_AS_REF && fn->params[i].type != NULL)
        {
            TypeKind kind = fn->params[i].type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            bool is_struct = (kind == TYPE_STRUCT);
            is_ref_param = is_prim || is_struct;
        }
        if (is_ref_param)
        {
            fprintf(gen->output, "%s *", param_type);
        }
        else
        {
            fprintf(gen->output, "%s", param_type);
        }
    }

    /* With new arena model, ALL non-main functions have at least the arena param,
     * so we never need to output "void" for empty parameter list */

    fprintf(gen->output, ");\n");
}

/* Generate extern declaration for native function without body */
static void code_gen_native_extern_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);
    const char *ret_c = get_c_type(gen->arena, fn->return_type);

    indented_fprintf(gen, 0, "extern %s %s(", ret_c, fn_name);

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        /* 'as ref' primitive and struct parameters become pointer types */
        bool is_ref_param = false;
        if (fn->params[i].mem_qualifier == MEM_AS_REF && fn->params[i].type != NULL)
        {
            TypeKind kind = fn->params[i].type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            bool is_struct = (kind == TYPE_STRUCT);
            is_ref_param = is_prim || is_struct;
        }
        if (is_ref_param)
        {
            fprintf(gen->output, "%s *", param_type);
        }
        else
        {
            fprintf(gen->output, "%s", param_type);
        }
    }

    /* Add variadic marker if function is variadic */
    if (fn->is_variadic)
    {
        if (fn->param_count > 0)
        {
            fprintf(gen->output, ", ...");
        }
        else
        {
            fprintf(gen->output, "...");
        }
    }
    else if (fn->param_count == 0)
    {
        fprintf(gen->output, "void");
    }

    fprintf(gen->output, ");\n");
}

/* Helper to add a pragma include (with deduplication) */
static void code_gen_add_pragma_include(CodeGen *gen, const char *include)
{
    /* Check for duplicates */
    for (int i = 0; i < gen->pragma_include_count; i++)
    {
        if (strcmp(gen->pragma_includes[i], include) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_include_count >= gen->pragma_include_capacity)
    {
        int new_capacity = gen->pragma_include_capacity == 0 ? 8 : gen->pragma_include_capacity * 2;
        char **new_includes = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->pragma_include_count; i++)
        {
            new_includes[i] = gen->pragma_includes[i];
        }
        gen->pragma_includes = new_includes;
        gen->pragma_include_capacity = new_capacity;
    }
    gen->pragma_includes[gen->pragma_include_count++] = arena_strdup(gen->arena, include);
}

/* Helper to add a pragma link (with deduplication) */
static void code_gen_add_pragma_link(CodeGen *gen, const char *link)
{
    /* Check for duplicates */
    for (int i = 0; i < gen->pragma_link_count; i++)
    {
        if (strcmp(gen->pragma_links[i], link) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_link_count >= gen->pragma_link_capacity)
    {
        int new_capacity = gen->pragma_link_capacity == 0 ? 8 : gen->pragma_link_capacity * 2;
        char **new_links = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->pragma_link_count; i++)
        {
            new_links[i] = gen->pragma_links[i];
        }
        gen->pragma_links = new_links;
        gen->pragma_link_capacity = new_capacity;
    }
    gen->pragma_links[gen->pragma_link_count++] = arena_strdup(gen->arena, link);
}

/* Helper to extract directory from a file path */
static const char *get_directory_from_path(Arena *arena, const char *filepath)
{
    if (filepath == NULL) return ".";

    char *path_copy = arena_strdup(arena, filepath);
    char *last_sep = strrchr(path_copy, '/');
#ifdef _WIN32
    char *last_win_sep = strrchr(path_copy, '\\');
    if (last_win_sep > last_sep) last_sep = last_win_sep;
#endif

    if (last_sep != NULL)
    {
        *last_sep = '\0';
        return path_copy;
    }
    return ".";
}

/* Helper to add a pragma source file with location info (with deduplication) */
static void code_gen_add_pragma_source(CodeGen *gen, const char *source, const char *source_dir)
{
    /* Check for duplicates (same value and source_dir) */
    for (int i = 0; i < gen->pragma_source_count; i++)
    {
        if (strcmp(gen->pragma_sources[i].value, source) == 0 &&
            strcmp(gen->pragma_sources[i].source_dir, source_dir) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_source_count >= gen->pragma_source_capacity)
    {
        int new_capacity = gen->pragma_source_capacity == 0 ? 8 : gen->pragma_source_capacity * 2;
        PragmaSourceInfo *new_sources = arena_alloc(gen->arena, new_capacity * sizeof(PragmaSourceInfo));
        for (int i = 0; i < gen->pragma_source_count; i++)
        {
            new_sources[i] = gen->pragma_sources[i];
        }
        gen->pragma_sources = new_sources;
        gen->pragma_source_capacity = new_capacity;
    }
    gen->pragma_sources[gen->pragma_source_count].value = arena_strdup(gen->arena, source);
    gen->pragma_sources[gen->pragma_source_count].source_dir = arena_strdup(gen->arena, source_dir);
    gen->pragma_source_count++;
}

/* Collect pragma directives from a list of statements (recursively handles imports) */
static void code_gen_collect_pragmas(CodeGen *gen, Stmt **statements, int count)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_PRAGMA)
        {
            if (stmt->as.pragma.pragma_type == PRAGMA_INCLUDE)
            {
                code_gen_add_pragma_include(gen, stmt->as.pragma.value);
            }
            else if (stmt->as.pragma.pragma_type == PRAGMA_LINK)
            {
                code_gen_add_pragma_link(gen, stmt->as.pragma.value);
            }
            else if (stmt->as.pragma.pragma_type == PRAGMA_SOURCE)
            {
                const char *source_dir = get_directory_from_path(gen->arena,
                    stmt->token ? stmt->token->filename : NULL);
                code_gen_add_pragma_source(gen, stmt->as.pragma.value, source_dir);
            }
        }
        else if (stmt->type == STMT_IMPORT &&
                 stmt->as.import.imported_stmts != NULL)
        {
            /* Recursively collect pragmas from all imported modules */
            code_gen_collect_pragmas(gen, stmt->as.import.imported_stmts,
                                     stmt->as.import.imported_count);
        }
    }
}

void code_gen_module(CodeGen *gen, Module *module)
{
    DEBUG_VERBOSE("Entering code_gen_module");

    // First collect pragma directives
    code_gen_collect_pragmas(gen, module->statements, module->count);

    code_gen_headers(gen);

    // Emit pragma includes after standard headers
    if (gen->pragma_include_count > 0)
    {
        indented_fprintf(gen, 0, "/* User-specified includes */\n");
        for (int i = 0; i < gen->pragma_include_count; i++)
        {
            indented_fprintf(gen, 0, "#include %s\n", gen->pragma_includes[i]);
        }
        indented_fprintf(gen, 0, "\n");
    }

    code_gen_externs(gen);

    // Emit opaque type forward struct declarations
    // Skip standard C library types that are already defined (e.g., FILE from stdio.h)
    int opaque_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_TYPE_DECL)
        {
            TypeDeclStmt *type_decl = &stmt->as.type_decl;
            if (type_decl->type->kind == TYPE_OPAQUE && type_decl->type->as.opaque.name != NULL)
            {
                const char *name = type_decl->type->as.opaque.name;
                /* Skip standard C library types that are typically provided by headers */
                if (strcmp(name, "FILE") == 0 ||
                    strcmp(name, "DIR") == 0 ||
                    strcmp(name, "dirent") == 0)
                {
                    continue;
                }
                if (opaque_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Opaque type forward declarations */\n");
                }
                indented_fprintf(gen, 0, "typedef struct %s %s;\n", name, name);
                opaque_count++;
            }
        }
    }
    if (opaque_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit forward declarations for native structs with c_alias
    // These are opaque handle types that alias external C types (e.g., SnDate -> RtDate)
    int native_alias_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            if (struct_decl->is_native && struct_decl->c_alias != NULL)
            {
                if (native_alias_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Native struct forward declarations */\n");
                }
                indented_fprintf(gen, 0, "typedef struct %s %s;\n",
                    struct_decl->c_alias, struct_decl->c_alias);
                native_alias_count++;
            }
        }
    }
    if (native_alias_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit struct type definitions
    int struct_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;

            /* Skip native structs that have a c_alias - they are aliases to external types
             * and should not generate a typedef. The c_alias will be used directly in code. */
            if (struct_decl->is_native && struct_decl->c_alias != NULL)
            {
                continue;
            }

            if (struct_count == 0)
            {
                indented_fprintf(gen, 0, "/* Struct type definitions */\n");
            }
            /* Emit #pragma pack for packed structs */
            if (struct_decl->is_packed)
            {
                indented_fprintf(gen, 0, "#pragma pack(push, 1)\n");
            }
            /* Generate: typedef struct { fields... } StructName; */
            indented_fprintf(gen, 0, "typedef struct {\n");
            for (int j = 0; j < struct_decl->field_count; j++)
            {
                StructField *field = &struct_decl->fields[j];
                const char *c_type = get_c_type(gen->arena, field->type);
                indented_fprintf(gen, 1, "%s %s;\n", c_type, field->name);
            }
            char *struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
            indented_fprintf(gen, 0, "} %s;\n", struct_name);
            /* Close #pragma pack for packed structs */
            if (struct_decl->is_packed)
            {
                indented_fprintf(gen, 0, "#pragma pack(pop)\n");
            }
            struct_count++;
        }
    }
    if (struct_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    /* Emit struct method forward declarations */
    int method_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);

            /* Create lowercase struct name for native method naming */
            char *struct_name_lower = arena_strdup(gen->arena, struct_name);
            for (char *p = struct_name_lower; *p; p++)
            {
                *p = (char)tolower((unsigned char)*p);
            }

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                if (method_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Struct method forward declarations */\n");
                }

                const char *ret_type = get_c_type(gen->arena, method->return_type);

                if (method->is_native && method->body == NULL)
                {
                    /* Native method - generate extern declaration.
                     * If c_alias is set, use that as the function name.
                     * Otherwise, use the rt_{struct_lowercase}_{method_name} convention. */
                    const char *func_name;
                    if (method->c_alias != NULL)
                    {
                        func_name = method->c_alias;
                    }
                    else
                    {
                        func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
                    }

                    /* For opaque handle types (native struct with c_alias), use the C type directly */
                    const char *self_c_type;
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is already a pointer (e.g., RtDate *) */
                        self_c_type = arena_sprintf(gen->arena, "%s *", struct_decl->c_alias);
                    }
                    else
                    {
                        self_c_type = struct_name;
                    }

                    if (method->is_static)
                    {
                        /* Static native: extern RetType func_name(params); */
                        indented_fprintf(gen, 0, "extern %s %s(",
                                         ret_type, func_name);
                        if (method->param_count == 0)
                        {
                            indented_fprintf(gen, 0, "void");
                        }
                        else
                        {
                            for (int k = 0; k < method->param_count; k++)
                            {
                                if (k > 0)
                                    indented_fprintf(gen, 0, ", ");
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_type(gen->arena, param->type);
                                indented_fprintf(gen, 0, "%s", param_type);
                            }
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                    else
                    {
                        /* Instance native: extern RetType func_name(self_type self, params); */
                        indented_fprintf(gen, 0, "extern %s %s(%s",
                                         ret_type, func_name, self_c_type);
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_type(gen->arena, param->type);
                            indented_fprintf(gen, 0, ", %s", param_type);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                else
                {
                    /* Non-native method - regular forward declaration with arena */
                    if (method->is_static)
                    {
                        /* Static method: no self parameter */
                        if (method->param_count == 0)
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena);\n",
                                             ret_type, struct_name, method->name);
                        }
                        else
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena",
                                             ret_type, struct_name, method->name);
                            for (int k = 0; k < method->param_count; k++)
                            {
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_type(gen->arena, param->type);
                                char *param_name = arena_strndup(gen->arena, param->name.start, param->name.length);
                                indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                            }
                            indented_fprintf(gen, 0, ");\n");
                        }
                    }
                    else
                    {
                        /* Instance method: first parameter is self (pointer to struct) */
                        /* For opaque handle types (native struct with c_alias), self is already a pointer */
                        if (struct_decl->is_native && struct_decl->c_alias != NULL)
                        {
                            /* Opaque handle: self type is the C alias pointer */
                            indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena, %s *self",
                                             ret_type, struct_name, method->name, struct_decl->c_alias);
                        }
                        else
                        {
                            /* Regular struct: self is pointer to struct */
                            indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena, %s *self",
                                             ret_type, struct_name, method->name, struct_name);
                        }
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_type(gen->arena, param->type);
                            char *param_name = arena_strndup(gen->arena, param->name.start, param->name.length);
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                method_count++;
            }
        }
    }
    if (method_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit type declarations (native callback typedefs) before forward declarations
    int typedef_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_TYPE_DECL)
        {
            TypeDeclStmt *type_decl = &stmt->as.type_decl;
            if (type_decl->type->kind == TYPE_FUNCTION && type_decl->type->as.function.is_native)
            {
                if (typedef_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Native callback type definitions */\n");
                }
                code_gen_native_callback_typedef(gen, type_decl);
                typedef_count++;
            }
        }
    }
    if (typedef_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit extern declarations for native functions without body
    int native_extern_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            FunctionStmt *fn = &stmt->as.function;
            if (fn->is_native && fn->body_count == 0)
            {
                /* Skip extern declaration for runtime functions (starting with rt_)
                 * since they are already declared in runtime headers. */
                char *fn_name = arena_strndup(gen->arena, fn->name.start, fn->name.length);
                if (strncmp(fn_name, "rt_", 3) == 0)
                {
                    continue;
                }
                if (native_extern_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Native function extern declarations */\n");
                }
                code_gen_native_extern_declaration(gen, fn);
                native_extern_count++;
            }
        }
    }
    if (native_extern_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Second pass: emit forward declarations for all user-defined functions
    indented_fprintf(gen, 0, "/* Forward declarations */\n");
    int forward_decl_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            char *fn_name = get_var_name(gen->arena, stmt->as.function.name);
            if (strcmp(fn_name, "main") != 0)
            {
                code_gen_forward_declaration(gen, &stmt->as.function);
                forward_decl_count++;
            }
        }
    }
    if (forward_decl_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Second pass: emit full function definitions to a temp file
    // This allows us to collect lambda forward declarations first
    FILE *original_output = gen->output;
    FILE *func_temp = tmpfile();
    if (func_temp == NULL)
    {
        fprintf(stderr, "Error: Failed to create temp file for function buffering\n");
        exit(1);
    }
    gen->output = func_temp;

    bool has_main = false;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION &&
            stmt->as.function.name.length == 4 &&
            strncmp(stmt->as.function.name.start, "main", 4) == 0)
        {
            has_main = true;
        }
        code_gen_statement(gen, stmt, 0);
    }

    /* Emit struct method implementations */
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                /* Skip native methods with no body - they are extern declared elsewhere */
                if (method->is_native && method->body == NULL)
                {
                    continue;
                }

                const char *ret_type = get_c_type(gen->arena, method->return_type);

                /* Generate function signature */
                if (method->is_static)
                {
                    if (method->param_count == 0)
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena) {\n",
                                         ret_type, struct_name, method->name);
                    }
                    else
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena",
                                         ret_type, struct_name, method->name);
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_type(gen->arena, param->type);
                            char *param_name = arena_strndup(gen->arena, param->name.start, param->name.length);
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ") {\n");
                    }
                }
                else
                {
                    /* Instance method: first parameter is self (pointer to struct) */
                    /* For opaque handle types (native struct with c_alias), self is already a pointer */
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is the C alias pointer */
                        indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena, %s *self",
                                         ret_type, struct_name, method->name, struct_decl->c_alias);
                    }
                    else
                    {
                        /* Regular struct: self is pointer to struct */
                        indented_fprintf(gen, 0, "%s %s_%s(RtArena *arena, %s *self",
                                         ret_type, struct_name, method->name, struct_name);
                    }
                    for (int k = 0; k < method->param_count; k++)
                    {
                        Parameter *param = &method->params[k];
                        const char *param_type = get_c_type(gen->arena, param->type);
                        char *param_name = arena_strndup(gen->arena, param->name.start, param->name.length);
                        indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                    }
                    indented_fprintf(gen, 0, ") {\n");
                }

                /* Set up code generator state for method */
                char *method_full_name = arena_sprintf(gen->arena, "%s_%s", struct_name, method->name);
                const char *saved_function = gen->current_function;
                Type *saved_return_type = gen->current_return_type;
                const char *saved_arena_var = gen->current_arena_var;

                gen->current_function = method_full_name;
                gen->current_return_type = method->return_type;
                gen->current_arena_var = "arena";  /* Methods receive arena as first parameter */

                /* Determine if we need a _return_value variable */
                bool has_return_value = (method->return_type && method->return_type->kind != TYPE_VOID);

                /* Add _return_value if needed */
                if (has_return_value)
                {
                    const char *default_val = get_default_value(method->return_type);
                    indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_type, default_val);
                }

                /* Generate method body */
                for (int k = 0; k < method->body_count; k++)
                {
                    if (method->body[k] != NULL)
                    {
                        code_gen_statement(gen, method->body[k], 1);
                    }
                }

                /* Add return label and return statement */
                indented_fprintf(gen, 0, "%s_return:\n", method_full_name);
                if (has_return_value)
                {
                    indented_fprintf(gen, 1, "return _return_value;\n");
                }
                else
                {
                    indented_fprintf(gen, 1, "return;\n");
                }

                /* Restore code generator state */
                gen->current_function = saved_function;
                gen->current_return_type = saved_return_type;
                gen->current_arena_var = saved_arena_var;

                /* Close function */
                indented_fprintf(gen, 0, "}\n\n");
            }
        }
    }

    if (!has_main)
    {
        // Generate main with arena lifecycle
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "RtArena *__local_arena__ = rt_arena_create(NULL);\n");
        indented_fprintf(gen, 1, "int _return_value = 0;\n");
        indented_fprintf(gen, 1, "goto main_return;\n");
        indented_fprintf(gen, 0, "main_return:\n");
        indented_fprintf(gen, 1, "rt_arena_destroy(__local_arena__);\n");
        indented_fprintf(gen, 1, "return _return_value;\n");
        indented_fprintf(gen, 0, "}\n");
    }

    // Restore original output
    gen->output = original_output;

    /* Output accumulated lambda forward declarations BEFORE function definitions */
    if (gen->lambda_forward_decls && strlen(gen->lambda_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* Lambda forward declarations */\n");
        fprintf(gen->output, "%s", gen->lambda_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

    /* Output accumulated interceptor thunk forward declarations */
    if (gen->thunk_forward_decls && strlen(gen->thunk_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* Interceptor thunk forward declarations */\n");
        fprintf(gen->output, "%s", gen->thunk_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

    /* Now copy the buffered function definitions */
    fseek(func_temp, 0, SEEK_END);
    long func_size = ftell(func_temp);
    if (func_size > 0)
    {
        fseek(func_temp, 0, SEEK_SET);
        char *func_buf = arena_alloc(gen->arena, func_size + 1);
        size_t read_size = fread(func_buf, 1, func_size, func_temp);
        func_buf[read_size] = '\0';
        fprintf(gen->output, "%s", func_buf);
    }
    fclose(func_temp);

    /* Output accumulated lambda function definitions at the end */
    if (gen->lambda_definitions && strlen(gen->lambda_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* Lambda function definitions */\n");
        fprintf(gen->output, "%s", gen->lambda_definitions);
    }

    /* Output accumulated interceptor thunk definitions */
    if (gen->thunk_definitions && strlen(gen->thunk_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* Interceptor thunk definitions */\n");
        fprintf(gen->output, "%s", gen->thunk_definitions);
    }
}
