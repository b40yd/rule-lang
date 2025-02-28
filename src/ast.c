#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pool.h"
#include "ast.h"

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

void print_ast(const ast_node_t* root, int indent) {
    if (!root) return;
    
    char indent_str[256] = {0};
    for (int i = 0; i < indent; i++) {
        strcat(indent_str, "  ");
    }
    
    switch (root->type) {
        case AST_PROGRAM:
            printf("%sProgram\n", indent_str);
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
            printf("%sGlobal: %s\n", indent_str, root->data.global.name);
            if (root->data.global.members) {
                ast_list_t* member = root->data.global.members;
                while (member) {
                    print_ast(member->node, indent + 1);
                    member = member->next;
                }
            }
            break;
            
        case AST_NAMESPACE:
            printf("%sNamespace: %s\n", indent_str, root->data.namespace.name);
            if (root->data.namespace.rules) {
                ast_list_t* rule = root->data.namespace.rules;
                while (rule) {
                    print_ast(rule->node, indent + 1);
                    rule = rule->next;
                }
            }
            break;
            
        case AST_RULE:
            printf("%sRule: %s\n", indent_str, root->data.rule.name);
            if (root->data.rule.after_rules) {
                printf("%s  After Rules:\n", indent_str);
                ast_list_t* after = root->data.rule.after_rules;
                while (after) {
                    print_ast(after->node, indent + 2);
                    after = after->next;
                }
            }
            if (root->data.rule.before_rules) {
                printf("%s  Before Rules:\n", indent_str);
                ast_list_t* before = root->data.rule.before_rules;
                while (before) {
                    print_ast(before->node, indent + 2);
                    before = before->next;
                }
            }
            if (root->data.rule.body) {
                printf("%s  Body:\n", indent_str);
                ast_list_t* stmt = root->data.rule.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;

        case AST_STRUCT_MEMBER:
            printf("%sStruct Member: %s (type: %s)\n", indent_str, 
                   root->data.struct_member.name, 
                   root->data.struct_member.type);
            break;
            
        case AST_LET_STMT:
            printf("%sLet: %s\n", indent_str, root->data.let_stmt.name);
            if (root->data.let_stmt.init) {
                print_ast(root->data.let_stmt.init, indent + 1);
            }
            break;
            
        case AST_IF_STMT:
            printf("%sIf\n", indent_str);
            if (root->data.if_stmt.condition) {
                printf("%s  Condition:\n", indent_str);
                print_ast(root->data.if_stmt.condition, indent + 2);
            }
            if (root->data.if_stmt.then_body) {
                printf("%s  Then:\n", indent_str);
                ast_list_t* stmt = root->data.if_stmt.then_body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            if (root->data.if_stmt.else_body) {
                printf("%s  Else:\n", indent_str);
                ast_list_t* stmt = root->data.if_stmt.else_body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_FOR_STMT:
            printf("%sFor: %s\n", indent_str, root->data.for_stmt.iterator);
            if (root->data.for_stmt.range) {
                printf("%s  Range:\n", indent_str);
                print_ast(root->data.for_stmt.range, indent + 2);
            }
            if (root->data.for_stmt.body) {
                printf("%s  Body:\n", indent_str);
                ast_list_t* stmt = root->data.for_stmt.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_WHILE_STMT:
            printf("%sWhile\n", indent_str);
            if (root->data.while_stmt.condition) {
                printf("%s  Condition:\n", indent_str);
                print_ast(root->data.while_stmt.condition, indent + 2);
            }
            if (root->data.while_stmt.body) {
                printf("%s  Body:\n", indent_str);
                ast_list_t* stmt = root->data.while_stmt.body;
                while (stmt) {
                    print_ast(stmt->node, indent + 2);
                    stmt = stmt->next;
                }
            }
            break;
            
        case AST_RETURN_STMT:
            printf("%sReturn: %s\n", indent_str, 
                root->data.return_stmt.type == RETURN_CONTINUE ? "continue" :
                root->data.return_stmt.type == RETURN_SKIP ? "skip" : "block");
            break;

        case AST_ASSIGN_STMT:
            printf("%sAssignment\n", indent_str);
            printf("%s  Target:\n", indent_str);
            print_ast(root->data.assign_stmt.target, indent + 2);
            printf("%s  Value:\n", indent_str);
            print_ast(root->data.assign_stmt.value, indent + 2);
            break;

        case AST_FUNC_CALL:
            printf("%sFunction Call: %s\n", indent_str, root->data.func_call.name);
            if (root->data.func_call.args) {
                printf("%s  Arguments:\n", indent_str);
                ast_list_t* arg = root->data.func_call.args;
                while (arg) {
                    print_ast(arg->node, indent + 2);
                    arg = arg->next;
                }
            }
            break;

        case AST_MAP_ACCESS:
            printf("%sMap Access\n", indent_str);
            printf("%s  Target:\n", indent_str);
            print_ast(root->data.map_access.target, indent + 2);
            printf("%s  Key:\n", indent_str);
            print_ast(root->data.map_access.key, indent + 2);
            break;

        case AST_MEMBER_ACCESS:
            printf("%sMember Access: %s\n", indent_str, root->data.member_access.member);
            printf("%s  Target:\n", indent_str);
            print_ast(root->data.member_access.target, indent + 2);
            break;
            
        case AST_BINARY_EXPR:
            printf("%sBinary Expression\n", indent_str);
            if (root->data.binary_expr.left) {
                printf("%s  Left:\n", indent_str);
                print_ast(root->data.binary_expr.left, indent + 2);
            }
            if (root->data.binary_expr.right) {
                printf("%s  Right:\n", indent_str);
                print_ast(root->data.binary_expr.right, indent + 2);
            }
            break;
            
        case AST_IDENTIFIER:
            printf("%sIdentifier: %s\n", indent_str, root->data.identifier.name);
            break;
            
        case AST_STRING_LITERAL:
            printf("%sString: %s\n", indent_str, root->data.string_literal.value);
            break;
            
        case AST_INTEGER_LITERAL:
            printf("%sInteger: %d\n", indent_str, root->data.integer_literal.value);
            break;
            
        case AST_FLOAT_LITERAL:
            printf("%sFloat: %f\n", indent_str, root->data.float_literal.value);
            break;
            
        case AST_ARRAY_LITERAL:
            printf("%sArray\n", indent_str);
            if (root->data.array_literal.items) {
                ast_list_t* item = root->data.array_literal.items;
                while (item) {
                    print_ast(item->node, indent + 1);
                    item = item->next;
                }
            }
            break;
            
        default:
            printf("%sUnknown Node Type: %d\n", indent_str, root->type);
            break;
    }
} 