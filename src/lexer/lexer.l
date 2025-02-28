%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "parser.h"

void count_column(void);
%}

%option noyywrap
%option yylineno

%%
[ \t]+         { count_column(); } /* 处理空格和制表符 */
[\n\r]+        { count_column(); } /* 处理换行符 */

"global"        { count_column(); return GLOBAL; }
"namespace"     { count_column(); return NAMESPACE; }
"rule"          { count_column(); return RULE; }
"if"            { count_column(); return IF; }
"let"           { count_column(); return LET; }
"return"        { count_column(); return RETURN; }
"continue"      { count_column(); return CONTINUE; }
"skip"          { count_column(); return SKIP; }
"block"         { count_column(); return BLOCK; }
"after"         { count_column(); return AFTER; }
"before"        { count_column(); return BEFORE; }
"for"           { count_column(); return FOR; }
"range"         { count_column(); return RANGE; }
"in"            { count_column(); return IN; }
"while"         { count_column(); return WHILE; }
"map"           { count_column(); return MAP; }
"nil"           { count_column(); return NIL; }
"string"        { count_column(); return STRING_TYPE; }
"int"           { count_column(); return INT_TYPE; }
"float"         { count_column(); return FLOAT_TYPE; }
"array"         { count_column(); return ARRAY_TYPE; }
"match_keyword" { count_column(); return MATCH_KEYWORD; }
"match_keyword_value" { count_column(); return MATCH_KEYWORD_VALUE; }

[a-zA-Z_][a-zA-Z0-9_]* { 
    yylval.str_val = strdup(yytext);
    count_column();
    return IDENTIFIER;
}

\"([^\"\\]|\\.)*\"     { 
    yylval.str_val = strdup(yytext);
    count_column();
    return STRING_LITERAL;
}

'([^'\\]|\\.)*'        { 
    yylval.str_val = strdup(yytext);
    count_column();
    return STRING_LITERAL;
}

[0-9]+\.[0-9]+ {
    yylval.float_val = atof(yytext);
    count_column();
    return FLOAT_LITERAL;
}

[0-9]+         {
    yylval.int_val = atoi(yytext);
    count_column();
    return INTEGER_LITERAL;
}

"=="           { count_column(); return EQ; }
"!="           { count_column(); return NE; }
">="           { count_column(); return GE; }
"<="           { count_column(); return LE; }
">"            { count_column(); return GT; }
"<"            { count_column(); return LT; }
"&&"           { count_column(); return AND; }
"||"           { count_column(); return OR; }

"{"            { count_column(); return '{'; }
"}"            { count_column(); return '}'; }
"["            { count_column(); return '['; }
"]"            { count_column(); return ']'; }
"("            { count_column(); return '('; }
")"            { count_column(); return ')'; }
"="            { count_column(); return '='; }
","            { count_column(); return ','; }
"."            { count_column(); return '.'; }
";"            { count_column(); return ';'; }
"$"            { count_column(); return '$'; }

#[^\n]*\n      { count_column(); } /* 处理单行注释 */
.              { count_column(); return yytext[0]; } /* 返回未知字符以支持错误恢复 */

%%

int current_column = 0;

void count_column(void) {
    for (int i = 0; yytext[i] != '\0'; i++) {
        if (yytext[i] == '\n')
            current_column = 0;
        else if (yytext[i] == '\t')
            current_column += 8 - (current_column % 8);
        else
            current_column++;
    }
} 