/**
 * @file hash_table.c
 * @brief 哈希表实现 - 管理在线设备的ID与socket映射
 *
 * 本模块实现了一个基于链地址法的哈希表，用于：
 *   - 存储在线设备的 "设备ID → TCP socket" 映射关系
 *   - 支持设备的动态上线（插入）和离线（删除）
 *   - 当MQTT收到控制命令时，通过设备ID快速查找对应的socket转发
 *
 * 数据结构：
 *   哈希表由一个桶数组（buckets）组成，每个桶是一个链表。
 *   当多个key映射到同一个桶时（哈希冲突），通过链表串联。
 *
 *   示例（capacity=5）：
 *   buckets[0] → NULL
 *   buckets[1] → ["wengyuanhang"→socket5] → ["device002"→socket6] → NULL
 *   buckets[2] → NULL
 *   buckets[3] → ["device003"→socket7] → NULL
 *   buckets[4] → NULL
 *
 * 哈希函数：
 *   采用经典的乘法哈希：hash = (key[0]*31^(n-1) + key[1]*31^(n-2) + ... + key[n-1]) % capacity
 *   31是经验值，分布均匀且计算高效（31*i = 32*i - i = i<<5 - i）
 *
 * 线程安全说明：
 *   本实现未加锁，但本项目中的使用场景是安全的：
 *   - 插入：只在device_server.c的通信线程中调用
 *   - 删除：只在device_server.c的通信线程中调用（设备断开时）
 *   - 查找：只在mqtt_client.c的messageArrived回调中调用
 *   - paho-mqtt的回调运行在独立的内部线程中，与通信线程可能并发
 *   - 在高并发场景下建议加互斥锁保护
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"


/* 全局哈希表指针，在device_server_init()中创建
 * 被device_server.c和mqtt_client.c共同使用
 * - device_server.c: 插入（设备上线）、删除（设备离线）
 * - mqtt_client.c: 查找（收到控制命令时定位设备socket）
 */
HashTable* ci_table = NULL;


/**
 * @brief 创建哈希表
 *
 * 分配哈希表结构体和桶数组的内存，初始化容量和大小。
 *
 * @param capacity 哈希表的桶数量（建议使用质数以减少冲突）
 * @return 成功返回哈希表指针，失败返回NULL
 *
 * @note 本项目中capacity=10000，远大于实际设备数量，冲突率极低
 */
HashTable* create_hash_table(size_t capacity) 
{
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));

    if (!table) 
    {
        return NULL;
    }

    /* calloc将内存初始化为0，确保所有桶指针为NULL */
    table->buckets = (HashNode**)calloc(capacity, sizeof(HashNode*));

    if (!table->buckets) 
    {
        free(table);
        return NULL;
    }

    table->size = 0;         /* 初始元素数量为0 */
    table->capacity = capacity;

    return table;
}


/**
 * @brief 哈希函数 - 将字符串key映射到桶索引
 *
 * 采用经典的乘法哈希算法：
 *   hash = (key[0]*31^(n-1) + key[1]*31^(n-2) + ... + key[n-1]*31^0) % capacity
 *
 * 例如 key="abc"：
 *   r = 'a'*31^2 + 'b'*31^1 + 'c'*31^0
 *   r = 97*961 + 98*31 + 99
 *   r = 93217 + 3038 + 99 = 96354
 *   r = 96354 % capacity
 *
 * 31的选择原因：
 *   1. 31是质数，有助于减少哈希冲突
 *   2. 31*i 可以优化为 (i<<5) - i，CPU计算高效
 *   3. Java的String.hashCode()也使用31
 *
 * @param key      设备ID字符串
 * @param capacity 哈希表容量
 * @return 桶索引，范围 [0, capacity)
 */
static size_t hash_function(const char* key, size_t capacity)
{
	size_t r = 0, i;

	for(i = 0; key[i] != '\0'; i++)
	{
		r = r * 31 + key[i];  /* 等价于 r = r * 31 + key[i] */
	}

	r %= capacity;

	return r;
}


/**
 * @brief 在哈希表中插入元素
 *
 * 如果key已存在，更新其value；如果key不存在，在对应桶的链表头部插入新节点。
 *
 * @param table 哈希表指针
 * @param key   设备ID字符串（会被strdup复制一份存储）
 * @param value 对应的值（本项目为TCP socket描述符，存储为void*）
 *
 * @note 新节点插入到链表头部（头插法），时间复杂度O(1)
 * @note key通过strdup()复制，删除时需要free()
 */
