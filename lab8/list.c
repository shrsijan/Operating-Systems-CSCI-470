#include <stdio.h>
#include <stdlib.h>
#include "list.h"

/**
 * Allocate a new list structure
 * @return: pointer to newly allocated list
 */
list_t *list_alloc() { 
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list != NULL) {
        list->head = NULL;
    }
    return list;
}

/**
 * Allocate a new node structure
 * @return: pointer to newly allocated node
 */
node_t *node_alloc(block_t *blk) {   
    node_t *node = (node_t *)malloc(sizeof(node_t));
    if (node != NULL) {
        node->next = NULL;
        node->blk = blk;
    }
    return node;
}

/**
 * Free a list and all its nodes and blocks
 * CRITICAL: This prevents memory leaks by freeing all allocated memory
 * @param l: list to free
 */
void list_free(list_t *l){
    if (l == NULL) return;
    
    node_t *current = l->head;
    while (current != NULL) {
        node_t *next = current->next;
        // Free the block data
        if (current->blk != NULL) {
            free(current->blk);
        }
        // Free the node itself
        free(current);
        current = next;
    }
    // Free the list structure
    free(l);
}

/**
 * Free a single node (but not the block)
 * @param n: node to free
 */
void node_free(node_t *n){
    if (n != NULL) {
        free(n);
    }
}

/**
 * Print all blocks in the list
 * @param l: list to print
 */
void list_print(list_t *l) {
    if (l == NULL) return;
    
    node_t *current = l->head;
    block_t *block;
    int i = 0;
    
    while (current != NULL) {
        block = current->blk;
        printf("Block %d:\tSTART: %d\tEND: %d\tSIZE: %d", 
               i, block->start, block->end, block->end - block->start + 1);
        
        if (block->pid != 0) {
            printf("\tPID: %d\n", block->pid);
        } else {
            printf("\n");
        }
        
        current = current->next;
        i++;
    }
}

/**
 * Get the length of the list
 * @param l: list
 * @return: number of nodes in list
 */
