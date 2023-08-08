#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include "node.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct LinkedList LinkedList;

LinkedList *ll_create(bool mtf);

void ll_delete(LinkedList **ll);

uint32_t ll_length(LinkedList *ll);

Node *ll_lookup(LinkedList *ll, char *uri);

void ll_insert(LinkedList *ll, char *uri, pthread_mutex_t lock);

void ll_print(LinkedList *ll);

#endif
