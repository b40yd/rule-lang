%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex(void);
extern int yylineno;
extern char* yytext;
void yyerror(const char *s);

// 符号表结构
typedef struct symbol_entry {
    char* name;
    char* type;
    char* scope;
    struct symbol_entry* next;
} symbol_entry_t;

// 作用域结构
typedef struct scope_stack {
    char* scope_name;
    struct scope_stack* prev;
} scope_stack_t;

static symbol_entry_t* symbol_table = NULL;
static scope_stack_t* current_scope = NULL;

void push_scope(const char* scope_name);
void pop_scope(void);
void add_symbol(const char* name, const char* type);
symbol_entry_t* find_symbol(const char* name);
%}

%union {
    int int_val;
    double float_val;
    char* str_val;
    struct {
        char* value;
        char* type;
    } expr_val;
}

%token <str_val> IDENTIFIER STRING_LITERAL
%token <int_val> INTEGER_LITERAL
%token <float_val> FLOAT_LITERAL
%token GLOBAL NAMESPACE RULE IF LET RETURN CONTINUE SKIP BLOCK
%token AFTER BEFORE FOR RANGE IN MAP NIL WHILE
%token STRING_TYPE INT_TYPE FLOAT_TYPE ARRAY_TYPE
%token EQ NE GE LE GT LT AND OR
%token MATCH_KEYWORD MATCH_KEYWORD_VALUE

%type <expr_val> expression array_literal map_access comparison_expression function_call primary_expression
%type <str_val> rule_name type_spec basic_type map_type array_type

// 定义运算符优先级和结合性
%left OR
%left AND
%left EQ NE
%left GT LT GE LE
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%left '.' '['       // 成员访问和数组访问具有相同的优先级
%nonassoc ')'

%%

program
    : global_section namespace_sections
    ;

global_section
    : GLOBAL IDENTIFIER '{' struct_members '}'
    {
        add_symbol($2, "struct");
        printf("Global struct declared: %s\n", $2);
        free($2);
    }
    | /* empty */
    ;

struct_members
    : struct_members struct_member
    | struct_member
    ;

struct_member
    : IDENTIFIER type_spec
    {
        char type_buf[256];
        snprintf(type_buf, sizeof(type_buf), "struct_member:%s", $2);
        add_symbol($1, type_buf);
        printf("Struct member declared: %s of type %s\n", $1, $2);
        free($1);
        free($2);
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
    | namespace_section
    | /* empty */
    ;

namespace_section
    : NAMESPACE IDENTIFIER '{'
    {
        char scope_name[256];
        snprintf(scope_name, sizeof(scope_name), "namespace:%s", $2);
        push_scope(scope_name);
        add_symbol($2, "namespace");
        printf("Entering namespace: %s\n", $2);
    }
    namespace_items_list
    '}'
    {
        pop_scope();
        printf("Exiting namespace: %s\n", $2);
        free($2);
    }
    ;

namespace_items_list
    : /* empty */
    | namespace_items
    ;

namespace_items
    : namespace_items namespace_item
    | namespace_item
    ;

namespace_item
    : rule_declaration
    | error ';'  /* 错误恢复 */
    ;

rule_body
    : rule_statements_list
    ;

rule_statements_list
    : /* empty */
    | rule_statements
    ;

rule_statements
    : rule_statements rule_statement
    | rule_statement
    ;

rule_statement
    : let_statement optional_semicolon
    | if_statement
    | return_statement optional_semicolon
    | for_statement
    | while_statement
    | assignment_statement optional_semicolon
    | function_call optional_semicolon
    | error optional_semicolon  /* 错误恢复 */
    ;

optional_semicolon
    : ';'
    | /* empty */
    ;

let_statement
    : LET IDENTIFIER '=' expression
    {
        add_symbol($2, "local");
        printf("Local variable declared: %s\n", $2);
        free($2);
    }
    ;

assignment_statement
    : primary_expression '=' expression
    {
        if ($1.type && strcmp($1.type, "identifier") == 0) {
            if (!find_symbol($1.value)) {
                printf("Warning: Assignment to undeclared variable %s\n", $1.value);
                add_symbol($1.value, "local");
            }
        }
        free($1.type);
        free($1.value);
    }
    ;

primary_expression
    : IDENTIFIER
    {
        $$.type = strdup("identifier");
        $$.value = $1;
    }
    | map_access
    ;

if_statement
    : IF expression '{' rule_statements '}'
    | IF '(' expression ')' '{' rule_statements '}'
    ;

return_statement
    : RETURN CONTINUE
    | RETURN SKIP
    | RETURN BLOCK
    ;

for_statement
    : FOR IDENTIFIER IN expression '{' rule_statements '}'
    {
        add_symbol($2, "iterator");
        free($2);
    }
    | FOR IDENTIFIER RANGE expression '{' rule_statements '}'
    {
        add_symbol($2, "iterator");
        free($2);
    }
    ;

while_statement
    : WHILE '(' expression ')' '{' rule_statements '}'
    ;

function_call
    : MATCH_KEYWORD '(' expression ')'
    {
        $$.type = strdup("bool");
        $$.value = strdup("match_result");
    }
    | MATCH_KEYWORD_VALUE '(' expression ',' expression ')'
    {
        $$.type = strdup("bool");
        $$.value = strdup("match_value_result");
    }
    ;

expression
    : primary_expression
    | STRING_LITERAL
    {
        $$.type = strdup("string");
        $$.value = $1;
    }
    | INTEGER_LITERAL
    {
        $$.type = strdup("int");
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", $1);
        $$.value = strdup(buf);
    }
    | FLOAT_LITERAL
    {
        $$.type = strdup("float");
        char buf[32];
        snprintf(buf, sizeof(buf), "%f", $1);
        $$.value = strdup(buf);
    }
    | NIL
    {
        $$.type = strdup("nil");
        $$.value = strdup("nil");
    }
    | array_literal
    | comparison_expression
    | function_call
    | '(' expression ')'
    {
        $$ = $2;
    }
    ;

array_literal
    : '[' array_items ']'
    {
        $$.type = strdup("array");
        $$.value = strdup("array_value");
    }
    ;

array_items
    : array_items ',' expression
    | expression
    | /* empty */
    ;

map_access
    : primary_expression '[' expression ']'
    {
        $$.type = strdup("map_element");
        $$.value = $1.value;
        free($1.type);
    }
    | primary_expression '.' IDENTIFIER
    {
        $$.type = strdup("struct_member");
        char* member_access = malloc(strlen($1.value) + strlen($3) + 2);
        sprintf(member_access, "%s.%s", $1.value, $3);
        $$.value = member_access;
        free($1.type);
        free($1.value);
        free($3);
    }
    ;

comparison_expression
    : expression EQ expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("eq_result");
    }
    | expression NE expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("ne_result");
    }
    | expression GT expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("gt_result");
    }
    | expression LT expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("lt_result");
    }
    | expression GE expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("ge_result");
    }
    | expression LE expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("le_result");
    }
    | expression AND expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("and_result");
    }
    | expression OR expression
    {
        $$.type = strdup("bool");
        $$.value = strdup("or_result");
    }
    ;