int list_length(list_t *l) { 
    if (l == NULL) return 0;
    
    int count = 0;
    node_t *current = l->head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

/**
 * Add block to front of list
 * @param l: list
 * @param blk: block to add
 */
void list_add_to_front(list_t *l, block_t *blk){  
    if (l == NULL || blk == NULL) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    new_node->next = l->head;
    l->head = new_node;
}

/**
 * Add block to back of list
 * @param l: list
 * @param blk: block to add
 */
void list_add_to_back(list_t *l, block_t *blk) {  
    if (l == NULL || blk == NULL) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    // Empty list case
    if (l->head == NULL) {
        l->head = new_node;
        return;
    }
    
    // Traverse to end of list
    node_t *current = l->head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
}

/**
 * Add block at specific index
 * @param l: list
 * @param blk: block to add
 * @param index: position to insert (0-indexed)
 */
void list_add_at_index(list_t *l, block_t *blk, int index){
    if (l == NULL || blk == NULL || index < 0) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    // Insert at head
    if (index == 0) {
        new_node->next = l->head;
        l->head = new_node;
        return;
    }
    
    // Traverse to position before insertion point
    node_t *current = l->head;
    for (int i = 0; i < index - 1 && current != NULL; i++) {
        current = current->next;
    }
    
    // If index is beyond list length, add to end
    if (current == NULL) return;
    
    new_node->next = current->next;
    current->next = new_node;
}

/**
 * Remove and return block from front of list
 * @param l: list
 * @return: removed block, or NULL if list empty
 */
block_t *list_remove_from_front(list_t *l) { 
    if (l == NULL || l->head == NULL) return NULL;
    
    node_t *old_head = l->head;
    block_t *blk = old_head->blk;
    
    l->head = old_head->next;
    node_free(old_head);
    
    return blk;
}

/**
 * Remove and return block from back of list
 * @param l: list
 * @return: removed block, or NULL if list empty
 */
block_t *list_remove_from_back(list_t *l) {  
    if (l == NULL || l->head == NULL) return NULL;
    
    // Single element case
    if (l->head->next == NULL) {
        block_t *blk = l->head->blk;
        node_free(l->head);
        l->head = NULL;
        return blk;
    }
    
    // Traverse to second-to-last node
    node_t *current = l->head;
    while (current->next->next != NULL) {
        current = current->next;
    }
    
    block_t *blk = current->next->blk;
    node_free(current->next);
    current->next = NULL;
    
    return blk;
}

/**
 * Remove and return block at specific index
 * @param l: list
 * @param index: position to remove (0-indexed)
 * @return: removed block, or NULL if invalid index
 */
block_t *list_remove_at_index(list_t *l, int index) { 
    if (l == NULL || l->head == NULL || index < 0) return NULL;
    
    // Remove from head
    if (index == 0) {
        return list_remove_from_front(l);
    }
    
    // Traverse to node before target
    node_t *current = l->head;
    for (int i = 0; i < index - 1 && current->next != NULL; i++) {
        current = current->next;
    }
    
    // Index out of bounds
    if (current->next == NULL) return NULL;
    
    node_t *target = current->next;
    block_t *blk = target->blk;
    current->next = target->next;
    node_free(target);
    
    return blk;
}

/**
 * Check if list contains block with given PID
 * @param l: list
 * @param pid: process ID to search for
 * @return: 1 if found, 0 otherwise
 */
int list_is_in_by_pid(list_t *l, int pid) { 
    if (l == NULL) return 0;
    
    node_t *current = l->head;
    while (current != NULL) {
        if (current->blk->pid == pid) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

/**
 * Check if list contains block with size >= given size
 * @param l: list
 * @param size: minimum block size to search for
 * @return: 1 if found, 0 otherwise
 */
int list_is_in_by_size(list_t *l, int size) { 
    if (l == NULL) return 0;
    
    node_t *current = l->head;
    while (current != NULL) {
        int block_size = current->blk->end - current->blk->start + 1;
        if (block_size >= size) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

/**
 * Get index of block with given PID
 * @param l: list
 * @param pid: process ID to search for
 * @return: index of block, or -1 if not found
 */
int list_get_index_of_by_Pid(list_t *l, int pid) {
    if (l == NULL) return -1;
    
    node_t *current = l->head;
    int index = 0;
    
    while (current != NULL) {
        if (current->blk->pid == pid) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

/**
 * Get index of first block with size >= given size
 * For FIFO policy (finds first suitable block)
 * @param l: list
 * @param size: minimum block size to search for
 * @return: index of block, or -1 if not found
 */
int list_get_index_of_by_Size(list_t *l, int size) {
    if (l == NULL) return -1;
    
    node_t *current = l->head;
    int index = 0;
    
    while (current != NULL) {
        int block_size = current->blk->end - current->blk->start + 1;
        if (block_size >= size) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

/**
 * Add block in ascending order by memory address
 * Used for allocated list and coalescing
 * @param l: list
 * @param blk: block to add
 */
void list_add_ascending_by_address(list_t *l, block_t *blk) {
    if (l == NULL || blk == NULL) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    // Empty list or insert at head
    if (l->head == NULL || blk->start < l->head->blk->start) {
        new_node->next = l->head;
        l->head = new_node;
        return;
    }
    
    // Find insertion point
    node_t *current = l->head;
    while (current->next != NULL && current->next->blk->start < blk->start) {
        current = current->next;
    }
    
    new_node->next = current->next;
    current->next = new_node;
}

/**
 * Add block in ascending order by block size (BESTFIT policy)
 * Smaller blocks first
 * @param l: list
 * @param blk: block to add
 */
void list_add_ascending_by_blocksize(list_t *l, block_t *blk) {
    if (l == NULL || blk == NULL) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    int new_size = blk->end - blk->start + 1;
    
    // Empty list or insert at head
    if (l->head == NULL) {
        l->head = new_node;
        return;
    }
    
    int head_size = l->head->blk->end - l->head->blk->start + 1;
    if (new_size < head_size) {
        new_node->next = l->head;
        l->head = new_node;
        return;
    }
    
    // Find insertion point
    node_t *current = l->head;
    while (current->next != NULL) {
        int next_size = current->next->blk->end - current->next->blk->start + 1;
        if (new_size < next_size) {
            break;
        }
        current = current->next;
    }
    
    new_node->next = current->next;
    current->next = new_node;
}

/**
 * Add block in descending order by block size (WORSTFIT policy)
 * Larger blocks first
 * @param l: list
 * @param blk: block to add
 */
void list_add_descending_by_blocksize(list_t *l, block_t *blk) {
    if (l == NULL || blk == NULL) return;
    
    node_t *new_node = node_alloc(blk);
    if (new_node == NULL) return;
    
    int new_size = blk->end - blk->start + 1;
    
    // Empty list or insert at head
    if (l->head == NULL) {
        l->head = new_node;
        return;
    }
    
    int head_size = l->head->blk->end - l->head->blk->start + 1;
    if (new_size > head_size) {
        new_node->next = l->head;
        l->head = new_node;
        return;
    }
    
    // Find insertion point
    node_t *current = l->head;
    while (current->next != NULL) {
        int next_size = current->next->blk->end - current->next->blk->start + 1;
        if (new_size > next_size) {
            break;
        }
        current = current->next;
    }
    
    new_node->next = current->next;
    current->next = new_node;
}

/**
 * Coalesce adjacent memory blocks in the list
 * CRITICAL: List must be sorted by address before calling this function
 * Merges physically contiguous blocks into single larger blocks
 * @param l: list sorted by address
 */
void list_coalese_nodes(list_t *l) {
    if (l == NULL || l->head == NULL) return;
    
    node_t *current = l->head;
    
    while (current != NULL && current->next != NULL) {
        block_t *current_block = current->blk;
        block_t *next_block = current->next->blk;
        
        // Check if blocks are adjacent (current.end + 1 == next.start)
        if (current_block->end + 1 == next_block->start) {
            // Merge: extend current block to include next block
            current_block->end = next_block->end;
            
            // Remove next node from list
            node_t *temp = current->next;
            current->next = temp->next;
            
            // Free the merged block and node
            free(temp->blk);
            node_free(temp);
            
            // Don't advance current - check if we can merge with new next
        } else {
            // Blocks not adjacent, move to next
            current = current->next;
        }
    }
}