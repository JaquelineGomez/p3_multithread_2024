#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct {
    int id;
    char op_type[9]; // PURCHASE or SALE
    int units;
} Operation;

typedef struct queue {
    Operation *buffer; // Circular buffer
    int size;          // Size of the buffer
    int front;         // Front index
    int rear;          // Rear index
    int count;         // Number of elements in the buffer
    pthread_mutex_t mutex; // Mutex for synchronization
    pthread_cond_t full;   // Condition variable for full buffer
    pthread_cond_t empty;  // Condition variable for empty buffer
} queue;

queue* queue_init(int size);
int queue_destroy(queue *q);
int queue_put(queue *q, Operation *op);
Operation *queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);

#endif
