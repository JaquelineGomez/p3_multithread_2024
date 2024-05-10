#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include "queue.h"

// Function implementations
queue* queue_init(int size) {
    queue *q = (queue *)malloc(sizeof(queue));
    if (q == NULL) {
        perror("Error allocating memory for queue");
        exit(EXIT_FAILURE);
    }

    q->buffer = (Operation *)malloc(size * sizeof(Operation));
    if (q->buffer == NULL) {
        perror("Error allocating memory for buffer");
        free(q);
        exit(EXIT_FAILURE);
    }

    q->size = size;
    q->front = 0;
    q->rear = -1;
    q->count = 0;

    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        free(q->buffer);
        free(q);
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&q->full, NULL) != 0) {
        perror("Condition variable initialization failed");
        pthread_mutex_destroy(&q->mutex);
        free(q->buffer);
        free(q);
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&q->empty, NULL) != 0) {
        perror("Condition variable initialization failed");
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->full);
        free(q->buffer);
        free(q);
        exit(EXIT_FAILURE);
    }

    return q;
}

int queue_destroy(queue *q) {
    pthread_mutex_lock(&q->mutex);
    free(q->buffer);
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->full);
    pthread_cond_destroy(&q->empty);
    free(q);
    return 0;
}

int queue_put(queue *q, Operation *op) {
    pthread_mutex_lock(&q->mutex);
    while (queue_full(q)) {
        pthread_cond_wait(&q->full, &q->mutex);
    }

    if (op == NULL) { // This block might be unnecessary if you manage shutdown signals differently
        pthread_cond_broadcast(&q->empty);
    } else {
        q->rear = (q->rear + 1) % q->size;
        memcpy(&q->buffer[q->rear], op, sizeof(Operation)); // Use memcpy to avoid pointer aliasing
        q->count++;
        pthread_cond_signal(&q->empty);  // Signal that the queue is no longer empty
    }

    pthread_mutex_unlock(&q->mutex);
    return 0;
}

Operation *queue_get(queue *q) {
    pthread_mutex_lock(&q->mutex);
    while (queue_empty(q)) {
        pthread_cond_wait(&q->empty, &q->mutex);
    }

    Operation *op = &q->buffer[q->front];
    q->front = (q->front + 1) % q->size;
    q->count--;

    pthread_cond_signal(&q->full);  // Signal that the queue is no longer full
    pthread_mutex_unlock(&q->mutex);
    return op;
}

int queue_empty(queue *q) {
    return (q->count == 0);
}

int queue_full(queue *q) {
    return (q->count == q->size);
}