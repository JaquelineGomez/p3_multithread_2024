#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_BUFFER 10  // Define the maximum buffer size

// Function prototypes
Operation* parse_operation(const char *line);
void process_operation(Operation *op);

// Global variables
int profits = 0;
int product_stock[5] = {0};
int purchase_prices[5] = {2, 5, 15, 25, 100}; // Purchase cost per unit
int sale_prices[5] = {3, 10, 20, 40, 125};     // Sales price per unit
queue *q;
pthread_mutex_t profits_mutex;
pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_full;
pthread_cond_t queue_not_empty;
pthread_mutex_t product_stock_mutex = PTHREAD_MUTEX_INITIALIZER;

// Struct to hold arguments for threads
typedef struct {
    queue *q;           // Pointer to the queue
    Operation *operations;  // Array of operations
    int start_idx;      // Starting index for the thread
    int end_idx;        // Ending index for the thread
    int *product_stock; // Pointer to product stock array
    int *profits;       // Pointer to profits variable
} ThreadArgs;

// Function prototypes
void *producer(void *arg);
void *consumer(void *arg);

int main(int argc, const char * argv[]) {
    int profits = 0;
    int product_stock[5] = {0}; // Initialize product stock

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
        return 1;
    }

    // Get command line arguments
    const char *file_name = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int buff_size = atoi(argv[4]);

    // Open the file
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    queue *q = queue_init(buff_size);
    // Read the number of operations from the file
    int num_operations;
    if (fscanf(file, "%d", &num_operations) != 1) {
        fprintf(stderr, "Error reading number of operations from file\n");
        fclose(file);
        return 1;
    }

    // Allocate memory for the operations array
    Operation *operations = malloc(num_operations * sizeof(Operation));
    if (operations == NULL) {
        perror("Error allocating memory for operations");
        fclose(file);
        return 1;
    }

    // Read operations from the file and store them in the operations array
    for (int i = 0; i < num_operations; i++) {
        if (fscanf(file, "%d %8s %d", &operations[i].id, operations[i].op_type, &operations[i].units) != 3) {
            fprintf(stderr, "Error reading operation %d from file\n", i + 1);
            free(operations);
            fclose(file);
            return 1;
        }
    }

    // Close the file
    fclose(file);

    // Calculate the number of operations per producer and per consumer
    int ops_per_producer = num_operations / num_producers;
    int ops_per_consumer = num_operations / num_consumers;

    // Adjust the number of operations for the last producer and last consumer
    int remaining_ops_producer = num_operations % num_producers;
    int remaining_ops_consumer = num_operations % num_consumers;

    // Create producer threads
    ThreadArgs producer_args[num_producers];
    pthread_t producer_threads[num_producers];
    for (int i = 0; i < num_producers; i++) {
        int start_idx = i * ops_per_producer;
        int end_idx = start_idx + ops_per_producer;
        if (i == num_producers - 1) {
            // Last producer handles remaining operations
            end_idx += remaining_ops_producer;
        }
        // Create ThreadArgs for producer thread
        
        ThreadArgs arg = { .q = q, 
                    .operations = operations, 
                    .start_idx = start_idx, 
                    .end_idx = end_idx, 
                    .product_stock = product_stock, 
                    .profits = &profits };
        producer_args[i] = arg;
        pthread_create(&producer_threads[i], NULL, producer,&producer_args[i]);
    }

    // Create consumer threads
    ThreadArgs consumer_args[num_consumers];
    pthread_t consumer_threads[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        int start_idx = i * ops_per_consumer;
        int end_idx = start_idx + ops_per_consumer;
        if (i == num_consumers - 1) {
            // Last consumer handles remaining operations
            end_idx += remaining_ops_consumer;
        }
        // Create ThreadArgs for consumer thread
       ThreadArgs arg = { .q = q, 
                    .operations = operations, 
                    .start_idx = start_idx, 
                    .end_idx = end_idx, 
                    .product_stock = product_stock, 
                    .profits = &profits };
        consumer_args[i] = arg;
        pthread_create(&consumer_threads[i], NULL, consumer, &consumer_args[i]);
    }

    // Join producer threads
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }

    // Join consumer threads
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }

    // Output results
    printf("Total profit: %d\n", profits);
    printf("Product stock:\n");
    for (int i = 0; i < 5; i++) {
        printf("Product %d: %d\n", i + 1, product_stock[i]);
    }

    // Free allocated memory for operations array
    free(operations);

    return 0;
}

