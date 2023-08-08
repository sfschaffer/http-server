#include "node.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//makes node
Node *node_create(char *uri, pthread_mutex_t lock) {
    Node *n = (Node *) malloc(sizeof(Node));
    n->lock = lock;
    n->uri = uri;
    pthread_mutex_init(&lock, NULL);
    return n;
}

//deletes node
void node_delete(Node **n) {
    free(*n);
    *n = NULL;
}

void node_print(Node *n) {
    printf("%s\n", n->uri);
}
