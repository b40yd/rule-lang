#ifndef POOL_H
#define POOL_H

#include <stdlib.h>

// 内存池大小
#define POOL_SIZE (8 * 1024)  // 8KB per pool

// 内存池结构
typedef struct memory_pool {
    char* start;           // 内存块起始位置
    char* current;         // 当前分配位置
    size_t size;          // 内存块大小
    struct memory_pool* next;  // 下一个内存块
} memory_pool_t;

// 内存池函数声明
memory_pool_t* create_pool(size_t size);
void* palloc(memory_pool_t* pool, size_t size);
char* pstrdup(memory_pool_t* pool, const char* str);
void destroy_pool(memory_pool_t* pool);

#endif