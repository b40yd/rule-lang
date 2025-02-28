#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pool.h"
#include "ast.h"

// 颜色代码
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

parser_context_t* create_parser_context(void) {
    memory_pool_t* pool = create_pool(POOL_SIZE);
    if (!pool) return NULL;

    parser_context_t* ctx = palloc(pool, sizeof(parser_context_t));
    if (!ctx) {
        destroy_pool(pool);
        return NULL;
    }

    ctx->pool = pool;
    ctx->root = NULL;
    ctx->current_scope = NULL;
    ctx->symbol_table = NULL;
    ctx->error_count = 0;
    ctx->current_file = NULL;
    ctx->line_number = 1;

    return ctx;
}

void destroy_parser_context(parser_context_t* ctx) {
    if (ctx) {
        destroy_pool(ctx->pool);
    }
}

scope_t* create_scope(parser_context_t* ctx, const char* name) {
    scope_t* scope = palloc(ctx->pool, sizeof(scope_t));
    if (!scope) return NULL;

    scope->name = pstrdup(ctx->pool, name);
    scope->parent = NULL;
    scope->pool = create_pool(POOL_SIZE);
    scope->symbols = NULL;

    return scope;
}

void push_scope(parser_context_t* ctx, scope_t* scope) {
    scope->parent = ctx->current_scope;
    ctx->current_scope = scope;
}

void pop_scope(parser_context_t* ctx) {
    if (ctx->current_scope) {
        scope_t* old_scope = ctx->current_scope;
        ctx->current_scope = old_scope->parent;
        destroy_pool(old_scope->pool);
    }
}

symbol_entry_t* add_symbol(parser_context_t* ctx, const char* name, const char* type) {
    memory_pool_t* pool = ctx->current_scope ? ctx->current_scope->pool : ctx->pool;
    symbol_entry_t* symbol = palloc(pool, sizeof(symbol_entry_t));
    
    symbol->name = pstrdup(pool, name);
    symbol->type = pstrdup(pool, type);
    symbol->scope = ctx->current_scope;
    
    symbol->next = ctx->symbol_table;
    ctx->symbol_table = symbol;
    
    return symbol;
}

symbol_entry_t* find_symbol(parser_context_t* ctx, const char* name) {
    for (scope_t* scope = ctx->current_scope; scope; scope = scope->parent) {
        for (symbol_entry_t* sym = scope->symbols; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
        }
    }
    return NULL;
}

ast_node_t* create_ast_node(parser_context_t* ctx, ast_node_type_t type) {
    ast_node_t* node = palloc(ctx->pool, sizeof(ast_node_t));
    if (node) {
        node->type = type;
        memset(&node->data, 0, sizeof(node->data));
    }
    return node;
}

ast_list_t* create_ast_list(parser_context_t* ctx, ast_node_t* node) {
    ast_list_t* list = palloc(ctx->pool, sizeof(ast_list_t));
    if (list) {
        list->node = node;
        list->next = NULL;
    }
    return list;
}

void append_ast_list(parser_context_t* ctx, ast_list_t* list, ast_node_t* node) {
    if (!list) return;
    
    while (list->next) {
        list = list->next;
    }
    list->next = create_ast_list(ctx, node);
}

ast_node_t* create_identifier_node(parser_context_t* ctx, const char* name) {
    ast_node_t* node = create_ast_node(ctx, AST_IDENTIFIER);
    if (node) {
        node->data.identifier.name = pstrdup(ctx->pool, name);
    }
    return node;
}

ast_node_t* create_string_literal_node(parser_context_t* ctx, const char* value) {
    ast_node_t* node = create_ast_node(ctx, AST_STRING_LITERAL);
    if (node) {
        node->data.string_literal.value = pstrdup(ctx->pool, value);
    }
    return node;
}

ast_node_t* create_integer_literal_node(parser_context_t* ctx, int value) {
    ast_node_t* node = create_ast_node(ctx, AST_INTEGER_LITERAL);
    if (node) {
        node->data.integer_literal.value = value;
    }
    return node;
}

ast_node_t* create_float_literal_node(parser_context_t* ctx, double value) {
    ast_node_t* node = create_ast_node(ctx, AST_FLOAT_LITERAL);
    if (node) {
        node->data.float_literal.value = value;
    }
    return node;
}

ast_node_t* create_binary_expr_node(parser_context_t* ctx, operator_type_t op, 
                                   ast_node_t* left, ast_node_t* right) {
    ast_node_t* node = create_ast_node(ctx, AST_BINARY_EXPR);
    if (node) {
        node->data.binary_expr.op = op;
        node->data.binary_expr.left = left;
        node->data.binary_expr.right = right;
    }
    return node;
}

