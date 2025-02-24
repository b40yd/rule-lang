#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"  // Bison 生成的头文件


extern int result;       // 声明全局变量
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE buffer);

void test_expression(const char *expr, int expected) {
    YY_BUFFER_STATE buffer = yy_scan_string(expr);
    yyparse();
    yy_delete_buffer(buffer);

    if (result == expected) {
        printf("[PASS] %s = %d\n", expr, expected);
    } else {
        printf("[FAIL] %s (Expected: %d, Actual: %d)\n", expr, expected, result);
    }
}

int main() {
    // 测试用例
    test_expression("1 + 2\n", 3);
    test_expression("3 * 4\n", 12);
    test_expression("(5 - 2)/1\n", 3);
    return 0;
}
