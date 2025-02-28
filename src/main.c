#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "parser.h"

extern FILE* yyin;
extern int yylineno;
extern int yyparse(parser_context_t* ctx);

int main(int argc, char **argv) {
    // 创建解析器上下文
    parser_context_t* ctx = create_parser_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create parser context\n");
        return 1;
    }

    // 设置输入文件
    if (argc > 1) {
        FILE *input = fopen(argv[1], "r");
        if (!input) {
            fprintf(stderr, "Cannot open input file '%s'\n", argv[1]);
            destroy_parser_context(ctx);
            return 1;
        }
        yyin = input;
        ctx->current_file = argv[1];
        printf("Parsing file: %s\n", argv[1]);
    } else {
        printf("Reading from standard input...\n");
    }
    
    printf("Starting parser...\n");
    printf("===================\n");
    
    // 重置行号并开始解析
    yylineno = 1;
    int result = yyparse(ctx);
    
    printf("===================\n");
    if (result == 0 && ctx->error_count == 0) {
        printf("Parsing completed successfully.\n");
        // 打印AST
        if (ctx->root) {
            printf("\nAbstract Syntax Tree:\n");
            print_ast(ctx->root, 0);
        }
    } else {
        printf("Parsing failed with %d errors.\n", ctx->error_count);
    }
    
    // 清理资源
    if (argc > 1) {
        fclose(yyin);
    }
    destroy_parser_context(ctx);
    
    return result;
}
