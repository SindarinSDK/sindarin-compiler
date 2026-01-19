#include "ast/ast_print.h"
#include "ast/ast_type.h"
#include "debug.h"
#include <inttypes.h>

/* Helper functions to convert memory modifiers to strings */
static const char *memory_qualifier_to_string(MemoryQualifier qual)
{
    switch (qual)
    {
    case MEM_DEFAULT: return NULL;  /* Don't print default */
    case MEM_AS_VAL: return "as val";
    case MEM_AS_REF: return "as ref";
    default: return "unknown";
    }
}

static const char *block_modifier_to_string(BlockModifier mod)
{
    switch (mod)
    {
    case BLOCK_DEFAULT: return NULL;  /* Don't print default */
    case BLOCK_SHARED: return "shared";
    case BLOCK_PRIVATE: return "private";
    default: return "unknown";
    }
}

static const char *function_modifier_to_string(FunctionModifier mod)
{
    switch (mod)
    {
    case FUNC_DEFAULT: return NULL;  /* Don't print default */
    case FUNC_SHARED: return "shared";
    case FUNC_PRIVATE: return "private";
    default: return "unknown";
    }
}

void ast_print_stmt(Arena *arena, Stmt *stmt, int indent_level)
{
    if (stmt == NULL)
    {
        return;
    }

    switch (stmt->type)
    {
    case STMT_EXPR:
        DEBUG_VERBOSE_INDENT(indent_level, "ExpressionStmt:");
        ast_print_expr(arena, stmt->as.expression.expression, indent_level + 1);
        break;

    case STMT_VAR_DECL:
    {
        const char *mem_qual = memory_qualifier_to_string(stmt->as.var_decl.mem_qualifier);
        if (mem_qual)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "VarDecl: %.*s (type: %s, %s)",
                                 stmt->as.var_decl.name.length,
                                 stmt->as.var_decl.name.start,
                                 ast_type_to_string(arena, stmt->as.var_decl.type),
                                 mem_qual);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "VarDecl: %.*s (type: %s)",
                                 stmt->as.var_decl.name.length,
                                 stmt->as.var_decl.name.start,
                                 ast_type_to_string(arena, stmt->as.var_decl.type));
        }
        if (stmt->as.var_decl.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            ast_print_expr(arena, stmt->as.var_decl.initializer, indent_level + 2);
        }
        break;
    }

    case STMT_FUNCTION:
    {
        const char *func_mod = function_modifier_to_string(stmt->as.function.modifier);
        if (func_mod)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Function: %.*s %s (return: %s)",
                                 stmt->as.function.name.length,
                                 stmt->as.function.name.start,
                                 func_mod,
                                 ast_type_to_string(arena, stmt->as.function.return_type));
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Function: %.*s (return: %s)",
                                 stmt->as.function.name.length,
                                 stmt->as.function.name.start,
                                 ast_type_to_string(arena, stmt->as.function.return_type));
        }
        if (stmt->as.function.param_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Parameters:");
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                const char *param_qual = memory_qualifier_to_string(stmt->as.function.params[i].mem_qualifier);
                if (param_qual)
                {
                    DEBUG_VERBOSE_INDENT(indent_level + 1, "%.*s: %s %s",
                                         stmt->as.function.params[i].name.length,
                                         stmt->as.function.params[i].name.start,
                                         ast_type_to_string(arena, stmt->as.function.params[i].type),
                                         param_qual);
                }
                else
                {
                    DEBUG_VERBOSE_INDENT(indent_level + 1, "%.*s: %s",
                                         stmt->as.function.params[i].name.length,
                                         stmt->as.function.params[i].name.start,
                                         ast_type_to_string(arena, stmt->as.function.params[i].type));
                }
            }
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        for (int i = 0; i < stmt->as.function.body_count; i++)
        {
            ast_print_stmt(arena, stmt->as.function.body[i], indent_level + 2);
        }
        break;
    }

    case STMT_RETURN:
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Return:");
        if (stmt->as.return_stmt.value)
        {
            ast_print_expr(arena, stmt->as.return_stmt.value, indent_level + 1);
        }
        break;

    case STMT_BLOCK:
    {
        const char *block_mod = block_modifier_to_string(stmt->as.block.modifier);
        if (block_mod)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Block (%s):", block_mod);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Block:");
        }
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            ast_print_stmt(arena, stmt->as.block.statements[i], indent_level + 1);
        }
        break;
    }

    case STMT_IF:
        DEBUG_VERBOSE_INDENT(indent_level, "If:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        ast_print_expr(arena, stmt->as.if_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Then:");
        ast_print_stmt(arena, stmt->as.if_stmt.then_branch, indent_level + 2);
        if (stmt->as.if_stmt.else_branch)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Else:");
            ast_print_stmt(arena, stmt->as.if_stmt.else_branch, indent_level + 2);
        }
        break;

    case STMT_WHILE:
        if (stmt->as.while_stmt.is_shared)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "While (shared):");
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "While:");
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        ast_print_expr(arena, stmt->as.while_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_stmt(arena, stmt->as.while_stmt.body, indent_level + 2);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.is_shared)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "For (shared):");
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "For:");
        }
        if (stmt->as.for_stmt.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            ast_print_stmt(arena, stmt->as.for_stmt.initializer, indent_level + 2);
        }
        if (stmt->as.for_stmt.condition)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
            ast_print_expr(arena, stmt->as.for_stmt.condition, indent_level + 2);
        }
        if (stmt->as.for_stmt.increment)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Increment:");
            ast_print_expr(arena, stmt->as.for_stmt.increment, indent_level + 2);
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_stmt(arena, stmt->as.for_stmt.body, indent_level + 2);
        break;

    case STMT_FOR_EACH:
        if (stmt->as.for_each_stmt.is_shared)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "ForEach (shared): %.*s",
                                 stmt->as.for_each_stmt.var_name.length,
                                 stmt->as.for_each_stmt.var_name.start);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "ForEach: %.*s",
                                 stmt->as.for_each_stmt.var_name.length,
                                 stmt->as.for_each_stmt.var_name.start);
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Iterable:");
        ast_print_expr(arena, stmt->as.for_each_stmt.iterable, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_stmt(arena, stmt->as.for_each_stmt.body, indent_level + 2);
        break;

    case STMT_IMPORT:
        if (stmt->as.import.namespace != NULL)
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Import: %.*s as %.*s",
                                 stmt->as.import.module_name.length,
                                 stmt->as.import.module_name.start,
                                 stmt->as.import.namespace->length,
                                 stmt->as.import.namespace->start);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level, "Import: %.*s",
                                 stmt->as.import.module_name.length,
                                 stmt->as.import.module_name.start);
        }
        break;

    case STMT_BREAK:
        DEBUG_VERBOSE_INDENT(indent_level, "Break");
        break;

    case STMT_CONTINUE:
        DEBUG_VERBOSE_INDENT(indent_level, "Continue");
        break;

    case STMT_PRAGMA:
        DEBUG_VERBOSE_INDENT(indent_level, "Pragma: %s \"%s\"",
                             stmt->as.pragma.pragma_type == PRAGMA_INCLUDE ? "include" : "link",
                             stmt->as.pragma.value);
        break;

    case STMT_TYPE_DECL:
        DEBUG_VERBOSE_INDENT(indent_level, "TypeDecl: %.*s = %s",
                             stmt->as.type_decl.name.length,
                             stmt->as.type_decl.name.start,
                             ast_type_to_string(arena, stmt->as.type_decl.type));
        break;
    }
}