// 运算符字符串表
static const char* operator_to_string(operator_type_t op) {
    switch (op) {
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_GT: return ">";
        case OP_LT: return "<";
        case OP_GE: return ">=";
        case OP_LE: return "<=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_MINUS: return "-";
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_NOT: return "!";
        case OP_BAND: return "&";
        case OP_BOR: return "|";
        case OP_BXOR: return "^";
        case OP_LSHIFT: return "<<";
        case OP_RSHIFT: return ">>";
        case OP_INC: return "++";
        case OP_DEC: return "--";
        case OP_ADD_ASSIGN: return "+=";
        case OP_SUB_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";
        case OP_DIV_ASSIGN: return "/=";
        case OP_MOD_ASSIGN: return "%=";
        case OP_BAND_ASSIGN: return "&=";
        case OP_BOR_ASSIGN: return "|=";
        case OP_BXOR_ASSIGN: return "^=";
        case OP_LSHIFT_ASSIGN: return "<<=";
        case OP_RSHIFT_ASSIGN: return ">>=";
        default: return "unknown";
    }
}

void print_ast(const ast_node_t* root, int indent) {
    if (!root) return;
    
    char indent_str[256] = {0};
    for (int i = 0; i < indent; i++) {
        strcat(indent_str, "  ");
    }
    
    switch (root->type) {
        case AST_PROGRAM:
            printf("%s%s┌── Program%s\n", indent_str, COLOR_BLUE, COLOR_RESET);
            if (root->data.program.global) {
                print_ast(root->data.program.global, indent + 1);
            }
            if (root->data.program.namespaces) {
                ast_list_t* ns = root->data.program.namespaces;
                while (ns) {
                    print_ast(ns->node, indent + 1);
                    ns = ns->next;
                }
            }
            break;
            
        case AST_GLOBAL:
            printf("%s%s├── Global: %s%s\n", indent_str, COLOR_GREEN, 
                   root->data.global.name, COLOR_RESET);
            if (root->data.global.members) {
                ast_list_t* member = root->data.global.members;
                while (member) {
                    print_ast(member->node, indent + 1);
                    member = member->next;
                }
            }
            break;
            
        case AST_NAMESPACE:
            printf("%s%s├── Namespace: %s%s\n", indent_str, COLOR_YELLOW, 
                   root->data.namespace.name, COLOR_RESET);
            if (root->data.namespace.rules) {
                ast_list_t* rule = root->data.namespace.rules;
                while (rule) {
                    print_ast(rule->node, indent + 1);
                    rule = rule->next;
                }
            }
            break;
            
        case AST_RULE:
            printf("%s%s├── Rule: %s%s\n", indent_str, COLOR_MAGENTA, 
                   root->data.rule.name, COLOR_RESET);
            if (root->data.rule.body) {
                printf("%s  %s└── Body:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* stmt = root->data.rule.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;

        case AST_STRUCT_MEMBER:
            printf("%s%s├── Member: %s (type: %s)%s\n", indent_str, COLOR_GREEN,
                   root->data.struct_member.name, 
                   root->data.struct_member.type,
                   COLOR_RESET);
            break;
            
        case AST_LET_STMT:
            printf("%s%s├── Let: %s%s\n", indent_str, COLOR_CYAN,
                   root->data.let_stmt.name, COLOR_RESET);
            if (root->data.let_stmt.init) {
                print_ast(root->data.let_stmt.init, indent + 1);
            }
            break;
            
        case AST_IF_STMT:
            printf("%s%s├── If%s\n", indent_str, COLOR_YELLOW, COLOR_RESET);
            if (root->data.if_stmt.condition) {
                printf("%s  %s├── Condition:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                print_ast(root->data.if_stmt.condition, indent + 2);
            }
            if (root->data.if_stmt.then_body) {
                printf("%s  %s├── Then:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* stmt = root->data.if_stmt.then_body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            if (root->data.if_stmt.else_body) {
                printf("%s  %s└── Else:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* stmt = root->data.if_stmt.else_body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_FOR_STMT:
            printf("%s%s├── For: %s%s\n", indent_str, COLOR_YELLOW,
                   root->data.for_stmt.iterator, COLOR_RESET);
            if (root->data.for_stmt.range) {
                printf("%s  %s├── Range:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                print_ast(root->data.for_stmt.range, indent + 2);
            }
            if (root->data.for_stmt.body) {
                printf("%s  %s└── Body:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* stmt = root->data.for_stmt.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_WHILE_STMT:
            printf("%s%s├── While%s\n", indent_str, COLOR_YELLOW, COLOR_RESET);
            if (root->data.while_stmt.condition) {
                printf("%s  %s├── Condition:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                print_ast(root->data.while_stmt.condition, indent + 2);
            }
            if (root->data.while_stmt.body) {
                printf("%s  %s└── Body:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* stmt = root->data.while_stmt.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_RETURN_STMT:
            printf("%s%s└── Return: %s%s\n", indent_str, COLOR_RED,
                   root->data.return_stmt.type == RETURN_CONTINUE ? "continue" :
                   root->data.return_stmt.type == RETURN_SKIP ? "skip" : "block",
                   COLOR_RESET);
            break;

        case AST_ASSIGN_STMT:
            printf("%s%s├── Assignment%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            printf("%s  %s├── Target:%s\n", indent_str, COLOR_MAGENTA, COLOR_RESET);
            print_ast(root->data.assign_stmt.target, indent + 2);
            printf("%s  %s└── Value:%s\n", indent_str, COLOR_MAGENTA, COLOR_RESET);
            print_ast(root->data.assign_stmt.value, indent + 2);
            break;

        case AST_FUNC_CALL:
            printf("%s%s├── Call: %s%s\n", indent_str, COLOR_BLUE,
                   root->data.func_call.name, COLOR_RESET);
            if (root->data.func_call.args) {
                printf("%s  %s└── Args:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
                ast_list_t* arg = root->data.func_call.args;
                while (arg) {
                    print_ast(arg->node, indent + 2);
                    arg = arg->next;
                }
            }
            break;

        case AST_MAP_ACCESS:
            printf("%s%s├── Map Access%s\n", indent_str, COLOR_YELLOW, COLOR_RESET);
            printf("%s  %s├── Target:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.map_access.target, indent + 2);
            printf("%s  %s└── Key:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.map_access.key, indent + 2);
            break;

        case AST_MEMBER_ACCESS:
            printf("%s%s├── Member: %s%s\n", indent_str, COLOR_YELLOW,
                   root->data.member_access.member, COLOR_RESET);
            printf("%s  %s└── Target:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.member_access.target, indent + 2);
            break;
            
        case AST_BINARY_EXPR:
            printf("%s%s├── Binary: %s%s\n", indent_str, COLOR_MAGENTA,
                   operator_to_string(root->data.binary_expr.op), COLOR_RESET);
            printf("%s  %s├── Left:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.binary_expr.left, indent + 2);
            printf("%s  %s└── Right:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.binary_expr.right, indent + 2);
            break;

        case AST_UNARY_EXPR:
            printf("%s%s├── Unary: %s%s\n", indent_str, COLOR_MAGENTA,
                   operator_to_string(root->data.unary_expr.op), COLOR_RESET);
            printf("%s  %s└── Operand:%s\n", indent_str, COLOR_CYAN, COLOR_RESET);
            print_ast(root->data.unary_expr.operand, indent + 2);
            break;
            
        case AST_IDENTIFIER:
            printf("%s%s└── ID: %s%s\n", indent_str, COLOR_GREEN,
                   root->data.identifier.name, COLOR_RESET);
            break;
            
        case AST_STRING_LITERAL:
            printf("%s%s└── String: \"%s\"%s\n", indent_str, COLOR_GREEN,
                   root->data.string_literal.value, COLOR_RESET);
            break;
            
        case AST_INTEGER_LITERAL:
            printf("%s%s└── Int: %d%s\n", indent_str, COLOR_GREEN,
                   root->data.integer_literal.value, COLOR_RESET);
            break;
            
        case AST_FLOAT_LITERAL:
            printf("%s%s└── Float: %f%s\n", indent_str, COLOR_GREEN,
                   root->data.float_literal.value, COLOR_RESET);
            break;
            
        case AST_ARRAY_LITERAL:
            printf("%s%s├── Array%s\n", indent_str, COLOR_YELLOW, COLOR_RESET);
            if (root->data.array_literal.items) {
                ast_list_t* item = root->data.array_literal.items;
                while (item) {
                    print_ast(item->node, indent + 1);
                    item = item->next;
                }
            }
            break;
            
        default:
            printf("%s%s└── Unknown Node Type: %d%s\n", indent_str, COLOR_RED,
                   root->type, COLOR_RESET);
            break;
    }
} 