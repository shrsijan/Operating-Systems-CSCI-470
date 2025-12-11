#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

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

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    // Allocate memory for the restaurant
    BENSCHILLIBOWL* bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    
    // Initialize all variables
    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;
    
    // Initialize synchronization objects
    pthread_mutex_init(&(bcb->mutex), NULL);
    pthread_cond_init(&(bcb->can_add_orders), NULL);
    pthread_cond_init(&(bcb->can_get_orders), NULL);
    
    printf("Restaurant is open!\n");
    return bcb;
}

/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    // Check that orders handled matches expected
    if (bcb->orders_handled != bcb->expected_num_orders) {
        fprintf(stderr, "Warning: Expected %d orders, but handled %d\n", 
                bcb->expected_num_orders, bcb->orders_handled);
    }
    
    // Free any remaining orders in the queue
    while (bcb->orders != NULL) {
        Order* temp = bcb->orders;
        bcb->orders = bcb->orders->next;
        free(temp);
    }
    
    // Destroy synchronization objects
    pthread_mutex_destroy(&(bcb->mutex));
    pthread_cond_destroy(&(bcb->can_add_orders));
    pthread_cond_destroy(&(bcb->can_get_orders));
    
    // Free the restaurant
    free(bcb);
    
    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    // Acquire the lock
    pthread_mutex_lock(&(bcb->mutex));
    
    // Wait while the restaurant is full
    while (IsFull(bcb)) {
        pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
    }
    
    // Assign order number
    order->order_number = bcb->next_order_number;
    bcb->next_order_number++;
    
    // Add order to the back of the queue
    AddOrderToBack(&(bcb->orders), order);
    bcb->current_size++;
    
    // Signal that there are orders available to get
    pthread_cond_signal(&(bcb->can_get_orders));
    
    // Release the lock
    pthread_mutex_unlock(&(bcb->mutex));
    
    return order->order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    // Acquire the lock
    pthread_mutex_lock(&(bcb->mutex));
    
    // Wait while the restaurant is empty AND we haven't handled all expected orders
    while (IsEmpty(bcb)) {
        // If all orders have been handled, signal others and return NULL
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_cond_broadcast(&(bcb->can_get_orders));
            pthread_mutex_unlock(&(bcb->mutex));
            return NULL;
        }
        pthread_cond_wait(&(bcb->can_get_orders), &(bcb->mutex));
    }
    
    // Double check there's an order (safety check)
    if (bcb->orders == NULL) {
        pthread_cond_broadcast(&(bcb->can_get_orders));
        pthread_mutex_unlock(&(bcb->mutex));
        return NULL;
    }
    
    // Get order from the front of the queue
    Order* order = bcb->orders;
    bcb->orders = bcb->orders->next;
    bcb->current_size--;
    bcb->orders_handled++;
    
    // Signal that there is space to add orders
    pthread_cond_signal(&(bcb->can_add_orders));
    
    // Release the lock
    pthread_mutex_unlock(&(bcb->mutex));
    
    return order;
}

// Optional helper functions
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    order->next = NULL;
    
    // If queue is empty, this order becomes the head
    if (*orders == NULL) {
        *orders = order;
    } else {
        // Traverse to the end of the queue
        Order* current = *orders;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = order;
    }
}