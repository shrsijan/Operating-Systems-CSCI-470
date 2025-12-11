#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

/**
 * Convert string to uppercase in-place
 * 
 * This function modifies the input string directly by converting all 
 * lowercase characters to uppercase. Used for normalizing command-line
 * policy arguments (e.g., "-f" becomes "-F").
 * 
 * @param arr: null-terminated string to convert (modified in-place)
 * 
 * @note This function is named in lowercase to follow standard C convention
 *       (all-caps names typically indicate preprocessor macros)
 */
void str_to_upper(char * arr){
    for(int i = 0; i < strlen(arr); i++){
        arr[i] = toupper(arr[i]);
    }
}

/**
 * Parse command line arguments and input file
 * 
 * Reads the input file containing memory operations and determines the
 * memory management policy based on command-line arguments.
 * 
 * @param args: command line arguments array (argv from main)
 *              args[1] = input file path
 *              args[2] = policy flag (-F, -B, or -W)
 * @param input: 2D array to store parsed operations [operation][pid,size]
 * @param n: pointer to store number of operations read from file
 * @param size: pointer to store total partition size from file
 * @param policy: pointer to store memory management policy
 *                1 = FIFO (First-In-First-Out)
 *                2 = BESTFIT (smallest sufficient block)
 *                3 = WORSTFIT (largest available block)
 * 
 * @note The function modifies args[2] in-place (converts to uppercase).
 *       This is safe for this assignment's scope but may not be portable
 *       to all systems where argv might be read-only.
 * 
 * @exit Terminates program if file cannot be opened or invalid policy specified
 */
