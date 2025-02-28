%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "pool.h"

extern int yylex(void);
extern int yylineno;
extern char* yytext;

void yyerror(parser_context_t* ctx, const char* s) {
    fprintf(stderr, "Error at line %d: %s near '%s'\n", yylineno, s, yytext);
}

int yyparse(parser_context_t* ctx);

%}

%parse-param { parser_context_t* ctx }

%union {
    int int_val;
    double float_val;
    char* str_val;
    ast_node_t* node;
    ast_list_t* list;
    operator_type_t op;
}

%token <str_val> IDENTIFIER STRING_LITERAL
%token <int_val> INTEGER_LITERAL
%token <float_val> FLOAT_LITERAL
%token GLOBAL NAMESPACE RULE IF ELSE LET RETURN CONTINUE SKIP BLOCK
%token AFTER BEFORE FOR RANGE IN MAP NIL WHILE THEN
%token STRING_TYPE INT_TYPE FLOAT_TYPE ARRAY_TYPE
%token EQ NE GE LE GT LT AND OR NOT BAND BOR BXOR LSHIFT RSHIFT
%token MATCH_KEYWORD MATCH_KEYWORD_VALUE

%type <node> program global_section namespace_section rule_declaration namespace_item
%type <node> struct_member expression primary_expression unary_expression
%type <node> let_statement if_statement for_statement while_statement
%type <node> return_statement assignment_statement function_call
%type <node> array_literal rule_statement
%type <list> namespace_sections namespace_items_list rule_statements namespace_items
%type <list> struct_members array_items identifier_list
%type <str_val> rule_name type_spec basic_type map_type array_type
%type <op> comparison_operator

%right '='
%left OR
%left AND
%left BOR
%left BXOR
%left BAND
%left EQ NE
%left GT LT GE LE
%left LSHIFT RSHIFT
%left '+' '-'
%left '*' '/' '%'
%right UMINUS NOT
%left '.' '[' ']'
%left '('

%%

program
    : global_section namespace_sections
    {
        ast_node_t* node = create_ast_node(ctx, AST_PROGRAM);
        node->data.program.global = $1;
        node->data.program.namespaces = $2;
        ctx->root = node;
        $$ = node;
    }
    ;

global_section
    : GLOBAL IDENTIFIER '{' struct_members '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_GLOBAL);
        node->data.global.name = $2;
        node->data.global.members = $4;
        $$ = node;
        add_symbol(ctx, $2, "struct");
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

struct_members
    : struct_members struct_member
    {
        $$ = $1;
        append_ast_list(ctx, $$, $2);
    }
    | struct_member
    {
        $$ = create_ast_list(ctx, $1);
    }
    ;

struct_member
    : IDENTIFIER type_spec
    {
        ast_node_t* node = create_ast_node(ctx, AST_STRUCT_MEMBER);
        node->data.struct_member.name = $1;
        node->data.struct_member.type = $2;
        $$ = node;
        add_symbol(ctx, $1, $2);
    }
    ;

type_spec
    : basic_type { $$ = $1; }
    | map_type { $$ = $1; }
    | array_type { $$ = $1; }
    ;

basic_type
    : STRING_TYPE { $$ = strdup("string"); }
    | INT_TYPE { $$ = strdup("int"); }
    | FLOAT_TYPE { $$ = strdup("float"); }
    ;

map_type
    : MAP '[' type_spec ']' type_spec
    {
        char* type = malloc(strlen($3) + strlen($5) + 16);
        sprintf(type, "map[%s]%s", $3, $5);
        free($3);
        free($5);
        $$ = type;
    }
    ;

array_type
    : ARRAY_TYPE '[' type_spec ']'
    {
        char* type = malloc(strlen($3) + 16);
        sprintf(type, "array[%s]", $3);
        free($3);
        $$ = type;
    }
    ;

namespace_sections
    : namespace_sections namespace_section
    {
        $$ = $1;
        append_ast_list(ctx, $$, $2);
    }
    | namespace_section
    {
        $$ = create_ast_list(ctx, $1);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

namespace_section
    : NAMESPACE IDENTIFIER '{'
    {
        scope_t* scope = create_scope(ctx, $2);
        push_scope(ctx, scope);
    }
    namespace_items_list '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_NAMESPACE);
        node->data.namespace.name = $2;
        node->data.namespace.rules = $5;
        $$ = node;
        pop_scope(ctx);
    }
    ;

