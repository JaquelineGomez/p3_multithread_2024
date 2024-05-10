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
// Function prototypes
void *producer(void *arg);
void *consumer(void *arg);

int main(int argc, const char * argv[]) {
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

    // Initialize queue
    q = queue_init(buff_size);

    // Launch producer threads
    pthread_t producers[num_producers];
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, (void *)file);
    }

    // Launch consumer threads
    pthread_t consumers[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    // Wait for all producer threads to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    // Signal consumers to exit
    for (int i = 0; i < num_consumers; i++) {
        Operation *shutdown_signal = NULL;  // Use NULL or a special shutdown operation
        queue_put(q, shutdown_signal);
    }

    // Wait for all consumer threads to finish
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Output profit and stock
    printf("Total: %d euros\n", profits);
    printf("Stock:\n");
    for (int i = 0; i < 5; i++) {
        printf("  Product %d: %d\n", i + 1, product_stock[i]);
    }

    // Destroy queue
    queue_destroy(q);

    // Close file
    fclose(file);

    return EXIT_SUCCESS;
}

void *producer(void *arg) {
    FILE *file = (FILE *)arg;
    char line[MAX_FILENAME_LENGTH];

    while (fgets(line, MAX_FILENAME_LENGTH, file) != NULL) {
        Operation *op = parse_operation(line);
        if (op) {
            queue_put(q, op);
            free(op); // If your queue is storing copies, free the op here after putting it in the queue
        }
    }

    for (int i = 0; i < num_consumers; i++) {
        queue_put(q, NULL); // Send a shutdown signal for each consumer
    }

    pthread_exit(NULL);
}

void *consumer(void *arg) {
    Operation *op;
    
    while (1) {
        op = queue_get(q);
        if (op == NULL) break; // Assuming NULL is pushed to the queue when producers are done.

        process_operation(op);
        free(op);
    }

    pthread_exit(NULL);
}

Operation* parse_operation(const char *line) {
    Operation *op = (Operation *)malloc(sizeof(Operation));
    if (!op) {
        perror("Error allocating memory for operation");
        exit(EXIT_FAILURE); // Consider returning NULL instead of exiting to handle error gracefully in calling function
    }

    int scanCount = sscanf(line, "%d %s %d", &op->id, op->op_type, &op->units);
    if (scanCount != 3) {
        fprintf(stderr, "Failed to parse operation: %s\n", line);
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
        profits -= purchase_cost;
        product_stock[product_id] += op->units;
    } else if (strcmp(op->op_type, "SALE") == 0) {
        sale_price = sale_prices[product_id] * op->units;
        profits += sale_price;
        product_stock[product_id] -= op->units;
    } else {
        fprintf(stderr, "Invalid operation type: %s\n", op->op_type);
    }
}