#include "BENSCHILLIBOWL.h"
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

static bool IsEmpty(BENSCHILLIBOWL* bcb) {
    bool isEmpty = (bcb->current_size == 0);
    printf("DEBUG: IsEmpty called. Returns %d (current size=%d)\n", isEmpty, bcb->current_size);
    return isEmpty;
}

static bool IsFull(BENSCHILLIBOWL* bcb) {
    bool isFull = (bcb->current_size == bcb->max_size);
    printf("DEBUG: IsFull called. Returns %d (current size=%d, max size=%d)\n", isFull, bcb->current_size, bcb->max_size);
    return isFull;
}

MenuItem PickRandomMenuItem() {
    int randomIndex = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[randomIndex];
}

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL* bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("The restaurant is now open!\n");
    return bcb;
}

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    printf("Orders handled: %d, Expected orders: %d\n", bcb->orders_handled, bcb->expected_num_orders);
    assert(bcb->orders_handled == bcb->expected_num_orders);

    Order* currentOrder = bcb->orders;
    while (currentOrder != NULL) {
        Order* temp = currentOrder;
        currentOrder = currentOrder->next;
        free(temp);
    }

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    free(bcb);
    printf("The restaurant is closed now.\n");
}

int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    int orderNumber = bcb->next_order_number++;
    order->order_number = orderNumber;

    if (bcb->orders == NULL) {
        bcb->orders = order;
    } else {
        Order* current = bcb->orders;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = order;
    }
    order->next = NULL;
    bcb->current_size++;

    printf("DEBUG: Order #%d added for Customer %d. Queue size: %d\n",
           order->order_number, order->customer_id, bcb->current_size);

    pthread_cond_broadcast(&bcb->can_get_orders);
    pthread_mutex_unlock(&bcb->mutex);
    return orderNumber;
}

Order* GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsEmpty(bcb) && bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    if (IsEmpty(bcb) && bcb->orders_handled == bcb->expected_num_orders) {
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    Order* order = bcb->orders;
    bcb->orders = order->next;
    bcb->current_size--;
    bcb->orders_handled++;

    printf("DEBUG: Order #%d retrieved. Current queue size: %d, Orders handled: %d\n",
           order->order_number, bcb->current_size, bcb->orders_handled);

    pthread_cond_broadcast(&bcb->can_add_orders);
    pthread_mutex_unlock(&bcb->mutex);
    return order;
}
