//SSOO-P3 23/24
#ifndef QUEUE_H
#define QUEUE_H

#ifndef HEADER_FILE
#define HEADER_FILE
#include <pthread.h>


struct element {
  int product_id; //Product identifier
  int op;         //Operation
  int units;      //Product units
};
typedef struct {
    int id;
    char op_type[9]; // PURCHASE or SALE
    int units;
} Operation;

typedef struct queue {
  // Define the struct yourself
  struct element *elements;  // Array of elements
  int capacity;              // Maximum number of items in the queue
  int front;                 // Index of the front element
  int rear;                  // Index of the last element
  int count;                 // Current size of the queue
  pthread_mutex_t lock;      // Mutex for protecting the queue
  pthread_cond_t not_empty;  // Condition variable for empty states
  pthread_cond_t not_full;
  int param1;
}queue;
    Operation *buffer; // Circular buffer
    int size;          // Size of the buffer
    int front;         // Front index
    int rear;          // Rear index
    int count;         // Number of elements in the buffer
    pthread_mutex_t mutex; // Mutex for synchronization
    pthread_cond_t full;   // Condition variable for full buffer
    pthread_cond_t empty;  // Condition variable for empty buffer
} queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
queue* queue_init(int size);
int queue_destroy(queue *q);
int queue_put(queue *q, Operation *op);
Operation *queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);

#endif