// Producer thread function
void *producer(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;

    // Extract arguments
    int start_idx = thread_args->start_idx;
    int end_idx = thread_args->end_idx;
    Operation *operations = thread_args->operations;
    // int *product_stock = thread_args->product_stock;
    int *profits = thread_args->profits;

    // Insert operations into the queue
    for (int i = start_idx; i < end_idx; i++) {
        Operation op = operations[i];
        
        // Lock product stock mutex before updating
        pthread_mutex_lock(&product_stock_mutex);
        product_stock[op.id - 1] += op.units;
        printf("%d units %d", op.id, op.units);
        pthread_mutex_unlock(&product_stock_mutex);
        
        // Lock profits mutex before updating
        pthread_mutex_lock(&profits_mutex);
        *profits -= (purchase_prices[op.id - 1] * op.units);
        pthread_mutex_unlock(&profits_mutex);
        
        // Put operation into the queue
        queue_put(thread_args->q, &op);
    }

    pthread_exit(NULL);
}

// Consumer thread function
void *consumer(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;

    // Extract arguments
    int start_idx = thread_args->start_idx;
    int end_idx = thread_args->end_idx;
    Operation *operations = thread_args->operations;
    // int *product_stock = thread_args->product_stock;
    int *profits = thread_args->profits;

    // Process operations from the queue
    for (int i = start_idx; i < end_idx; i++) {
        // Get operation from the queue
        Operation *op = queue_get(thread_args->q);

        // Lock product stock mutex before updating
        pthread_mutex_lock(&product_stock_mutex);
        product_stock[op->id - 1] -= op->units;
        printf("%d units %d", op->id, op->units);
        pthread_mutex_unlock(&product_stock_mutex);

        // Lock profits mutex before updating
        pthread_mutex_lock(&profits_mutex);
        *profits += (sale_prices[(op->id) - 1] * op->units);
        pthread_mutex_unlock(&profits_mutex);
    }

    pthread_exit(NULL);
}

// Operation* parse_operation(const char *line) {
//     Operation *op = (Operation *)malloc(sizeof(Operation));
//     if (!op) {
//         perror("Error allocating memory for operation");
//         exit(EXIT_FAILURE);
//     }

//     int scanCount = sscanf(line, "%d %s %d", &op->id, op->op_type, &op->units);
//     if (scanCount != 3) {
//         fprintf(stderr, "Failed to parse operation: %s", line);
//         //free(op);
//         return NULL;
//     }

//     return op;
// }

// void process_operation(Operation *op) {
//     int product_id = op->id - 1; // Product id is 1-indexed
//     int purchase_cost, sale_price;

//     if (strcmp(op->op_type, "PURCHASE") == 0) {
//         purchase_cost = purchase_prices[product_id] * op->units;
//         pthread_mutex_lock(&profits_mutex);
//         profits -= purchase_cost;
//         pthread_mutex_unlock(&profits_mutex);
//         product_stock[product_id] += op->units;
//     } else if (strcmp(op->op_type, "SALE") == 0) {
//         sale_price = sale_prices[product_id] * op->units;
//         pthread_mutex_lock(&profits_mutex);
//         profits += sale_price;
//         pthread_mutex_unlock(&profits_mutex);
//         product_stock[product_id] -= op->units;
//     } else {
//         fprintf(stderr, "Invalid operation type: %s\n", op->op_type);
//     }
// }