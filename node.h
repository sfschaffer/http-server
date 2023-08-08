#ifndef __NODE_H__
#define __NODE_H__

#include <pthread.h>

typedef struct Node Node;

struct Node {
    pthread_mutex_t lock;
    char *uri;
    Node *next;
    Node *prev;
};

Node *node_create(char *uri, pthread_mutex_t lock);

void node_delete(Node **n);

void node_print(Node *n);

#endif
