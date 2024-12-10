#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "BENSCHILLIBOWL.h"

/**
 * Default values can be overridden using command-line arguments.
 */
int NUM_CUSTOMERS = 90;
int NUM_COOKS = 10;
int ORDERS_PER_CUSTOMER = 3;
int BENSCHILLIBOWL_SIZE = 100;
int EXPECTED_NUM_ORDERS;

BENSCHILLIBOWL *bcb;

void* CustomerThread(void* tid) {
    int customer_id = (int)(long) tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        Order *order = (Order *)malloc(sizeof(Order));
        order->customer_id = customer_id;
        order->menu_item = PickRandomMenuItem();
        order->next = NULL;

        int order_number = AddOrder(bcb, order);
        if (order_number < 0) {
            // Failed to add order
            free(order);
        }
    }
    return NULL;
}

void* CookThread(void* tid) {
    int cook_id = (int)(long) tid;
    int fulfilled_orders = 0;

    while (1) {
        Order* order = GetOrder(bcb);
        if (order == NULL) {
            // No orders left to process
            break;
        }
        free(order);
        fulfilled_orders++;
    }

    printf("Cook #%d processed %d orders\n", cook_id, fulfilled_orders);
    return NULL;
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    if (argc > 1) NUM_CUSTOMERS = atoi(argv[1]);
    if (argc > 2) NUM_COOKS = atoi(argv[2]);
    if (argc > 3) ORDERS_PER_CUSTOMER = atoi(argv[3]);
    if (argc > 4) BENSCHILLIBOWL_SIZE = atoi(argv[4]);

    EXPECTED_NUM_ORDERS = NUM_CUSTOMERS * ORDERS_PER_CUSTOMER;

    printf("Scenario: %d customers, %d cooks, %d orders per customer, queue size %d\n",
           NUM_CUSTOMERS, NUM_COOKS, ORDERS_PER_CUSTOMER, BENSCHILLIBOWL_SIZE);

    srand(time(NULL)); // Initialize random seed

    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

    pthread_t customer_threads[NUM_CUSTOMERS];
    pthread_t cook_threads[NUM_COOKS];

    // Launch cook threads
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_create(&cook_threads[i], NULL, CookThread, (void*)(long)(i + 1));
    }

    // Launch customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customer_threads[i], NULL, CustomerThread, (void*)(long)(i + 1));
    }

    // Wait for customer threads to complete
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customer_threads[i], NULL);
    }

    // Wait for cook threads to complete
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cook_threads[i], NULL);
    }

    CloseRestaurant(bcb);
    return 0;
}
