%{
#include <stdio.h>
#include <stdlib.h>   // 为 free() 提供声明

int result;

// 声明外部变量（来自 Flex）
extern int yylineno;    // Flex 的行号变量
extern char* yytext;    // Flex 的当前词法值
extern int yylval;      // Flex 的词法值传递变量

%}

%code requires {
    int yylex(void);    // 声明 yylex 函数
}

%code provides {
    void yyerror(const char *s);  // 声明错误函数
}

// 定义运算符优先级
%left ADD SUB
%left MUL DIV
%left ABS

// 定义 Token
%token NUMBER OP CP EOL

// 启用位置追踪（可选增强）
%locations


%%

calclist:
    | calclist exp EOL { result = $2; }
    ;

exp: factor       { $$ = $1; }
    | exp ADD factor { $$ = $1 + $3; }
    | exp SUB factor { $$ = $1 - $3; }
    ;

factor: term      { $$ = $1; }
    | factor MUL term { $$ = $1 * $3; }
    | factor DIV term { $$ = $1 / $3; }
    ;

term: NUMBER      { $$ = $1; }
    | ABS term    { $$ = $2 >= 0 ? $2 : -$2;}
    | OP exp CP   { $$ = $2; }
    ;

%%

// 实现增强版错误处理
void yyerror(const char *s) {
    fprintf(stderr, "Error at line %d: %s\n", yylineno, s);
    fprintf(stderr, "Current token: '%s' (value=%d)\n", yytext, yylval);
}
