#include "parser.h"
#include "utils.h"

int main(int argc, char **argv)
{
    print_error("start calc:");
    yyparse();
    
    return 0;
}