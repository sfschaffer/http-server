#include "hash.h"

#include "ll.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct HashTable {
    uint32_t size;
    bool mtf;
    LinkedList **lists;
};

int hash(char *uri) {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += uri[i];
        if (uri[i] == 0) {
            break;
        }
    }
    return sum;
}

//creates hash table
HashTable *ht_create(uint32_t size, bool mtf) {
    HashTable *ht = (HashTable *) malloc(sizeof(HashTable));
    if (ht) {
        ht->size = size;
        ht->mtf = mtf;
        ht->lists = (LinkedList **) calloc(size, sizeof(LinkedList *));
        if (!ht->lists) {
            free(ht);
            ht = NULL;
        }
    }
    return ht;
}

//deletes hash table
void ht_delete(HashTable **ht) {
    for (uint32_t i = 0; i < (*ht)->size; i++) {
        if ((*ht)->lists[i]) {
            LinkedList *ll = (*ht)->lists[i];
            ll_delete(&ll);
        } else {
            free((*ht)->lists[i]);
        }
    }
    free((*ht)->lists);
    free(*ht);
    *ht = NULL;
}

//returns size of table
uint32_t ht_size(HashTable *ht) {
    return ht->size;
}

//searches hashtable for oldspeak
Node *ht_lookup(HashTable *ht, char *uri) {
    LinkedList *ll = ht->lists[hash(uri) % ht->size];
    if (!ll) {
        return NULL;
    }
    return ll_lookup(ll, uri);
}

//inserts old/newspeak combo to table
void ht_insert(HashTable *ht, char *uri, pthread_mutex_t lock) {
    uint32_t index = hash(uri);
    index %= ht->size;
    if (!ht->lists[index]) {
        ht->lists[index] = ll_create(ht->mtf);
    }
    ll_insert(ht->lists[index], uri, lock);
}

//prints hashtable
void ht_print(HashTable *ht) {
    for (uint32_t i = 0; i < ht->size; i++) {
        if (ht->lists[i]) {
            printf("Index : %d\n", i);
            ll_print(ht->lists[i]);
        }
    }
}