void ast_print_expr(Arena *arena, Expr *expr, int indent_level)
{
    if (expr == NULL)
    {
        return;
    }

    switch (expr->type)
    {
    case EXPR_BINARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Binary: %d", expr->as.binary.operator);
        ast_print_expr(arena, expr->as.binary.left, indent_level + 1);
        ast_print_expr(arena, expr->as.binary.right, indent_level + 1);
        break;

    case EXPR_UNARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Unary: %d", expr->as.unary.operator);
        ast_print_expr(arena, expr->as.unary.operand, indent_level + 1);
        break;

    case EXPR_LITERAL:
        DEBUG_VERBOSE_INDENT(indent_level, "Literal%s: ", expr->as.literal.is_interpolated ? " (interpolated)" : "");
        switch (expr->as.literal.type->kind)
        {
        case TYPE_INT:
            DEBUG_VERBOSE_INDENT(indent_level, "%" PRId64, expr->as.literal.value.int_value);
            break;
        case TYPE_DOUBLE:
            DEBUG_VERBOSE_INDENT(indent_level, "%f", expr->as.literal.value.double_value);
            break;
        case TYPE_CHAR:
            DEBUG_VERBOSE_INDENT(indent_level, "'%c'", expr->as.literal.value.char_value);
            break;
        case TYPE_STRING:
            DEBUG_VERBOSE_INDENT(indent_level, "\"%s\"", expr->as.literal.value.string_value);
            break;
        case TYPE_BOOL:
            DEBUG_VERBOSE_INDENT(indent_level, "%s", expr->as.literal.value.bool_value ? "true" : "false");
            break;
        default:
            DEBUG_VERBOSE_INDENT(indent_level, "unknown");
        }
        DEBUG_VERBOSE_INDENT(indent_level, " (%s)", ast_type_to_string(arena, expr->as.literal.type));
        break;

    case EXPR_VARIABLE:
        DEBUG_VERBOSE_INDENT(indent_level, "Variable: %.*s",
                             expr->as.variable.name.length,
                             expr->as.variable.name.start);
        break;

    case EXPR_ASSIGN:
        DEBUG_VERBOSE_INDENT(indent_level, "Assign: %.*s",
                             expr->as.assign.name.length,
                             expr->as.assign.name.start);
        ast_print_expr(arena, expr->as.assign.value, indent_level + 1);
        break;

    case EXPR_INDEX_ASSIGN:
        DEBUG_VERBOSE_INDENT(indent_level, "IndexAssign:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Array:");
        ast_print_expr(arena, expr->as.index_assign.array, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Index:");
        ast_print_expr(arena, expr->as.index_assign.index, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Value:");
        ast_print_expr(arena, expr->as.index_assign.value, indent_level + 2);
        break;

    case EXPR_CALL:
        DEBUG_VERBOSE_INDENT(indent_level, "Call:");
        ast_print_expr(arena, expr->as.call.callee, indent_level + 1);
        if (expr->as.call.arg_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Arguments:");
            for (int i = 0; i < expr->as.call.arg_count; i++)
            {
                ast_print_expr(arena, expr->as.call.arguments[i], indent_level + 2);
            }
        }
        break;

    case EXPR_ARRAY:
        DEBUG_VERBOSE_INDENT(indent_level, "Array:");
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            ast_print_expr(arena, expr->as.array.elements[i], indent_level + 1);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        DEBUG_VERBOSE_INDENT(indent_level, "ArrayAccess:");
        ast_print_expr(arena, expr->as.array_access.array, indent_level + 1);
        ast_print_expr(arena, expr->as.array_access.index, indent_level + 1);
        break;

    case EXPR_INCREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Increment:");
        ast_print_expr(arena, expr->as.operand, indent_level + 1);
        break;

    case EXPR_DECREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Decrement:");
        ast_print_expr(arena, expr->as.operand, indent_level + 1);
        break;

    case EXPR_INTERPOLATED:
        DEBUG_VERBOSE_INDENT(indent_level, "Interpolated String:");
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            ast_print_expr(arena, expr->as.interpol.parts[i], indent_level + 1);
        }
        break;

    case EXPR_MEMBER:
        DEBUG_VERBOSE_INDENT(indent_level, "Member Access: %.*s",
                             expr->as.member.member_name.length,
                             expr->as.member.member_name.start);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Object:");
        ast_print_expr(arena, expr->as.member.object, indent_level + 2);
        break;

    case EXPR_ARRAY_SLICE:
        DEBUG_VERBOSE_INDENT(indent_level, "ArraySlice:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Array:");
        ast_print_expr(arena, expr->as.array_slice.array, indent_level + 2);
        if (expr->as.array_slice.start)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Start:");
            ast_print_expr(arena, expr->as.array_slice.start, indent_level + 2);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Start: (beginning)");
        }
        if (expr->as.array_slice.end)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "End:");
            ast_print_expr(arena, expr->as.array_slice.end, indent_level + 2);
        }
        else
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "End: (end)");
        }
        break;

    case EXPR_RANGE:
        DEBUG_VERBOSE_INDENT(indent_level, "Range:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Start:");
        ast_print_expr(arena, expr->as.range.start, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "End:");
        ast_print_expr(arena, expr->as.range.end, indent_level + 2);
        break;

    case EXPR_SPREAD:
        DEBUG_VERBOSE_INDENT(indent_level, "Spread:");
        ast_print_expr(arena, expr->as.spread.array, indent_level + 1);
        break;

    case EXPR_LAMBDA:
        DEBUG_VERBOSE_INDENT(indent_level, "Lambda (%d params):", expr->as.lambda.param_count);
        for (int i = 0; i < expr->as.lambda.param_count; i++)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Param: %.*s",
                                 expr->as.lambda.params[i].name.length,
                                 expr->as.lambda.params[i].name.start);
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_expr(arena, expr->as.lambda.body, indent_level + 2);
        break;

    case EXPR_STATIC_CALL:
        DEBUG_VERBOSE_INDENT(indent_level, "Static Call: %.*s.%.*s",
                             expr->as.static_call.type_name.length, expr->as.static_call.type_name.start,
                             expr->as.static_call.method_name.length, expr->as.static_call.method_name.start);
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Arg %d:", i);
            ast_print_expr(arena, expr->as.static_call.arguments[i], indent_level + 2);
        }
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        DEBUG_VERBOSE_INDENT(indent_level, "SizedArrayAlloc: %s[]",
                             ast_type_to_string(arena, expr->as.sized_array_alloc.element_type));
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Size:");
        ast_print_expr(arena, expr->as.sized_array_alloc.size_expr, indent_level + 2);
        if (expr->as.sized_array_alloc.default_value)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Default:");
            ast_print_expr(arena, expr->as.sized_array_alloc.default_value, indent_level + 2);
        }
        break;

    case EXPR_THREAD_SPAWN:
        DEBUG_VERBOSE_INDENT(indent_level, "ThreadSpawn:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Call:");
        ast_print_expr(arena, expr->as.thread_spawn.call, indent_level + 2);
        break;

    case EXPR_THREAD_SYNC:
        DEBUG_VERBOSE_INDENT(indent_level, "ThreadSync%s:",
                             expr->as.thread_sync.is_array ? " (sync list)" : "");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Handle:");
        ast_print_expr(arena, expr->as.thread_sync.handle, indent_level + 2);
        break;

    case EXPR_SYNC_LIST:
        DEBUG_VERBOSE_INDENT(indent_level, "SyncList (%d elements):",
                             expr->as.sync_list.element_count);
        for (int i = 0; i < expr->as.sync_list.element_count; i++)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "[%d]:", i);
            ast_print_expr(arena, expr->as.sync_list.elements[i], indent_level + 2);
        }
        break;

    case EXPR_AS_VAL:
        DEBUG_VERBOSE_INDENT(indent_level, "AsVal:");
        ast_print_expr(arena, expr->as.as_val.operand, indent_level + 1);
        break;

    case EXPR_AS_REF:
        DEBUG_VERBOSE_INDENT(indent_level, "AsRef:");
        ast_print_expr(arena, expr->as.as_ref.operand, indent_level + 1);
        break;
    }
}
