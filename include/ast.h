#ifndef AST_H
#define AST_H

typedef enum {
    // 节点类型
    PROGRAM_NODE, GLOBAL_BLOCK, NAMESPACE_NODE, RULE_NODE,
    ASSIGN_NODE, CONDITION_NODE, RETURN_NODE, BINOP_NODE,
    IDENT_NODE, LITERAL_NODE, SYSVAR_NODE, RULE_OPT_NODE
} ast_node_type;

typedef enum {
    // 值类型
    STRING_TYPE, ARRAY_TYPE, HOST_VAR, BLACKLIST_VAR
} value_type;

typedef enum {
    // 操作符类型
    EQ_OP, AND_OP, OR_OP
} operator_type;

typedef enum {
    // 规则顺序类型
    AFTER_OPT, BEFORE_OPT
} rule_opt_type;

typedef enum {
    // 返回类型
    BLOCK_RET, CONTINUE_RET
} return_type;

struct ast_node {
    ast_node_type type;
    int lineno;
    union {
        char* str_val;
        operator_type op;
        return_type ret;
        rule_opt_type opt;
        value_type val;
    };
    struct ast_node* children[4]; // 主要子节点
    struct ast_node* next;        // 链表结构
};

#endif
