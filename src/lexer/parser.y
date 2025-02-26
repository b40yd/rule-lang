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
    char* str_val;
    struct {
        char* value;
        char* type;
    } expr_val;
}

%token <str_val> IDENTIFIER STRING_LITERAL
%token <int_val> INTEGER_LITERAL
%token GLOBAL NAMESPACE RULE IF LET RETURN CONTINUE SKIP BLOCK
%token AFTER BEFORE FOR RANGE IN MAP NIL WHILE
%token EQ NE GE LE GT LT AND OR

%type <expr_val> expression
%type <str_val> rule_name

%%

program
    : global_section namespace_sections
    ;

global_section
    : GLOBAL '{' global_declarations '}'
    | /* empty */
    ;

global_declarations
    : global_declarations global_declaration
    | global_declaration
    ;

global_declaration
    : IDENTIFIER '=' expression
    {
        add_symbol($1, "global");
        printf("Global variable declared: %s\n", $1);
    }
    ;

namespace_sections
    : namespace_sections namespace_section
    | namespace_section
    ;

namespace_section
    : NAMESPACE IDENTIFIER '{'
    {
        push_scope($2);
        printf("Entering namespace: %s\n", $2);
    }
    rule_declarations
    '}'
    {
        pop_scope();
        printf("Exiting namespace: %s\n", $2);
    }
    ;

rule_declarations
    : rule_declarations rule_declaration
    | rule_declaration
    ;

rule_declaration
    : RULE rule_name
    {
        push_scope($2);
    }
    rule_modifiers '{' rule_body '}'
    {
        pop_scope();
        printf("Rule declared: %s\n", $2);
    }
    ;

rule_name
    : IDENTIFIER
    {
        $$ = $1;
    }
    ;

rule_modifiers
    : /* empty */
    | AFTER IDENTIFIER
    | BEFORE IDENTIFIER
    | AFTER IDENTIFIER BEFORE IDENTIFIER
    ;

rule_body
    : statements
    ;

statements
    : statements statement
    | statement
    ;

statement
    : let_statement
    | if_statement
    | return_statement
    | for_statement
    | while_statement
    | assignment_statement
    ;

let_statement
    : LET IDENTIFIER '=' expression
    {
        add_symbol($2, "local");
        printf("Local variable declared: %s\n", $2);
    }
    ;

assignment_statement
    : IDENTIFIER '=' expression
    {
        if (!find_symbol($1)) {
            printf("Warning: Assignment to undeclared variable %s\n", $1);
            add_symbol($1, "local");
        }
    }
    ;

if_statement
    : IF expression '{' statements '}'
    | IF '(' expression ')' '{' statements '}'
    ;

return_statement
    : RETURN CONTINUE
    | RETURN SKIP
    | RETURN BLOCK
    ;

for_statement
    : FOR IDENTIFIER IN expression '{' statements '}'
    | FOR IDENTIFIER RANGE expression '{' statements '}'
    ;

while_statement
    : WHILE '(' expression ')' '{' statements '}'
    ;

expression
    : IDENTIFIER
    | STRING_LITERAL
    | INTEGER_LITERAL
    | NIL
    | array_literal
    | map_access
    | comparison_expression
    ;

array_literal
    : '[' array_items ']'
    ;

array_items
    : array_items ',' expression
    | expression
    | /* empty */
    ;

map_access
    : IDENTIFIER '[' expression ']'
    ;

comparison_expression
    : expression EQ expression
    | expression NE expression
    | expression GT expression
    | expression LT expression
    | expression GE expression
    | expression LE expression
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error at line %d: %s\n", yylineno, s);
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
    
    while (scope) {
        current = symbol_table;
        while (current) {
            if (strcmp(current->name, name) == 0 &&
                strcmp(current->scope, scope->scope_name) == 0) {
                return current;
            }
            current = current->next;
        }
        scope = scope->prev;
    }
    return NULL;
}