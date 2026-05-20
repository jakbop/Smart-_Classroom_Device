#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"


HashTable* ci_table = NULL;


// 创建哈希表
HashTable* create_hash_table(size_t capacity) 
{
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));

    if (!table) 
    {
        return NULL;
    }

    table->buckets = (HashNode**)calloc(capacity, sizeof(HashNode*));

    if (!table->buckets) 
    {
        free(table);
        return NULL;
    }

    table->size = 0;
    table->capacity = capacity;

    return table;
}


// 哈希函数（经典实现）
static size_t hash_function(const char* key, size_t capacity)
{
	// 假如 key 为 "abc"，那么
	// r = ('a' * 31^2 + 'b' * 31^1 + 'c' * 31^0) % capacity;
	size_t r = 0, i;

	for(i = 0; key[i] != '\0'; i++)
	{
		r = r * 31 + key[i];
	}

	r %= capacity;

	return r;
}


// 在哈希表中插入元素
void hash_table_insert(HashTable* table, const char* key, void* value)
{
	size_t pos = hash_function(key, table->capacity);

	HashNode* p = table->buckets[pos];

	while(p != NULL)
	{
		// 如果 key 已经存在，就更新它对应的 value
		if(strcmp(p->key, key) == 0)
		{
			p->value = value;
			return;
		}
		
		p  = p->next;
	}

	// 如果 key 不存在，就在哈希桶中插入新节点
	p = malloc(sizeof(HashNode));

	if(p == NULL)
	{
		perror("malloc");
		return;
	}

	p->key = strdup(key);
	p->value = value;
	p->next = table->buckets[pos];

	table->buckets[pos] = p;

	table->size++;
}


// 在哈希表中删除元素
int hash_table_remove(HashTable* table, const char* key)
{
	size_t pos = hash_function(key, table->capacity);

	HashNode* p = table->buckets[pos], *q = NULL;

	while(p != NULL)
	{
		// 如果找到了 key，就删除它并返回 1
		if(strcmp(p->key, key) == 0)
		{
			if(q != NULL)
				q->next = p->next;
			else
				table->buckets[pos] = p->next;

			free(p->key);
			free(p);

			table->size--;
			
			return 1;
		}
		
		q = p;
		p  = p->next;
	}

	// 如果未找到 key 就返回 0，表示删除失败
	return 0;
}


// 在哈希表中查找元素
void* hash_table_get(HashTable* table, const char* key)
{
	size_t pos = hash_function(key, table->capacity);

	HashNode* p = table->buckets[pos];

	while(p != NULL)
	{
		// 如果 key 存在，就返回它对应的 value
		if(strcmp(p->key, key) == 0)
			return p->value;
		
		p  = p->next;
	}

	// 如果 key 不存在，就返回 NULL
	return NULL;
}


// 计算哈希表的长度
size_t hash_table_size(HashTable* table)
{
	return table->size;
}


// 判断哈希表是否为空
int hash_table_empty(HashTable* table)
{
	return !table->size;
}


// 销毁哈希表
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

        while(head != NULL) 
        {
            temp = head;
            head = head->next;
            free(temp->key);
            free(temp);
        }
    }

    free(table->buckets);
    free(table);
}

