#ifndef AST_H
#define AST_H

#include "pool.h"

// 操作符类型枚举
typedef enum {
    OP_EQ,  // ==
    OP_NE,  // !=
    OP_GT,  // >
    OP_LT,  // <
    OP_GE,  // >=
    OP_LE,  // <=
    OP_AND, // &&
    OP_OR,   // ||
    OP_MINUS, // -
    OP_ADD,  // +
    OP_SUB,  // -
    OP_MUL,  // *
    OP_DIV,  // /
    OP_MOD,  // %
    OP_NOT,  // !
    OP_BAND, // &
    OP_BOR,  // |
    OP_BXOR, // ^
    OP_LSHIFT, // <<
    OP_RSHIFT  // >>
} operator_type_t;

// 返回类型枚举
typedef enum {
    RETURN_CONTINUE,
    RETURN_SKIP,
    RETURN_BLOCK
} return_type_t;

// AST 节点类型枚举
typedef enum {
    AST_PROGRAM,
    AST_GLOBAL,
    AST_NAMESPACE,
    AST_RULE,
    AST_STRUCT_MEMBER,
    AST_LET_STMT,
    AST_IF_STMT,
    AST_FOR_STMT,
    AST_WHILE_STMT,
    AST_RETURN_STMT,
    AST_ASSIGN_STMT,
    AST_FUNC_CALL,
    AST_MAP_ACCESS,
    AST_MEMBER_ACCESS,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_IDENTIFIER,
    AST_STRING_LITERAL,
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_ARRAY_LITERAL
} ast_node_type_t;

// 前向声明
typedef struct ast_node ast_node_t;
typedef struct ast_list ast_list_t;
typedef struct scope scope_t;
typedef struct symbol_entry symbol_entry_t;
typedef struct parser_context parser_context_t;

// 符号表条目
struct symbol_entry {
    char* name;
    char* type;
    scope_t* scope;
    symbol_entry_t* next;
};

// 作用域结构
struct scope {
    char* name;
    scope_t* parent;
    memory_pool_t* pool;
    symbol_entry_t* symbols;
};

// AST 列表结构
struct ast_list {
    ast_node_t* node;
    ast_list_t* next;
};

// AST 节点结构
struct ast_node {
    ast_node_type_t type;
    union {
        struct {
            ast_node_t* global;
            ast_list_t* namespaces;
        } program;
        
        struct {
            char* name;
            ast_list_t* members;
        } global;
        
        struct {
            char* name;
            ast_list_t* rules;
        } namespace;
        
        struct {
            char* name;
            ast_list_t* body;
            ast_list_t* after_rules;
            ast_list_t* before_rules;
        } rule;
        
        struct {
            char* name;
            char* type;
        } struct_member;
        
        struct {
            char* name;
            ast_node_t* init;
        } let_stmt;
        
        struct {
            ast_node_t* condition;
            ast_list_t* then_body;
            ast_list_t* else_body;
        } if_stmt;
        
        struct {
            char* iterator;
            ast_node_t* range;
            ast_list_t* body;
        } for_stmt;
        
        struct {
            ast_node_t* condition;
            ast_list_t* body;
        } while_stmt;
        
        struct {
            return_type_t type;
        } return_stmt;
        
        struct {
            ast_node_t* target;
            ast_node_t* value;
        } assign_stmt;
        
        struct {
            char* name;
            ast_list_t* args;
        } func_call;
        
        struct {
            ast_node_t* target;
            ast_node_t* key;
        } map_access;
        
        struct {
            ast_node_t* target;
            char* member;
        } member_access;
        
        struct {
            operator_type_t op;
            ast_node_t* left;
            ast_node_t* right;
        } binary_expr;

        struct {
            operator_type_t op;
            ast_node_t* operand;
        } unary_expr;
        
        struct {
            char* name;
        } identifier;
        
        struct {
            char* value;
        } string_literal;
        
        struct {
            int value;
        } integer_literal;
        
        struct {
            double value;
        } float_literal;
        
        struct {
            ast_list_t* items;
        } array_literal;
    } data;
};

// 解析器上下文
struct parser_context {
    memory_pool_t* pool;
    ast_node_t* root;
    scope_t* current_scope;
    symbol_entry_t* symbol_table;
    int error_count;
    char* current_file;
    int line_number;
};

// 函数声明
parser_context_t* create_parser_context(void);
void destroy_parser_context(parser_context_t* ctx);

scope_t* create_scope(parser_context_t* ctx, const char* name);
void push_scope(parser_context_t* ctx, scope_t* scope);
void pop_scope(parser_context_t* ctx);

symbol_entry_t* add_symbol(parser_context_t* ctx, const char* name, const char* type);
symbol_entry_t* find_symbol(parser_context_t* ctx, const char* name);

ast_node_t* create_ast_node(parser_context_t* ctx, ast_node_type_t type);
ast_list_t* create_ast_list(parser_context_t* ctx, ast_node_t* node);
void append_ast_list(parser_context_t* ctx, ast_list_t* list, ast_node_t* node);

ast_node_t* create_identifier_node(parser_context_t* ctx, const char* name);
ast_node_t* create_string_literal_node(parser_context_t* ctx, const char* value);
ast_node_t* create_integer_literal_node(parser_context_t* ctx, int value);
ast_node_t* create_float_literal_node(parser_context_t* ctx, double value);
ast_node_t* create_binary_expr_node(parser_context_t* ctx, operator_type_t op, 
                                   ast_node_t* left, ast_node_t* right);

void print_ast(const ast_node_t* root, int indent);

#endif // AST_H 