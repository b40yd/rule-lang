%{
#include "parser.h"
#include <stdlib.h>
%}

%option noyywrap 
%option yylineno

%%
"+"     { return ADD; }
"-"     { return SUB; }
"*"     { return MUL; }
"/"     { return DIV; }
"|"     { return ABS; }
"("     { return OP;  }
")"     { return CP;  }
"//".*  {  }
[0-9]+  { yylval = atoi(yytext); return NUMBER; }
\n      { return EOL; }
[ \t]   { /* 忽略空白字符 */ }
.       { printf("current token: %s\n", yytext); }
%%