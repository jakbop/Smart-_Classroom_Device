/**
 * @file hash_table.h
 * @brief 哈希表模块接口
 *
 * 声明哈希表的数据结构和操作函数，用于管理在线设备的ID与socket映射。
 *
 * 数据结构：
 *   HashNode - 哈希节点，包含key（设备ID）、value（socket）和链表指针
 *   HashTable - 哈希表，包含桶数组、元素数量和容量
 *
 * 使用场景：
 *   - 设备上线时：hash_table_insert(table, "wengyuanhang", (void*)(long)sock)
 *   - 收到控制命令时：sock = (long)hash_table_get(table, "wengyuanhang")
 *   - 设备离线时：hash_table_remove(table, "wengyuanhang")
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H


/**
 * @brief 哈希节点结构体
 *
 * 采用链地址法解决哈希冲突，每个桶是一个HashNode链表。
 * key存储设备ID字符串（通过strdup复制），value存储TCP socket描述符。
 */
typedef struct HashNode {
    char* key;              /* 设备ID字符串（strdup复制，需free） */
    void* value;            /* 值（本项目存储socket描述符，void*可存任意类型） */
    struct HashNode* next;  /* 链表下一个节点（哈希冲突时串联） */
} HashNode;


/**
 * @brief 哈希表结构体
 *
 * buckets是桶数组，每个元素指向一个HashNode链表（可能为NULL）。
 * size记录当前元素总数，capacity记录桶的数量。
 */
typedef struct HashTable {
    HashNode** buckets;     /* 哈希桶数组，每个桶是一个链表 */
    size_t size;            /* 哈希表中元素的数量 */
    size_t capacity;        /* 哈希表的容量（桶的数量） */
} HashTable;


/* 全局哈希表指针，存储 设备ID→socket 的映射 */
extern HashTable* ci_table;


/**
 * @brief 创建哈希表
 * @param capacity 桶的数量
 * @return 哈希表指针，失败返回NULL
 */
HashTable* create_hash_table(size_t capacity);

/**
 * @brief 插入元素（key已存在则更新value）
 * @param table 哈希表指针
 * @param key   设备ID
 * @param value 值（socket描述符）
 */
void hash_table_insert(HashTable* table, const char* key, void* value);

/**
 * @brief 删除元素
 * @param table 哈希表指针
 * @param key   设备ID
 * @return 1成功，0未找到
 */
int hash_table_remove(HashTable* table, const char* key);

/**
 * @brief 查找元素
 * @param table 哈希表指针
 * @param key   设备ID
 * @return value指针，未找到返回NULL
 */
void* hash_table_get(HashTable* table, const char* key);

/**
 * @brief 获取元素数量
 */
size_t hash_table_size(HashTable* table);

/**
 * @brief 判断是否为空
 * @return 1为空，0不为空
 */
int hash_table_empty(HashTable* table);

/**
 * @brief 销毁哈希表，释放所有内存
 */
void destroy_hash_table(HashTable* table);



#endif // HASH_TABLE_H
