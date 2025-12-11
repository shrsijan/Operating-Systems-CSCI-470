#ifndef LIST_H
#define LIST_H

/**
 * Memory block structure
 * Represents a contiguous block of memory
 */
typedef struct block {
    int pid;      // Process ID (0 = free block)
    int start;    // Starting address
    int end;      // Ending address (inclusive)
} block_t;

/**
 * Linked list node structure
 */
typedef struct node {
    block_t *blk;         // Pointer to memory block
    struct node *next;    // Pointer to next node
} node_t;

/**
 * Linked list structure
 */
typedef struct list {
    node_t *head;    // Pointer to first node
} list_t;

/* List allocation and deallocation */
list_t *list_alloc();
node_t *node_alloc(block_t *blk);
void list_free(list_t *l);
void node_free(node_t *n);

/* List utilities */
void list_print(list_t *l);
int list_length(list_t *l);

/* Basic list operations */
void list_add_to_front(list_t *l, block_t *blk);
void list_add_to_back(list_t *l, block_t *blk);
void list_add_at_index(list_t *l, block_t *blk, int index);

block_t *list_remove_from_front(list_t *l);
block_t *list_remove_from_back(list_t *l);
block_t *list_remove_at_index(list_t *l, int index);

/* Search operations */
int list_is_in_by_pid(list_t *l, int pid);
int list_is_in_by_size(list_t *l, int size);
int list_get_index_of_by_Pid(list_t *l, int pid);
int list_get_index_of_by_Size(list_t *l, int size);

/* Sorted insertion operations */
void list_add_ascending_by_address(list_t *l, block_t *blk);
void list_add_ascending_by_blocksize(list_t *l, block_t *blk);
void list_add_descending_by_blocksize(list_t *l, block_t *blk);

/* Memory coalescing */
void list_coalese_nodes(list_t *l);

#endif