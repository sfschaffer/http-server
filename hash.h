#ifndef __HASH_H__
#define __HASH_H__

#include "ll.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct HashTable HashTable;

HashTable *ht_create(uint32_t size, bool mtf);

void ht_delete(HashTable **ht);

uint32_t ht_size(HashTable *ht);

Node *ht_lookup(HashTable *ht, char *uri);

void ht_insert(HashTable *ht, char *uri, pthread_mutex_t lock);

void ht_print(HashTable *ht);

#endif