rule_declaration
    : RULE rule_name rule_modifiers '{' rule_body '}'
    {
        pop_scope();
        printf("Rule declared: %s\n", $2);
        free($2);
    }
    ;

rule_name
    : IDENTIFIER
    {
        char scope_name[256];
        snprintf(scope_name, sizeof(scope_name), "rule:%s", $1);
        push_scope(scope_name);
        add_symbol($1, "rule");
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
        add_symbol($1, "rule_dependency");
        free($1);
    }
    | identifier_list ',' IDENTIFIER
    {
        add_symbol($3, "rule_dependency");
        free($3);
    }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error at line %d: %s\n", yylineno, s);
    if (yytext[0] != '\0') {
        fprintf(stderr, "Near token: %s\n", yytext);
    }
    if (current_scope) {
        fprintf(stderr, "In scope: %s\n", current_scope->scope_name);
        
        // 添加更多上下文信息
        symbol_entry_t* current = symbol_table;
        fprintf(stderr, "Current scope symbols:\n");
        while (current) {
            if (strcmp(current->scope, current_scope->scope_name) == 0) {
                fprintf(stderr, "  %s (%s)\n", current->name, current->type);
            }
            current = current->next;
        }
    }
}

void push_scope(const char* scope_name) {
    scope_stack_t* new_scope = malloc(sizeof(scope_stack_t));
    new_scope->scope_name = strdup(scope_name);
    new_scope->prev = current_scope;
    current_scope = new_scope;
}

void pop_scope(void) {
    if (current_scope) {
        scope_stack_t* old_scope = current_scope;
        current_scope = current_scope->prev;
        free(old_scope->scope_name);
        free(old_scope);
    }
}

void add_symbol(const char* name, const char* type) {
    symbol_entry_t* new_symbol = malloc(sizeof(symbol_entry_t));
    new_symbol->name = strdup(name);
    new_symbol->type = strdup(type);
    new_symbol->scope = current_scope ? strdup(current_scope->scope_name) : strdup("global");
    new_symbol->next = symbol_table;
    symbol_table = new_symbol;
}

symbol_entry_t* find_symbol(const char* name) {
    symbol_entry_t* current = symbol_table;
    scope_stack_t* scope = current_scope;
    
    // 首先在当前作用域查找
    while (current) {
        if (strcmp(current->name, name) == 0 &&
            current_scope &&
            strcmp(current->scope, current_scope->scope_name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    // 然后在全局作用域查找
    current = symbol_table;
    while (current) {
        if (strcmp(current->name, name) == 0 &&
            strcmp(current->scope, "global") == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}