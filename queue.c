//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "queue.h"

//To create a queue
queue* queue_init(int size)
{

  queue *q = (queue *)malloc(sizeof(queue));
  if (q == NULL) {
        return NULL;
    }

    q->elements = (struct element *)malloc(sizeof(struct element) * size);
    if (q->elements == NULL) {
        free(q);
        return NULL;
    }

    q->capacity = size;
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
  return q;
}

// To Enqueue an element
int queue_put(queue *q, struct element* x)
{
  pthread_mutex_lock(&q->lock);
    while (queue_full(q)) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    q->rear = (q->rear + 1) % q->capacity;
    q->elements[q->rear] = *x;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
  return 0;
}

// To Dequeue an element.
struct element* queue_get(queue *q)
{
  struct element* element;
  pthread_mutex_lock(&q->lock);
    while (queue_empty(q)) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    struct element *elem = &q->elements[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
  return element;
}

//To check queue state
int queue_empty(queue *q)
{
  return q->count == 0;
}

int queue_full(queue *q)
{
  return q->count == q->capacity;
}

//To destroy the queue and free the resources
int queue_destroy(queue *q)
{
  free(q->elements);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q);
  return 0;
}
