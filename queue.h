//SSOO-P3 23/24

#ifndef HEADER_FILE
#define HEADER_FILE


struct element {
  int product_id; //Product identifier
  int op;         //Operation
  int units;      //Product units
};

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

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
