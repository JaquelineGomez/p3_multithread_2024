//SSOO-P3 23/24

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
//#include <sys/wait.h>

typedef struct {
    int product_id;
    char operation_type[10]; // Assuming "PURCHASE" or "SALE" as possible operations
    int units;
} operation_data;

int main (int argc, const char * argv[])
{
  int profits = 0;
  int product_stock [5] = {0};

  // Purchase costs and selling prices
    int purchase_costs[5] = {2, 5, 15, 25, 100};
    int selling_prices[5] = {3, 10, 20, 40, 125};

    if(argc < 5) {
        fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int buff_size = atoi(argv[4]);

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    int operations_count;
    fscanf(file, "%d", &operations_count);

    operation_data *operations = malloc(operations_count * sizeof(operation_data));
    if (!operations) {
        perror("Failed to allocate memory for operations");
        fclose(file);
        return EXIT_FAILURE;
    }

    for(int i = 0; i < operations_count; i++) {
        fscanf(file, "%d %s %d", &operations[i].product_id, operations[i].operation_type, &operations[i].units);
    }

    fclose(file);

    // Processing each operation
    for (int i = 0; i < operations_count; i++) {
        int product_index = operations[i].product_id - 1; // Product ID to array index (1-based to 0-based)

        if (strcmp(operations[i].operation_type, "PURCHASE") == 0) {
            product_stock[product_index] += operations[i].units;
        } else if (strcmp(operations[i].operation_type, "SALE") == 0) {
            product_stock[product_index] -= operations[i].units;
            profits += operations[i].units * (selling_prices[product_index] - purchase_costs[product_index]);
        }
    }


  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  return 0;
}
