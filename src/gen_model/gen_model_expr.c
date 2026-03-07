#include "gen_model/gen_model.h"
#include <string.h>

static const char *binary_op_str(SnTokenType op)
{
    switch (op)
    {
        case TOKEN_PLUS:           return "add";
        case TOKEN_MINUS:          return "subtract";
        case TOKEN_STAR:           return "multiply";
        case TOKEN_SLASH:          return "divide";
        case TOKEN_MODULO:         return "modulo";
        case TOKEN_EQUAL_EQUAL:    return "eq";
        case TOKEN_BANG_EQUAL:     return "neq";
        case TOKEN_LESS:           return "lt";
        case TOKEN_LESS_EQUAL:     return "lte";
        case TOKEN_GREATER:        return "gt";
        case TOKEN_GREATER_EQUAL:  return "gte";
        case TOKEN_AND:            return "and";
        case TOKEN_OR:             return "or";
        case TOKEN_AMPERSAND:      return "bitand";
        case TOKEN_PIPE:           return "bitor";
        case TOKEN_CARET:          return "bitxor";
        case TOKEN_LSHIFT:         return "shl";
        case TOKEN_RSHIFT:         return "shr";
        default:                   return "unknown";
    }
}

static const char *unary_op_str(SnTokenType op)
{
    switch (op)
    {
        case TOKEN_MINUS: return "negate";
        case TOKEN_BANG:  return "not";
        case TOKEN_TILDE: return "bitnot";
        default:          return "unknown";
    }
}

static const char *func_mod_str(FunctionModifier fm)
{
    switch (fm)
    {
        case FUNC_DEFAULT: return "default";
        default:           return "default";
    }
}

