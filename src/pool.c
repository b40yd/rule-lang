#include <string.h>
#include "pool.h"

// 内存池函数实现
memory_pool_t* create_pool(size_t size) {
    memory_pool_t* pool = malloc(sizeof(memory_pool_t));
    if (!pool) return NULL;
    
    pool->start = malloc(size);
    if (!pool->start) {
        free(pool);
        return NULL;
    }
    
    pool->current = pool->start;
    pool->size = size;
    pool->next = NULL;
    return pool;
}

void* palloc(memory_pool_t* pool, size_t size) {
    // 对齐到8字节边界
    size = (size + 7) & ~7;
    
    memory_pool_t* p = pool;
    while (p) {
        if ((size_t)(p->start + p->size - p->current) >= size) {
            void* mem = p->current;
            p->current += size;
            return mem;
        }
        if (!p->next) {
            p->next = create_pool(size > POOL_SIZE ? size : POOL_SIZE);
            if (!p->next) return NULL;
        }
        p = p->next;
    }
    return NULL;
}

char* pstrdup(memory_pool_t* pool, const char* str) {
    size_t len = strlen(str) + 1;
    char* new_str = palloc(pool, len);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}

void destroy_pool(memory_pool_t* pool) {
    memory_pool_t* p = pool;
    while (p) {
        memory_pool_t* next = p->next;
        free(p->start);
        free(p);
        p = next;
    }
}