namespace_items_list
    : namespace_items
    {
        $$ = $1;
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

namespace_items
    : namespace_items namespace_item
    {
        $$ = $1;
        if ($2) {
            append_ast_list(ctx, $$, $2);
        }
    }
    | namespace_item
    {
        if ($1) {
            $$ = create_ast_list(ctx, $1);
        } else {
            $$ = NULL;
        }
    }
    ;

namespace_item
    : rule_declaration
    {
        $$ = $1;
    }
    | error ';'  /* 错误恢复 */
    {
        $$ = NULL;
    }
    ;

rule_statements
    : rule_statements rule_statement
    {
        $$ = $1;
        if ($2) {
            append_ast_list(ctx, $$, $2);
        }
    }
    | rule_statement
    {
        if ($1) {
            $$ = create_ast_list(ctx, $1);
        } else {
            $$ = NULL;
        }
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

rule_statement
    : let_statement optional_semicolon { $$ = $1; }
    | if_statement { $$ = $1; }
    | return_statement optional_semicolon { $$ = $1; }
    | for_statement { $$ = $1; }
    | while_statement { $$ = $1; }
    | assignment_statement optional_semicolon { $$ = $1; }
    | function_call optional_semicolon { $$ = $1; }
    | error optional_semicolon { $$ = NULL; }
    ;

optional_semicolon
    : ';'
    | /* empty */
    ;

let_statement
    : LET IDENTIFIER '=' expression
    {
        ast_node_t* node = create_ast_node(ctx, AST_LET_STMT);
        node->data.let_stmt.name = $2;
        node->data.let_stmt.init = $4;
        $$ = node;
        add_symbol(ctx, $2, "local");
    }
    ;

assignment_statement
    : primary_expression '=' expression
    {
        ast_node_t* node = create_ast_node(ctx, AST_ASSIGN_STMT);
        node->data.assign_stmt.target = $1;
        node->data.assign_stmt.value = $3;
        $$ = node;
        
        if ($1->type == AST_IDENTIFIER) {
            if (!find_symbol(ctx, $1->data.identifier.name)) {
                printf("Warning: Assignment to undeclared variable %s\n", 
                       $1->data.identifier.name);
                add_symbol(ctx, $1->data.identifier.name, "local");
            }
        }
    }
    ;

primary_expression
    : IDENTIFIER
    {
        $$ = create_identifier_node(ctx, $1);
        free($1);
    }
    | STRING_LITERAL
    {
        $$ = create_string_literal_node(ctx, $1);
        free($1);
    }
    | INTEGER_LITERAL
    {
        $$ = create_integer_literal_node(ctx, $1);
    }
    | FLOAT_LITERAL
    {
        $$ = create_float_literal_node(ctx, $1);
    }
    | NIL
    {
        ast_node_t* node = create_ast_node(ctx, AST_IDENTIFIER);
        node->data.identifier.name = strdup("nil");
        $$ = node;
    }
    | array_literal
    {
        $$ = $1;
    }
    | function_call
    {
        $$ = $1;
    }
    | '(' expression ')'
    {
        $$ = $2;
    }
    ;

unary_expression
    : primary_expression
    {
        $$ = $1;
    }
    | '-' unary_expression %prec UMINUS
    {
        ast_node_t* node = create_ast_node(ctx, AST_UNARY_EXPR);
        node->data.unary_expr.op = OP_MINUS;
        node->data.unary_expr.operand = $2;
        $$ = node;
    }
    | NOT unary_expression
    {
        ast_node_t* node = create_ast_node(ctx, AST_UNARY_EXPR);
        node->data.unary_expr.op = OP_NOT;
        node->data.unary_expr.operand = $2;
        $$ = node;
    }
    ;

expression
    : unary_expression
    {
        $$ = $1;
    }
    | expression '.' IDENTIFIER
    {
        ast_node_t* node = create_ast_node(ctx, AST_MEMBER_ACCESS);
        node->data.member_access.target = $1;
        node->data.member_access.member = $3;
        $$ = node;
    }
    | expression '[' expression ']'
    {
        ast_node_t* node = create_ast_node(ctx, AST_MAP_ACCESS);
        node->data.map_access.target = $1;
        node->data.map_access.key = $3;
        $$ = node;
    }
    | expression '+' expression
    {
        $$ = create_binary_expr_node(ctx, OP_ADD, $1, $3);
    }
    | expression '-' expression
    {
        $$ = create_binary_expr_node(ctx, OP_SUB, $1, $3);
    }
    | expression '*' expression
    {
        $$ = create_binary_expr_node(ctx, OP_MUL, $1, $3);
    }
    | expression '/' expression
    {
        $$ = create_binary_expr_node(ctx, OP_DIV, $1, $3);
    }
    | expression '%' expression
    {
        $$ = create_binary_expr_node(ctx, OP_MOD, $1, $3);
    }
    | expression BAND expression
    {
        $$ = create_binary_expr_node(ctx, OP_BAND, $1, $3);
    }
    | expression BOR expression
    {
        $$ = create_binary_expr_node(ctx, OP_BOR, $1, $3);
    }
    | expression BXOR expression
    {
        $$ = create_binary_expr_node(ctx, OP_BXOR, $1, $3);
    }
    | expression LSHIFT expression
    {
        $$ = create_binary_expr_node(ctx, OP_LSHIFT, $1, $3);
    }
    | expression RSHIFT expression
    {
        $$ = create_binary_expr_node(ctx, OP_RSHIFT, $1, $3);
    }
    | expression comparison_operator expression
    {
        $$ = create_binary_expr_node(ctx, $2, $1, $3);
    }
    ;

if_statement
    : IF expression '{' rule_statements '}' %prec THEN
    {
        ast_node_t* node = create_ast_node(ctx, AST_IF_STMT);
        node->data.if_stmt.condition = $2;
        node->data.if_stmt.then_body = $4;
        node->data.if_stmt.else_body = NULL;
        $$ = node;
    }
    | IF expression '{' rule_statements '}' ELSE '{' rule_statements '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_IF_STMT);
        node->data.if_stmt.condition = $2;
        node->data.if_stmt.then_body = $4;
        node->data.if_stmt.else_body = $8;
        $$ = node;
    }
    ;

return_statement
    : RETURN CONTINUE
    {
        ast_node_t* node = create_ast_node(ctx, AST_RETURN_STMT);
        node->data.return_stmt.type = RETURN_CONTINUE;
        $$ = node;
    }
    | RETURN SKIP
    {
        ast_node_t* node = create_ast_node(ctx, AST_RETURN_STMT);
        node->data.return_stmt.type = RETURN_SKIP;
        $$ = node;
    }
    | RETURN BLOCK
    {
        ast_node_t* node = create_ast_node(ctx, AST_RETURN_STMT);
        node->data.return_stmt.type = RETURN_BLOCK;
        $$ = node;
    }
    ;

for_statement
    : FOR IDENTIFIER IN expression '{' rule_statements '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_FOR_STMT);
        node->data.for_stmt.iterator = $2;
        node->data.for_stmt.range = $4;
        node->data.for_stmt.body = $6;
        $$ = node;
        add_symbol(ctx, $2, "iterator");
    }
    | FOR IDENTIFIER RANGE expression '{' rule_statements '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_FOR_STMT);
        node->data.for_stmt.iterator = $2;
        node->data.for_stmt.range = $4;
        node->data.for_stmt.body = $6;
        $$ = node;
        add_symbol(ctx, $2, "iterator");
    }
    ;