json_object *gen_model_expr(Arena *arena, Expr *expr, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode)
{
    (void)symbol_table;

    if (!expr) return json_object_new_null();

    json_object *obj = json_object_new_object();

    /* Add type info if available */
    if (expr->expr_type)
    {
        json_object_object_add(obj, "type", gen_model_type(arena, expr->expr_type));
    }

    switch (expr->type)
    {
        case EXPR_LITERAL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("literal"));
            LiteralExpr *lit = &expr->as.literal;
            if (lit->type)
            {
                switch (lit->type->kind)
                {
                    case TYPE_INT:
                    case TYPE_INT32:
                    case TYPE_UINT:
                    case TYPE_UINT32:
                    case TYPE_LONG:
                        json_object_object_add(obj, "value_kind", json_object_new_string("int"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_DOUBLE:
                    case TYPE_FLOAT:
                        json_object_object_add(obj, "value_kind", json_object_new_string("double"));
                        json_object_object_add(obj, "value",
                            json_object_new_double(lit->value.double_value));
                        break;
                    case TYPE_STRING:
                        json_object_object_add(obj, "value_kind", json_object_new_string("string"));
                        json_object_object_add(obj, "value",
                            json_object_new_string(lit->value.string_value ? lit->value.string_value : ""));
                        break;
                    case TYPE_BOOL:
                        json_object_object_add(obj, "value_kind", json_object_new_string("bool"));
                        json_object_object_add(obj, "value",
                            json_object_new_boolean(lit->value.int_value != 0));
                        break;
                    case TYPE_CHAR:
                        json_object_object_add(obj, "value_kind", json_object_new_string("char"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_BYTE:
                        json_object_object_add(obj, "value_kind", json_object_new_string("byte"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_NIL:
                        json_object_object_add(obj, "value_kind", json_object_new_string("nil"));
                        break;
                    default:
                        json_object_object_add(obj, "value_kind", json_object_new_string("unknown"));
                        break;
                }
            }
            break;
        }

        case EXPR_VARIABLE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("variable"));
            json_object_object_add(obj, "name",
                json_object_new_string(expr->as.variable.name.start));
            break;
        }

        case EXPR_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("assign"));
            json_object_object_add(obj, "target",
                json_object_new_string(expr->as.assign.name.start));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.assign.value, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_COMPOUND_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("compound_assign"));
            json_object_object_add(obj, "op",
                json_object_new_string(binary_op_str(expr->as.compound_assign.operator)));
            json_object_object_add(obj, "target",
                gen_model_expr(arena, expr->as.compound_assign.target, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.compound_assign.value, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_BINARY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("binary"));
            json_object_object_add(obj, "op",
                json_object_new_string(binary_op_str(expr->as.binary.operator)));
            json_object_object_add(obj, "left",
                gen_model_expr(arena, expr->as.binary.left, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "right",
                gen_model_expr(arena, expr->as.binary.right, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "arithmetic_mode",
                json_object_new_string(arithmetic_mode == ARITH_CHECKED ? "checked" : "unchecked"));
            break;
        }

        case EXPR_UNARY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("unary"));
            json_object_object_add(obj, "op",
                json_object_new_string(unary_op_str(expr->as.unary.operator)));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.unary.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("call"));
            json_object_object_add(obj, "callee",
                gen_model_expr(arena, expr->as.call.callee, symbol_table, arithmetic_mode));
            json_object *args = json_object_new_array();
            for (int i = 0; i < expr->as.call.arg_count; i++)
            {
                json_object_array_add(args,
                    gen_model_expr(arena, expr->as.call.arguments[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "args", args);
            json_object_object_add(obj, "is_tail_call",
                json_object_new_boolean(expr->as.call.is_tail_call));
            break;
        }

        case EXPR_METHOD_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("method_call"));
            if (expr->as.method_call.object)
            {
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.method_call.object, symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "method_name",
                json_object_new_string(expr->as.method_call.method_name.start));
            json_object_object_add(obj, "is_static",
                json_object_new_boolean(expr->as.method_call.is_static));
            if (expr->as.method_call.struct_type)
            {
                json_object_object_add(obj, "struct_type",
                    gen_model_type(arena, expr->as.method_call.struct_type));
            }
            json_object *args = json_object_new_array();
            for (int i = 0; i < expr->as.method_call.arg_count; i++)
            {
                json_object_array_add(args,
                    gen_model_expr(arena, expr->as.method_call.args[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "args", args);
            break;
        }

        case EXPR_STATIC_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("static_call"));
            json_object_object_add(obj, "type_name",
                json_object_new_string(expr->as.static_call.type_name.start));
            json_object_object_add(obj, "method_name",
                json_object_new_string(expr->as.static_call.method_name.start));
            json_object *args = json_object_new_array();
            for (int i = 0; i < expr->as.static_call.arg_count; i++)
            {
                json_object_array_add(args,
                    gen_model_expr(arena, expr->as.static_call.arguments[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "args", args);
            break;
        }

        case EXPR_ARRAY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_literal"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.array.element_count; i++)
            {
                json_object_array_add(elements,
                    gen_model_expr(arena, expr->as.array.elements[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "elements", elements);
            break;
        }

        case EXPR_ARRAY_ACCESS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_access"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.array_access.array, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "index",
                gen_model_expr(arena, expr->as.array_access.index, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_ARRAY_SLICE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_slice"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.array_slice.array, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.start)
                json_object_object_add(obj, "start",
                    gen_model_expr(arena, expr->as.array_slice.start, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.end)
                json_object_object_add(obj, "end",
                    gen_model_expr(arena, expr->as.array_slice.end, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.step)
                json_object_object_add(obj, "step",
                    gen_model_expr(arena, expr->as.array_slice.step, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_INDEX_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("index_assign"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.index_assign.array, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "index",
                gen_model_expr(arena, expr->as.index_assign.index, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.index_assign.value, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_MEMBER:
        {
            json_object_object_add(obj, "kind", json_object_new_string("member"));
            json_object_object_add(obj, "object",
                gen_model_expr(arena, expr->as.member.object, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "member_name",
                json_object_new_string(expr->as.member.member_name.start));
            break;
        }

        case EXPR_MEMBER_ACCESS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("member_access"));
            json_object_object_add(obj, "object",
                gen_model_expr(arena, expr->as.member_access.object, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "field_name",
                json_object_new_string(expr->as.member_access.field_name.start));
            json_object_object_add(obj, "field_index",
                json_object_new_int(expr->as.member_access.field_index));
            break;
        }

        case EXPR_MEMBER_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("member_assign"));
            json_object_object_add(obj, "object",
                gen_model_expr(arena, expr->as.member_assign.object, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "field_name",
                json_object_new_string(expr->as.member_assign.field_name.start));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.member_assign.value, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_STRUCT_LITERAL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("struct_literal"));
            json_object_object_add(obj, "struct_name",
                json_object_new_string(expr->as.struct_literal.struct_name.start));
            json_object *fields = json_object_new_array();
            for (int i = 0; i < expr->as.struct_literal.field_count; i++)
            {
                json_object *f = json_object_new_object();
                json_object_object_add(f, "name",
                    json_object_new_string(expr->as.struct_literal.fields[i].name.start));
                json_object_object_add(f, "value",
                    gen_model_expr(arena, expr->as.struct_literal.fields[i].value, symbol_table, arithmetic_mode));
                json_object_array_add(fields, f);
            }
            json_object_object_add(obj, "fields", fields);
            break;
        }

        case EXPR_INTERPOLATED:
        {
            json_object_object_add(obj, "kind", json_object_new_string("interpolated_string"));
            json_object *parts = json_object_new_array();
            for (int i = 0; i < expr->as.interpol.part_count; i++)
            {
                json_object *part = json_object_new_object();
                Expr *p = expr->as.interpol.parts[i];
                if (p->type == EXPR_LITERAL && p->as.literal.type &&
                    p->as.literal.type->kind == TYPE_STRING)
                {
                    json_object_object_add(part, "kind", json_object_new_string("text"));
                    json_object_object_add(part, "value",
                        json_object_new_string(p->as.literal.value.string_value ? p->as.literal.value.string_value : ""));
                }
                else
                {
                    json_object_object_add(part, "kind", json_object_new_string("expr"));
                    json_object_object_add(part, "expr",
                        gen_model_expr(arena, p, symbol_table, arithmetic_mode));
                }
                if (expr->as.interpol.format_specs && expr->as.interpol.format_specs[i])
                {
                    json_object_object_add(part, "format_spec",
                        json_object_new_string(expr->as.interpol.format_specs[i]));
                }
                json_object_array_add(parts, part);
            }
            json_object_object_add(obj, "parts", parts);
            break;
        }

        case EXPR_INCREMENT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("increment"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_DECREMENT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("decrement"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_RANGE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("range"));
            json_object_object_add(obj, "start",
                gen_model_expr(arena, expr->as.range.start, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "end",
                gen_model_expr(arena, expr->as.range.end, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_SPREAD:
        {
            json_object_object_add(obj, "kind", json_object_new_string("spread"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.spread.array, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_SIZED_ARRAY_ALLOC:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sized_array"));
            json_object_object_add(obj, "element_type",
                gen_model_type(arena, expr->as.sized_array_alloc.element_type));
            json_object_object_add(obj, "size",
                gen_model_expr(arena, expr->as.sized_array_alloc.size_expr, symbol_table, arithmetic_mode));
            if (expr->as.sized_array_alloc.default_value)
            {
                json_object_object_add(obj, "default_value",
                    gen_model_expr(arena, expr->as.sized_array_alloc.default_value, symbol_table, arithmetic_mode));
            }
            break;
        }

        case EXPR_LAMBDA:
        {
            json_object_object_add(obj, "kind", json_object_new_string("lambda"));
            LambdaExpr *lam = &expr->as.lambda;
            json_object_object_add(obj, "lambda_id", json_object_new_int(lam->lambda_id));
            json_object_object_add(obj, "return_type", gen_model_type(arena, lam->return_type));
            json_object_object_add(obj, "modifier", json_object_new_string(func_mod_str(lam->modifier)));
            json_object_object_add(obj, "is_native", json_object_new_boolean(lam->is_native));

            json_object *params = json_object_new_array();
            for (int i = 0; i < lam->param_count; i++)
            {
                json_object *p = json_object_new_object();
                json_object_object_add(p, "name",
                    json_object_new_string(lam->params[i].name.start));
                json_object_object_add(p, "type",
                    gen_model_type(arena, lam->params[i].type));
                json_object_array_add(params, p);
            }
            json_object_object_add(obj, "params", params);

            /* Captures */
            json_object *captures = json_object_new_array();
            for (int i = 0; i < lam->capture_count; i++)
            {
                json_object *cap = json_object_new_object();
                json_object_object_add(cap, "name",
                    json_object_new_string(lam->captured_vars[i].start));
                json_object_object_add(cap, "type",
                    gen_model_type(arena, lam->captured_types[i]));
                json_object_array_add(captures, cap);
            }
            json_object_object_add(obj, "captures", captures);

            /* Body */
            if (lam->has_stmt_body)
            {
                json_object *body = json_object_new_array();
                for (int i = 0; i < lam->body_stmt_count; i++)
                {
                    json_object_array_add(body,
                        gen_model_stmt(arena, lam->body_stmts[i], symbol_table, arithmetic_mode));
                }
                json_object_object_add(obj, "body_stmts", body);
            }
            else if (lam->body)
            {
                json_object_object_add(obj, "body",
                    gen_model_expr(arena, lam->body, symbol_table, arithmetic_mode));
            }
            break;
        }

        case EXPR_MATCH:
        {
            json_object_object_add(obj, "kind", json_object_new_string("match"));
            json_object_object_add(obj, "subject",
                gen_model_expr(arena, expr->as.match_expr.subject, symbol_table, arithmetic_mode));
            json_object *arms = json_object_new_array();
            for (int i = 0; i < expr->as.match_expr.arm_count; i++)
            {
                MatchArm *arm = &expr->as.match_expr.arms[i];
                json_object *a = json_object_new_object();
                json_object_object_add(a, "is_else", json_object_new_boolean(arm->is_else));
                json_object *patterns = json_object_new_array();
                for (int j = 0; j < arm->pattern_count; j++)
                {
                    json_object_array_add(patterns,
                        gen_model_expr(arena, arm->patterns[j], symbol_table, arithmetic_mode));
                }
                json_object_object_add(a, "patterns", patterns);
                if (arm->body)
                {
                    json_object_object_add(a, "body",
                        gen_model_stmt(arena, arm->body, symbol_table, arithmetic_mode));
                }
                json_object_array_add(arms, a);
            }
            json_object_object_add(obj, "arms", arms);
            break;
        }

        case EXPR_THREAD_SPAWN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("thread_spawn"));
            json_object_object_add(obj, "call",
                gen_model_expr(arena, expr->as.thread_spawn.call, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "modifier",
                json_object_new_string(func_mod_str(expr->as.thread_spawn.modifier)));
            break;
        }

        case EXPR_THREAD_SYNC:
        {
            json_object_object_add(obj, "kind", json_object_new_string("thread_sync"));
            json_object_object_add(obj, "handle",
                gen_model_expr(arena, expr->as.thread_sync.handle, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "is_array",
                json_object_new_boolean(expr->as.thread_sync.is_array));
            break;
        }

        case EXPR_SYNC_LIST:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sync_list"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.sync_list.element_count; i++)
            {
                json_object_array_add(elements,
                    gen_model_expr(arena, expr->as.sync_list.elements[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "elements", elements);
            break;
        }

        case EXPR_ADDRESS_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("address_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.address_of.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_VALUE_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("value_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.value_of.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_COPY_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("copy_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.copy_of.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_TYPEOF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("typeof"));
            if (expr->as.typeof_expr.operand)
            {
                json_object_object_add(obj, "operand",
                    gen_model_expr(arena, expr->as.typeof_expr.operand, symbol_table, arithmetic_mode));
            }
            if (expr->as.typeof_expr.type_literal)
            {
                json_object_object_add(obj, "type_literal",
                    gen_model_type(arena, expr->as.typeof_expr.type_literal));
            }
            break;
        }

        case EXPR_IS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("is"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.is_expr.operand, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "check_type",
                gen_model_type(arena, expr->as.is_expr.check_type));
            break;
        }

        case EXPR_AS_TYPE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("as_type"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.as_type.operand, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "target_type",
                gen_model_type(arena, expr->as.as_type.target_type));
            break;
        }

        case EXPR_SIZEOF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sizeof"));
            if (expr->as.sizeof_expr.type_operand)
            {
                json_object_object_add(obj, "target_type",
                    gen_model_type(arena, expr->as.sizeof_expr.type_operand));
            }
            if (expr->as.sizeof_expr.expr_operand)
            {
                json_object_object_add(obj, "operand",
                    gen_model_expr(arena, expr->as.sizeof_expr.expr_operand, symbol_table, arithmetic_mode));
            }
            break;
        }
    }

    /* Escape info */
    if (expr->escape_info.escapes_scope || expr->escape_info.needs_heap_allocation)
    {
        json_object *esc = json_object_new_object();
        json_object_object_add(esc, "escapes_scope",
            json_object_new_boolean(expr->escape_info.escapes_scope));
        json_object_object_add(esc, "needs_heap",
            json_object_new_boolean(expr->escape_info.needs_heap_allocation));
        json_object_object_add(obj, "escape_info", esc);
    }

    return obj;
}