void get_input(char *args[], int input[][2], int *n, int *size, int *policy) 
{
    // Open and validate input file
    FILE *input_file = fopen(args[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(0);
    }

    // Parse memory operations from file
    parse_file(input_file, input, n, size);
    fclose(input_file);
    
    // Normalize policy argument to uppercase for comparison
    // NOTE: Modifies args[2] in-place - safe for this assignment
    str_to_upper(args[2]);
    
    // Determine memory management policy from command-line argument
    if((strcmp(args[2], "-F") == 0) || (strcmp(args[2], "-FIFO") == 0))
        *policy = 1;  // FIFO: use first sufficient block found
    else if((strcmp(args[2], "-B") == 0) || (strcmp(args[2], "-BESTFIT") == 0))
        *policy = 2;  // BESTFIT: use smallest sufficient block
    else if((strcmp(args[2], "-W") == 0) || (strcmp(args[2], "-WORSTFIT") == 0))
        *policy = 3;  // WORSTFIT: use largest available block
    else {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }
}

/**
 * Allocate memory for a process using specified policy
 * 
 * Searches the free list for a suitable block (size >= blocksize) according
 * to the memory management policy, allocates it to the process, and handles
 * any resulting memory fragmentation.
 * 
 * @param freelist: linked list of free (unallocated) memory blocks
 *                  Maintained in policy-specific order:
 *                  - FIFO: arbitrary order (first-found)
 *                  - BESTFIT: ascending by block size
 *                  - WORSTFIT: descending by block size
 * @param alloclist: linked list of allocated memory blocks
 *                   Always maintained in ascending order by start address
 * @param pid: process ID requesting memory (must be > 0)
 * @param blocksize: number of contiguous memory blocks requested
 * @param policy: memory management policy
 *                1 = FIFO (First-In-First-Out): allocate first sufficient block
 *                2 = BESTFIT: allocate smallest sufficient block
 *                3 = WORSTFIT: allocate largest available block
 * 
 * Algorithm Steps:
 * 1. Search freelist for block with size >= blocksize
 *    - Uses list_is_in_by_size() to verify existence
 *    - Uses list_get_index_of_by_Size() to find first match
 * 2. If no suitable block found, print error and return
 * 3. Remove selected block from freelist
 * 4. Assign block to process:
 *    - Set block->pid = pid
 *    - Resize block: block->end = block->start + blocksize - 1
 * 5. Add resized block to alloclist (sorted by address)
 * 6. Handle fragmentation (if any):
 *    - If original block was larger than needed, create fragment
 *    - Fragment starts at (new block->end + 1)
 *    - Fragment ends at (original block->end)
 *    - Add fragment back to freelist according to policy
 * 
 * Corner Cases Handled:
 * - No suitable block available: prints error, no allocation
 * - Exact fit (blocksize == available block size): no fragment created
 * - Partial fit: creates and adds fragment to maintain free memory
 * 
 * Memory Management Notes:
 * - Original block structure is reused for allocated block (no new malloc)
 * - Fragment is newly allocated (malloc) only when needed
 * - Fragment always has pid = 0 (marks it as free)
 * - No zero-sized or negative-sized blocks are ever created
 * 
 * @note For FIFO policy, list_get_index_of_by_Size returns first matching
 *       block found, regardless of list order
 * @note For BESTFIT/WORSTFIT, the sorted list ensures the first match is
 *       the optimal choice
 */
void allocate_memory(list_t * freelist, list_t * alloclist, int pid, int blocksize, int policy) {
    // Step 1: Verify sufficient free memory exists
    // This prevents attempting allocation when no block can satisfy request
    if (!list_is_in_by_size(freelist, blocksize)) {
        printf("Error: Memory Allocation %d blocks\n", blocksize);
        return;
    }
    
    // Step 2: Find and remove suitable block from freelist
    // For FIFO: returns first block with sufficient size
    // For BESTFIT: list is sorted ascending, so first match is smallest sufficient
    // For WORSTFIT: list is sorted descending, so first match is largest
    int block_index = list_get_index_of_by_Size(freelist, blocksize);
    block_t *blk = list_remove_at_index(freelist, block_index);
    
    // Step 3: Store original end address before modification
    // Needed to calculate fragment boundaries later
    int original_end = blk->end;
    
    // Step 4: Assign block to requesting process
    blk->pid = pid;
    // Resize block to exact requested size
    // Example: if blocksize=50 and blk->start=100, then blk->end=149
    blk->end = blk->start + blocksize - 1;
    
    // Step 5: Add allocated block to alloclist
    // Always sorted by address for easy visualization and coalescing later
    list_add_ascending_by_address(alloclist, blk);

    // Step 6: Handle memory fragmentation
    // Only create fragment if there's leftover memory after allocation
    // This check prevents creation of zero-sized or negative-sized blocks
    // Example: original_end=99, blk->end=99 → no fragment (exact fit)
    // Example: original_end=99, blk->end=49 → fragment [50-99] (size=50)
    if (blk->end < original_end) {
        // Allocate new block structure for the fragment
        block_t* fragment = (block_t *) malloc(sizeof(block_t));
        
        // Mark fragment as free memory
        fragment->pid = 0;
        
        // Set fragment boundaries
        // Fragment starts immediately after allocated block
        fragment->start = blk->end + 1;
        // Fragment ends where original block ended
        fragment->end = original_end;
        
        // Add fragment to freelist based on memory management policy
        if (policy == 1) {
            // FIFO: add to back of list (maintains insertion order)
            list_add_to_back(freelist, fragment);
        } else if (policy == 2) {
            // BESTFIT: maintain ascending order by block size
            // This ensures smallest sufficient block is always found first
            list_add_ascending_by_blocksize(freelist, fragment);
        } else if (policy == 3) {
            // WORSTFIT: maintain descending order by block size
            // This ensures largest available block is always found first
            list_add_descending_by_blocksize(freelist, fragment);
        }
    }
    // If no fragment needed (exact fit), we simply don't create one
    // This is the key fix that prevents zero-sized blocks in the freelist
}

/**
 * Deallocate memory from a process and return it to free pool
 * 
 * Searches the allocated list for a block belonging to the specified process,
 * removes it from the allocated list, marks it as free, and returns it to
 * the free list according to the memory management policy.
 * 
 * @param alloclist: linked list of allocated memory blocks
 *                   Maintained in ascending order by start address
 * @param freelist: linked list of free (unallocated) memory blocks
 *                  Order depends on policy (FIFO/BESTFIT/WORSTFIT)
 * @param pid: process ID whose memory should be deallocated
 * @param policy: memory management policy for reinserting freed block
 *                1 = FIFO: add to back of freelist
 *                2 = BESTFIT: insert in ascending order by size
 *                3 = WORSTFIT: insert in descending order by size
 * 
 * Algorithm Steps:
 * 1. Search alloclist for block with matching PID
 *    - Uses list_is_in_by_pid() to verify existence
 *    - Uses list_get_index_of_by_Pid() to find location
 * 2. If PID not found, print error and return (no deallocation)
 * 3. Remove block from alloclist
 * 4. Mark block as free (set pid = 0)
 * 5. Return block to freelist according to policy:
 *    - FIFO: append to end of list
 *    - BESTFIT: insert maintaining ascending size order
 *    - WORSTFIT: insert maintaining descending size order
 * 
 * Corner Cases Handled:
 * - PID not found in alloclist: prints error, no deallocation
 * - Multiple blocks with same PID: deallocates first found (as per spec)
 * - Empty alloclist: handled by list_is_in_by_pid returning false
 * 
 * Memory Management Notes:
 * - Block structure is reused (not freed/reallocated)
 * - Only the pid field is modified; start/end addresses unchanged
 * - Block maintains its original size and location in memory
 * - No new memory is allocated during deallocation
 * 
 * @note After deallocation, freed block remains fragmented until
 *       explicit coalescing operation is performed
 * @note Block size is preserved: (end - start + 1) remains constant
 */
void deallocate_memory(list_t * alloclist, list_t * freelist, int pid, int policy) { 
    // Step 1: Verify block with given PID exists in allocated list
    // Prevents attempting to deallocate non-existent or already-freed memory
    if (!list_is_in_by_pid(alloclist, pid)) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }
    
    // Step 2: Find and remove block from allocated list
    // list_get_index_of_by_Pid returns position of first block with matching PID
    int block_index = list_get_index_of_by_Pid(alloclist, pid);
    block_t *blk = list_remove_at_index(alloclist, block_index);
    
    // Step 3: Mark block as free by resetting PID to 0
    // Convention: pid = 0 indicates unallocated/free memory
    blk->pid = 0;
    
    // Step 4: Return freed block to freelist based on memory management policy
    if (policy == 1) {
        // FIFO: add to back of freelist
        // Maintains temporal ordering (most recently freed is at back)
        list_add_to_back(freelist, blk);
    } else if (policy == 2) {
        // BESTFIT: insert maintaining ascending order by block size
        // Ensures smallest sufficient block is always found first during allocation
        // Minimizes wasted space (internal fragmentation)
        list_add_ascending_by_blocksize(freelist, blk);
    } else if (policy == 3) {
        // WORSTFIT: insert maintaining descending order by block size
        // Ensures largest available block is always found first during allocation
        // Can help reduce external fragmentation in some scenarios
        list_add_descending_by_blocksize(freelist, blk);
    }
    // Note: Block may be adjacent to other free blocks but is NOT automatically
    // coalesced here. Coalescing must be explicitly triggered via coalese_memory().
}

