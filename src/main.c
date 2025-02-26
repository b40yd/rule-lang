#include <stdio.h>
#include "parser.h"

extern FILE* yyin;
extern int yylineno;

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *input = fopen(argv[1], "r");
        if (!input) {
            fprintf(stderr, "Cannot open input file '%s'\n", argv[1]);
            return 1;
        }
        yyin = input;
        printf("Parsing file: %s\n", argv[1]);
    } else {
        printf("Reading from standard input...\n");
    }
    
    printf("Starting parser...\n");
    printf("===================\n");
    
    yylineno = 1;  // 重置行号
    int result = yyparse();
    
    printf("===================\n");
    if (result == 0) {
        printf("Parsing completed successfully.\n");
    } else {
        printf("Parsing failed with errors.\n");
    }
    
    if (argc > 1) {
        fclose(yyin);
    }
    
    return result;
}
