#include "ll.h"

#include "node.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct LinkedList {
    uint32_t length;
    Node *head;
    Node *tail;
    bool mtf;
};

//makes empty list
LinkedList *ll_create(bool mtf) {
    LinkedList *ll = (LinkedList *) malloc(sizeof(LinkedList));
    pthread_mutex_t foo = PTHREAD_MUTEX_INITIALIZER;
    ll->head = node_create("head", foo);
    ll->tail = node_create("tail", foo);
    ll->head->next = ll->tail;
    ll->tail->prev = ll->head;
    ll->head->prev = NULL;
    ll->tail->next = NULL;
    ll->mtf = mtf;
    ll->length = 0;
    return ll;
}

//deletes list
void ll_delete(LinkedList **ll) {
    Node *current = (*ll)->head;
    Node *next = current->next;
    while (next != NULL) {
        next = current->next;
        node_delete(&current);
        current = next;
    }
    free(*ll);
    *ll = NULL;
    return;
}

//returns length
uint32_t length(LinkedList *ll) {
    return ll->length;
}

//looks for oldspeak in list
Node *ll_lookup(LinkedList *ll, char *uri) {
    Node *current = ll->head;
    for (uint32_t i = 0; i < ll->length; i++) {
        current = current->next;
        if (strcmp(current->uri, uri) == 0) {
            if (ll->mtf) {
                ll->head->next->prev = current;
                current->next = ll->head->next;
                ll->head->next = current;
                current->prev = ll->head;
            }
            return current;
        }
    }
    return NULL;
}

//inserts node to list
void ll_insert(LinkedList *ll, char *uri, pthread_mutex_t lock) {
    Node *n = node_create(uri, lock);
    ll->head->next->prev = n;
    n->next = ll->head->next;
    ll->head->next = n;
    n->prev = ll->head;
    ll->length += 1;
    return;
}

//prints list
void ll_print(LinkedList *ll) {
    Node *current = ll->head;
    for (uint32_t i = 0; i < ll->length; i++) {
        current = current->next;
        node_print(current);
    }
    return;
}