void hash_table_insert(HashTable* table, const char* key, void* value)
{
	/* 计算key对应的桶索引 */
	size_t pos = hash_function(key, table->capacity);

	/* 遍历该桶的链表，检查key是否已存在 */
	HashNode* p = table->buckets[pos];

	while(p != NULL)
	{
		if(strcmp(p->key, key) == 0)
		{
			/* key已存在，更新value（设备重连时socket可能变化） */
			p->value = value;
			return;
		}
		
		p = p->next;
	}

	/* key不存在，创建新节点并插入链表头部（头插法） */
	p = malloc(sizeof(HashNode));

	if(p == NULL)
	{
		perror("malloc");
		return;
	}

	p->key = strdup(key);    /* 复制key字符串，避免外部修改影响 */
	p->value = value;
	p->next = table->buckets[pos];  /* 新节点指向原链表头部 */

	table->buckets[pos] = p;  /* 更新链表头部为新节点 */

	table->size++;
}


/**
 * @brief 从哈希表中删除元素
 *
 * 根据key找到对应节点并删除，释放节点内存。
 *
 * @param table 哈希表指针
 * @param key   要删除的设备ID
 * @return 1 删除成功，0 未找到key
 *
 * @note 删除节点时会释放key字符串和节点本身的内存
 */
int hash_table_remove(HashTable* table, const char* key)
{
	size_t pos = hash_function(key, table->capacity);

	HashNode* p = table->buckets[pos], *q = NULL;

	while(p != NULL)
	{
		if(strcmp(p->key, key) == 0)
		{
			if(q != NULL)
				q->next = p->next;       /* 删除中间或尾部节点 */
			else
				table->buckets[pos] = p->next;  /* 删除头节点 */

			free(p->key);   /* 释放strdup()复制的key字符串 */
			free(p);        /* 释放节点本身 */

			table->size--;
			
			return 1;  /* 删除成功 */
		}
		
		q = p;    /* q始终指向p的前驱节点 */
		p  = p->next;
	}

	return 0;  /* 未找到key */
}


/**
 * @brief 在哈希表中查找元素
 *
 * 根据key查找对应的value。本项目用于：根据设备ID查找TCP socket描述符。
 *
 * @param table 哈希表指针
 * @param key   设备ID
 * @return 找到返回value指针，未找到返回NULL
 *
 * @note 本项目中value存储的是socket描述符（int），通过(void*)(long)转换存储
 *       取出时需要反向转换：(long)hash_table_get(table, dev_id)
 */
void* hash_table_get(HashTable* table, const char* key)
{
	size_t pos = hash_function(key, table->capacity);

	HashNode* p = table->buckets[pos];

	while(p != NULL)
	{
		if(strcmp(p->key, key) == 0)
			return p->value;
		
		p  = p->next;
	}

	return NULL;  /* 未找到 */
}


/**
 * @brief 获取哈希表中元素的数量
 *
 * @param table 哈希表指针
 * @return 当前存储的元素数量
 */
size_t hash_table_size(HashTable* table)
{
	return table->size;
}


/**
 * @brief 判断哈希表是否为空
 *
 * @param table 哈希表指针
 * @return 1 为空，0 不为空
 */
int hash_table_empty(HashTable* table)
{
	return !table->size;
}


/**
 * @brief 销毁哈希表，释放所有内存
 *
 * 遍历所有桶，释放每个桶链表中的所有节点（包括key字符串和节点本身），
 * 最后释放桶数组和哈希表结构体。
 *
 * @param table 哈希表指针
 *
 * @note 销毁后应将table指针置为NULL，避免悬空指针
 */
void destroy_hash_table(HashTable* table) 
{
    HashNode* head = NULL, *temp = NULL;

    if (!table) 
    {
        return;
    }

    for (size_t i = 0; i < table->capacity; i++) 
    {
        head = table->buckets[i];

        /* 逐个释放链表中的节点 */
        while(head != NULL) 
        {
            temp = head;
            head = head->next;
            free(temp->key);   /* 释放strdup()复制的key字符串 */
            free(temp);        /* 释放节点本身 */
        }
    }

    free(table->buckets);  /* 释放桶数组 */
    free(table);           /* 释放哈希表结构体 */
}
