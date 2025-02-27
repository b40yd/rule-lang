cmake_minimum_required(VERSION 3.15)
project(rule_lang C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 Bison 和 Flex
find_package(BISON 2.3 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

# 关键修复1：添加头文件搜索路径（包括项目include目录和生成目录）
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include   # 项目自定义头文件目录
    ${CMAKE_CURRENT_BINARY_DIR}           # 生成的parser.h所在目录
)

# 生成解析器代码
BISON_TARGET(
    rule_parser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
    COMPILE_FLAGS "-Wno-conflicts-sr -Wno-conflicts-rr" 
)

# 生成词法分析器代码
FLEX_TARGET(
    rule_lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.c
    COMPILE_FLAGS "--noyywrap"
)

# 添加 Bison 和 Flex 的依赖关系
ADD_FLEX_BISON_DEPENDENCY(rule_lexer rule_parser)

# 关键修复2：创建静态库（包含生成的词法/语法分析器代码）
add_library(parserlib STATIC
    ${FLEX_rule_lexer_OUTPUTS}
    ${BISON_rule_parser_OUTPUTS}
)

# 关键修复3：为所有目标添加头文件目录
target_include_directories(parserlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 主可执行文件
add_executable(rulec 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pool.c
)

# 关键修复4：链接静态库到可执行文件
target_link_libraries(rulec parserlib)

# 测试可执行文件
# add_executable(test_lexer 
#     ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_lexer.c
# )

# # 关键修复5：测试程序也需要链接库
# target_link_libraries(test_lexer parserlib)

# enable_testing()
# add_test(NAME lexer_test COMMAND test_lexer)
