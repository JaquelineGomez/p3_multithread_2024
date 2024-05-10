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

// Function prototypes
Operation* parse_operation(const char *line);
void process_operation(Operation *op);

// Global variables
int profits = 0;
int product_stock[5] = {0};
int purchase_prices[5] = {2, 5, 15, 25, 100}; // Purchase cost per unit
int sale_prices[5] = {3, 10, 20, 40, 125};     // Sales price per unit
queue *q;
pthread_mutex_t profits_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t product_stock_mutex = PTHREAD_MUTEX_INITIALIZER;
// Function prototypes
void *producer(void *arg);
void *consumer(void *arg);


typedef struct {
    Operation *operations;
    int start;
    int end;
} OperationSlice;

int main(int argc, const char * argv[]) {
    pthread_mutex_init(&profits_mutex, NULL);
    pthread_mutex_init(&product_stock_mutex, NULL);
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse command line arguments
    const char *filename = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int buff_size = atoi(argv[4]);

    // Open file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Read number of operations
    int num_operations;
    fscanf(file, "%d", &num_operations);
    // Allocate memory for all operations
    Operation *all_operations = malloc(num_operations * sizeof(Operation));
    if (!all_operations) {
        perror("Failed to allocate operations array");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Read operations into array
    char line[MAX_FILENAME_LENGTH];
    int actual_operations_read = 0;
    for (int i = 0; i < num_operations && fgets(line, MAX_FILENAME_LENGTH, file); i++) 
    {
      printf("\nLINE %d info : %s", i, line);
        Operation *op = parse_operation(line);
        if (op) {
            all_operations[actual_operations_read++] = *op;
            free(op); // Free the temporary operation object
        }
    }
    fclose(file);
    // Initialize queue
    q = queue_init(buff_size);

    pthread_t producers[num_producers];
    OperationSlice slices[num_producers]; // Hold slices in an array to avoid premature free
    for (int i = 0; i < num_producers; i++) {
        int start_index = i * (num_operations / num_producers);
        int end_index = (i == num_producers - 1) ? num_operations : start_index + (num_operations / num_producers);
        slices[i] = (OperationSlice){all_operations, start_index, end_index};
        pthread_create(&producers[i], NULL, producer, &slices[i]);
    }

    // Launch consumer threads
    pthread_t consumers[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    // Signal consumers to exit by pushing NULL operations
    for (int i = 0; i < num_consumers; i++) {
        queue_put(q, NULL);
    }

    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Clean up and output results
    printf("Total: %d euros\n", profits);
    printf("Stock:\n");
    for (int i = 0; i < 5; i++) {
        printf("  Product %d: %d\n", i + 1, product_stock[i]);
    }

    free(all_operations);
    queue_destroy(q);
    pthread_mutex_destroy(&profits_mutex);
    pthread_mutex_destroy(&product_stock_mutex);
    return EXIT_SUCCESS;
}

void *producer(void *arg) {
    OperationSlice *slice = (OperationSlice *)arg;
    for (int i = slice->start; i < slice->end; i++) {
        queue_put(q, &slice->operations[i]);
    }
    return NULL; // Ensure clean exit from thread function
}

void *consumer(void *arg) {
    while (1) {
        Operation *op = queue_get(q);
        if (op == NULL) break;  // Properly handle NULL signal for shutdown
        process_operation(op);
    }
    return NULL;  // Ensure clean exit from thread function
}


Operation* parse_operation(const char *line) {
    Operation *op = (Operation *)malloc(sizeof(Operation));
    if (!op) {
        perror("Error allocating memory for operation");
        exit(EXIT_FAILURE);
    }

    int scanCount = sscanf(line, "%d %s %d", &op->id, op->op_type, &op->units);
    printf("Scancount %d\n", scanCount);

    if (scanCount != 3) { // Ensure that we correctly parse three items
        fprintf(stderr, "Failed to parse operation: %s", line);
        free(op);
        return NULL;
    }
    if (strcmp(op->op_type, "PURCHASE") != 0 && strcmp(op->op_type, "SALE") != 0) {
        fprintf(stderr, "Invalid operation type encountered: %s\n", op->op_type);
        free(op);
        return NULL;
    }

    return op;
}

void process_operation(Operation *op) {
    int product_id = op->id - 1; // Product id is 1-indexed
    int purchase_cost, sale_price;

    if (strcmp(op->op_type, "PURCHASE") == 0) {
        purchase_cost = purchase_prices[product_id] * op->units;
        pthread_mutex_lock(&profits_mutex);
        profits -= purchase_cost;
        pthread_mutex_unlock(&profits_mutex);
        product_stock[product_id] += op->units;
    } else if (strcmp(op->op_type, "SALE") == 0) {
        sale_price = sale_prices[product_id] * op->units;
        pthread_mutex_lock(&profits_mutex);
        profits += sale_price;
        pthread_mutex_unlock(&profits_mutex);
        product_stock[product_id] -= op->units;
    } else {
        fprintf(stderr, "Invalid operation type: %s\n", op->op_type);
    }
}
