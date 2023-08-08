#include <stdlib.h>
#include <stdio.h>

#define SIZE 128

typedef struct Request {
    int op;
    int connfd;
    char *uri;
    int http;
    int reqid;
    int length;
} Request;

typedef struct Queue {
    int size;
    int head;
    int tail;
    Request **vals;
} Queue;

Queue *queue_create() {
    Queue *q = malloc(sizeof(Queue));
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    q->vals = (Request **) calloc(SIZE, sizeof(Request *));
    return q;
}

int getconnfd(Request *r) {
    return r->connfd;
}

void queue_enqueue(Queue *q, int x) {
    if (q->size == SIZE) {
        return;
    }
    Request *r = calloc(1, sizeof(Request));

    r->connfd = x;
    r->op = -1;
    r->http = -1;
    r->uri = NULL;
    r->reqid = -1;
    r->length = -1;
    q->vals[q->tail] = r;
    q->size++;
    q->tail = (q->tail + 1) % SIZE;
    return;
}

void queue_destroy(Queue **q) {
    for (int i = 0; i < SIZE; i++) {
        free((*q)->vals[i]);
    }
    free((*q)->vals);
    free(*q);
    *q = NULL;
}

int queue_empty(Queue *q) {
    return (q->size == 0);
}

int queue_full(Queue *q) {
    return q->size == SIZE;
}

Request *queue_dequeue(Queue *q) {
    if (q->size == 0) {
        return NULL;
    }
    Request *x = q->vals[q->head];
    q->head = (q->head + 1) % SIZE;
    q->size--;
    return x;
}
