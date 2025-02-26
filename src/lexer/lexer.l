%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

void count_column(void);
%}

%option noyywrap
%option yylineno

%%
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

[a-zA-Z_][a-zA-Z0-9_]* { 
    yylval.str_val = strdup(yytext);
    count_column();
    return IDENTIFIER;
}

\"[^\"]*\"     { 
    yylval.str_val = strdup(yytext);
    count_column();
    return STRING_LITERAL;
}

'[^']*'        { 
    yylval.str_val = strdup(yytext);
    count_column();
    return STRING_LITERAL;
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
"$"            { count_column(); return '$'; }

#[^\n]*        { /* 忽略注释 */ }
[ \t\n]+       { /* 忽略空白字符 */ }
.              { /* 忽略未知字符 */ }

%%

int current_column = 0;

void count_column(void) {
    int i;
    for (i = 0; yytext[i] != '\0'; i++) {
        if (yytext[i] == '\n')
            current_column = 0;
        else if (yytext[i] == '\t')
            current_column += 8 - (current_column % 8);
        else
            current_column++;
    }
} 