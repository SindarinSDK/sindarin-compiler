#include "code_gen/expr/code_gen_expr_match.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "../platform/compat_io.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *code_gen_match_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_match_expression");
    MatchExpr *match = &expr->as.match_expr;
    int match_id = gen->match_count++;

    /* Generate subject expression */
    bool saved_as_handle = gen->expr_as_handle;
    Type *subject_type = match->subject->expr_type;
    if (subject_type && is_handle_type(subject_type))
    {
        if (gen->current_arena_var != NULL)
            gen->expr_as_handle = true;  /* Arena mode: keep as handle */
        else
            gen->expr_as_handle = false;
    }
    char *subject_str = code_gen_expression(gen, match->subject);
    gen->expr_as_handle = saved_as_handle;

    /* Get C type for subject */
    const char *subject_c_type;
    if (subject_type && subject_type->kind == TYPE_STRING)
    {
        if (gen->current_arena_var != NULL)
            subject_c_type = "RtHandleV2 *";  /* Arena mode: handle type */
        else
            subject_c_type = "char *";
    }
    else if (subject_type && subject_type->kind == TYPE_ARRAY)
    {
        const char *elem_c = get_c_array_elem_type(gen->arena, subject_type->as.array.element_type);
        subject_c_type = arena_sprintf(gen->arena, "%s *", elem_c);
    }
    else
    {
        subject_c_type = get_c_type(gen->arena, subject_type);
    }

    /* Determine if expression context (non-void result type) */
    bool is_expr_context = (expr->expr_type != NULL && expr->expr_type->kind != TYPE_VOID);
    const char *result_c_type = is_expr_context ? get_c_type(gen->arena, expr->expr_type) : NULL;

    /* Variable names */
    char *subj_var = arena_sprintf(gen->arena, "_match_subj_%d", match_id);
    char *result_var = is_expr_context ? arena_sprintf(gen->arena, "_match_res_%d", match_id) : NULL;

    /* Start building the ({...}) block */
    char *result = arena_sprintf(gen->arena, "({ %s %s = %s; ", subject_c_type, subj_var, subject_str);

    if (is_expr_context)
    {
        result = arena_sprintf(gen->arena, "%s%s %s; ", result, result_c_type, result_var);
    }

    /* Generate if-else chain for arms */
    bool first_non_else = true;
    for (int i = 0; i < match->arm_count; i++)
    {
        MatchArm *arm = &match->arms[i];

        if (arm->is_else)
        {
            result = arena_sprintf(gen->arena, "%s else { ", result);
        }
        else
        {
            /* Build condition: subj == pattern1 || subj == pattern2 || ... */
            char *condition = arena_strdup(gen->arena, "");
            for (int j = 0; j < arm->pattern_count; j++)
            {
                /* Pattern values for string comparisons */
                bool saved_pat_handle = gen->expr_as_handle;
                if (subject_type && subject_type->kind == TYPE_STRING)
                {
                    if (gen->current_arena_var != NULL)
                        gen->expr_as_handle = true;  /* Arena mode: keep as handle */
                    else
                        gen->expr_as_handle = false;
                }
                char *pattern_str = code_gen_expression(gen, arm->patterns[j]);
                gen->expr_as_handle = saved_pat_handle;
                char *cmp;
                if (subject_type->kind == TYPE_STRING)
                {
                    if (gen->current_arena_var != NULL)
                        cmp = arena_sprintf(gen->arena, "rt_eq_string_v2(%s, %s)", subj_var, pattern_str);
                    else
                        cmp = arena_sprintf(gen->arena, "rt_eq_string(%s, %s)", subj_var, pattern_str);
                }
                else
                {
                    cmp = arena_sprintf(gen->arena, "%s == %s", subj_var, pattern_str);
                }
                if (j > 0)
                {
                    condition = arena_sprintf(gen->arena, "%s || %s", condition, cmp);
                }
                else
                {
                    condition = cmp;
                }
            }

            if (first_non_else)
            {
                result = arena_sprintf(gen->arena, "%sif (%s) { ", result, condition);
                first_non_else = false;
            }
            else
            {
                result = arena_sprintf(gen->arena, "%s else if (%s) { ", result, condition);
            }
        }

        /* Generate arm body */
        if (arm->body != NULL && arm->body->type == STMT_BLOCK)
        {
            FILE *old_output = gen->output;
            char *body_buffer = NULL;
            size_t body_size = 0;
            gen->output = open_memstream(&body_buffer, &body_size);

            int stmt_count = arm->body->as.block.count;
            for (int s = 0; s < stmt_count; s++)
            {
                Stmt *stmt = arm->body->as.block.statements[s];
                bool is_last = (s == stmt_count - 1);

                if (is_expr_context && is_last && stmt->type == STMT_EXPR)
                {
                    /* Last expression in expression context: assign to result variable.
                     * For handle types (string/array), set expr_as_handle so variables
                     * return handles and concat returns handles. */
                    bool saved_as_handle = gen->expr_as_handle;
                    if (expr->expr_type && is_handle_type(expr->expr_type) &&
                        gen->current_arena_var != NULL)
                    {
                        gen->expr_as_handle = true;
                    }
                    char *val_str = code_gen_expression(gen, stmt->as.expression.expression);
                    gen->expr_as_handle = saved_as_handle;

                    fprintf(gen->output, "%s = %s; ", result_var, val_str);
                }
                else
                {
                    code_gen_statement(gen, stmt, 0);
                }
            }

            sn_fclose(gen->output);
            gen->output = old_output;

            char *body_str = arena_strdup(gen->arena, body_buffer ? body_buffer : "");
            free(body_buffer);

            result = arena_sprintf(gen->arena, "%s%s} ", result, body_str);
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s} ", result);
        }
    }

    /* Close the ({...}) block */
    if (is_expr_context)
    {
        result = arena_sprintf(gen->arena, "%s%s; })", result, result_var);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s(void)0; })", result);
    }

    return result;
}