/**
 * Coalesce adjacent free memory blocks to reduce fragmentation
 * 
 * Combines physically contiguous free blocks into larger blocks, reducing
 * external fragmentation and improving allocation efficiency. This is
 * critical for maintaining usable memory even after many allocations and
 * deallocations have fragmented the free list.
 * 
 * @param list: freelist containing free memory blocks (possibly fragmented)
 * @return: new freelist with coalesced blocks (replaces original)
 * 
 * Algorithm Steps:
 * 1. Create temporary list for sorted blocks
 * 2. Remove all blocks from input list and re-insert in address order
 *    - Uses list_add_ascending_by_address for sorting
 *    - Ensures adjacent blocks in memory are adjacent in list
 * 3. Call list_coalese_nodes to merge physically contiguous blocks
 *    - Iteratively combines blocks where block1.end + 1 == block2.start
 *    - Creates single larger block, frees merged block structures
 * 4. Return coalesced list (caller must update freelist pointer)
 * 
 * Example of Coalescing:
 * Before: [0-49, PID=0] [50-99, PID=0] [100-149, PID=0] (3 blocks)
 * After:  [0-149, PID=0]                                 (1 block)
 * 
 * Benefits:
 * - Reduces external fragmentation (many small unusable holes)
 * - Allows larger allocations that would otherwise fail
 * - Simplifies freelist management
 * - Improves allocation performance (fewer blocks to search)
 * 
 * Corner Cases Handled:
 * - Empty freelist: returns empty list
 * - Single block: returns same block (no coalescing needed)
 * - Non-adjacent blocks: preserved separately (no merging)
 * - Multiple contiguous sequences: each sequence fully merged
 * 
 * Memory Management Notes:
 * - Original list structure is emptied (all nodes removed)
 * - New list structure is allocated and returned
 * - Merged block structures are freed by list_coalese_nodes
 * - Caller must update their freelist pointer to returned value
 * 
 * @note This function is typically called in response to input "-99999 0"
 * @note Original list parameter becomes empty after this operation
 * @note Blocks must have pid=0 (free) - allocated blocks should not be here
 */
