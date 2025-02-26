#include <stdio.h>
#include "parser.h"

extern FILE* yyin;

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *input = fopen(argv[1], "r");
        if (!input) {
            fprintf(stderr, "Cannot open input file '%s'\n", argv[1]);
            return 1;
        }
        yyin = input;
    }
    
    printf("Starting parser...\n");
    int result = yyparse();
    
    if (argc > 1) {
        fclose(yyin);
    }
    
    return result;
}