while_statement
    : WHILE expression '{' rule_statements '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_WHILE_STMT);
        node->data.while_stmt.condition = $2;
        node->data.while_stmt.body = $4;
        $$ = node;
    }
    ;

function_call
    : MATCH_KEYWORD '(' expression ')'
    {
        ast_node_t* node = create_ast_node(ctx, AST_FUNC_CALL);
        node->data.func_call.name = strdup("match_keyword");
        node->data.func_call.args = create_ast_list(ctx, $3);
        $$ = node;
    }
    | MATCH_KEYWORD_VALUE '(' expression ',' expression ')'
    {
        ast_node_t* node = create_ast_node(ctx, AST_FUNC_CALL);
        node->data.func_call.name = strdup("match_keyword_value");
        ast_list_t* args = create_ast_list(ctx, $3);
        append_ast_list(ctx, args, $5);
        node->data.func_call.args = args;
        $$ = node;
    }
    ;

array_literal
    : '[' array_items ']'
    {
        ast_node_t* node = create_ast_node(ctx, AST_ARRAY_LITERAL);
        node->data.array_literal.items = $2;
        $$ = node;
    }
    ;

array_items
    : array_items ',' expression
    {
        $$ = $1;
        append_ast_list(ctx, $$, $3);
    }
    | expression
    {
        $$ = create_ast_list(ctx, $1);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

comparison_operator
    : EQ { $$ = OP_EQ; }
    | NE { $$ = OP_NE; }
    | GT { $$ = OP_GT; }
    | LT { $$ = OP_LT; }
    | GE { $$ = OP_GE; }
    | LE { $$ = OP_LE; }
    | AND { $$ = OP_AND; }
    | OR { $$ = OP_OR; }
    ;

rule_declaration
    : RULE rule_name rule_modifiers '{' rule_statements '}'
    {
        ast_node_t* node = create_ast_node(ctx, AST_RULE);
        node->data.rule.name = $2;
        node->data.rule.body = $5;
        $$ = node;
        pop_scope(ctx);
    }
    ;

rule_name
    : IDENTIFIER
    {
        scope_t* scope = create_scope(ctx, $1);
        push_scope(ctx, scope);
        add_symbol(ctx, $1, "rule");
        $$ = $1;
    }
    ;

rule_modifiers
    : /* empty */
    | modifier_list
    ;

modifier_list
    : after_modifiers
    | before_modifiers
    | after_modifiers before_modifiers
    | before_modifiers after_modifiers
    ;

after_modifiers
    : AFTER identifier_list
    ;

before_modifiers
    : BEFORE identifier_list
    ;

identifier_list
    : IDENTIFIER
    {
        ast_list_t* list = create_ast_list(ctx, create_identifier_node(ctx, $1));
        $$ = list;
        add_symbol(ctx, $1, "rule_dependency");
    }
    | identifier_list ',' IDENTIFIER
    {
        $$ = $1;
        append_ast_list(ctx, $$, create_identifier_node(ctx, $3));
        add_symbol(ctx, $3, "rule_dependency");
    }
    ;

%%

int yyparse(parser_context_t* ctx);