list_t* coalese_memory(list_t * list){
    // Create temporary list to hold sorted blocks
    list_t *temp_list = list_alloc();
    block_t *blk;
    
    // Step 1: Sort freelist by memory address (ascending)
    // This is CRITICAL - coalescing only works if adjacent blocks in memory
    // are also adjacent in the list. Without sorting, we would miss mergeable blocks.
    // Example: list might have [100-149] [0-49] [50-99]
    //          after sorting: [0-49] [50-99] [100-149]
    //          now adjacent blocks are next to each other for merging
    while((blk = list_remove_from_front(list)) != NULL) {
        list_add_ascending_by_address(temp_list, blk);
    }
    // At this point: original list is empty, temp_list is sorted by address
    
    // Step 2: Combine physically adjacent free blocks
    // Iterates through sorted list and merges blocks where:
    //   current_block.end + 1 == next_block.start
    // This indicates the blocks are physically contiguous in memory
    // Example: [0-49] and [50-99] are adjacent (49+1=50), so merge to [0-99]
    list_coalese_nodes(temp_list);
    
    // Step 3: Return new coalesced list
    // Caller (main function) must update FREE_LIST = coalese_memory(FREE_LIST)
    return temp_list;
}

/**
 * Print all blocks in a list
 * @param list: list to print
 * @param message: header message
 */
void print_list(list_t * list, char * message){
    node_t *current = list->head;
    block_t *blk;
    int i = 0;
    
    printf("%s:\n", message);
    
    while(current != NULL){
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d", i, blk->start, blk->end);
        
        if(blk->pid != 0)
            printf("\t PID: %d\n", blk->pid);
        else  
            printf("\n");
        
        current = current->next;
        i += 1;
    }
}

/* DO NOT MODIFY */
int main(int argc, char *argv[]) 
{
    int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;
    
    list_t *FREE_LIST = list_alloc();   // list that holds all free blocks (PID is always zero)
    list_t *ALLOC_LIST = list_alloc();  // list that holds all allocated blocks
    int i;
    
    if(argc != 3) {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }
    
    get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);
    
    // Allocated the initial partition of size PARTITION_SIZE
    block_t * partition = malloc(sizeof(block_t));   // create the partition meta data
    partition->start = 0;
    partition->end = PARTITION_SIZE + partition->start - 1;
    
    list_add_to_front(FREE_LIST, partition);          // add partition to free list
    
    for(i = 0; i < N; i++) // loop through all the input data and simulate a memory management policy
    {
        printf("************************\n");
        if(inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
            printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
            allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
        }
        else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
            printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
            deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
        }
        else {
            printf("COALESCE/COMPACT\n");
            FREE_LIST = coalese_memory(FREE_LIST);
        }   
        
        printf("************************\n");
        print_list(FREE_LIST, "Free Memory");
        print_list(ALLOC_LIST,"\nAllocated Memory");
        printf("\n\n");
    }
    
    list_free(FREE_LIST);
    list_free(ALLOC_LIST);
    
    return 0;
}