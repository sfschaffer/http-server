#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct Queue Queue;

typedef struct Request Request;

int getconnfd(Request *r);

Queue *queue_create(void);

void *queue_destroy(Queue **q);

int queue_empty(Queue *q);

int queue_full(Queue *q);

int queue_enqueue(Queue *q, int x);

Request *queue_dequeue(Queue *q);

#endif
