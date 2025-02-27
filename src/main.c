#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "parser.h"
#include "pool.h"

// 内存池结构声明
typedef struct memory_pool memory_pool_t;

extern FILE* yyin;
extern int yylineno;
extern memory_pool_t* global_pool;

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
    
    // 创建全局内存池
    global_pool = create_pool(POOL_SIZE);
    if (!global_pool) {
        fprintf(stderr, "Failed to create global memory pool\n");
        return 1;
    }
    
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
    
    // 清理所有内存
    destroy_pool(global_pool);
    return result;
}
