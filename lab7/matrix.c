/*
 * Parallel Matrix Operations with Pthreads
 * 
 * Performs sum, difference, and product on two 20x20 matrices using 10 threads per operation.
 * Work is evenly distributed across threads with last thread handling any remainder cells.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX 20
#define NUM_THREADS 10

int matA[MAX][MAX]; 
int matB[MAX][MAX]; 

int matSumResult[MAX][MAX];
int matDiffResult[MAX][MAX]; 
int matProductResult[MAX][MAX];

// Thread parameter structure
typedef struct {
    int thread_id;
    int start_cell;
    int end_cell;
} ThreadData;

void fillMatrix(int matrix[MAX][MAX]) {
    for(int i = 0; i < MAX; i++) {
        for(int j = 0; j < MAX; j++) {
            matrix[i][j] = rand() % 10 + 1;
        }
    }
}

void printMatrix(int matrix[MAX][MAX]) {
    for(int i = 0; i < MAX; i++) {
        for(int j = 0; j < MAX; j++) {
            printf("%5d", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Compute matrix sum: matSumResult[i][j] = matA[i][j] + matB[i][j]
void* computeSum(void* args) {
    ThreadData *data = (ThreadData *)args;
    
    for (int cell = data->start_cell; cell <= data->end_cell; cell++) {
        int row = cell / MAX;
        int col = cell % MAX;
        matSumResult[row][col] = matA[row][col] + matB[row][col];
    }
    
    free(args);
    return NULL;
}

// Compute matrix difference: matDiffResult[i][j] = matA[i][j] - matB[i][j]
void* computeDiff(void* args) {
    ThreadData *data = (ThreadData *)args;
    
    for (int cell = data->start_cell; cell <= data->end_cell; cell++) {
        int row = cell / MAX;
        int col = cell % MAX;
        matDiffResult[row][col] = matA[row][col] - matB[row][col];
    }
    
    free(args);
    return NULL;
}

// Compute matrix product: matProductResult[i][j] = sum of matA[i][k] * matB[k][j]
void* computeProduct(void* args) {
    ThreadData *data = (ThreadData *)args;
    
    for (int cell = data->start_cell; cell <= data->end_cell; cell++) {
        int row = cell / MAX;
        int col = cell % MAX;
        
        int sum = 0;
        for (int k = 0; k < MAX; k++) {
            sum += matA[row][k] * matB[k][col];
        }
        matProductResult[row][col] = sum;
    }
    
    free(args);
    return NULL;
}

// Helper: Create NUM_THREADS threads for given operation
void create_threads(pthread_t threads[], void* (*thread_func)(void*), int total_cells) {
    int cells_per_thread = total_cells / NUM_THREADS;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        if (!data) {
            fprintf(stderr, "Error: Failed to allocate memory for thread %d\n", i);
            exit(1);
        }
        
        data->thread_id = i;
        data->start_cell = i * cells_per_thread;
        // Last thread handles remainder cells
        data->end_cell = (i == NUM_THREADS - 1) ? (total_cells - 1) : ((i + 1) * cells_per_thread - 1);
        
        if (pthread_create(&threads[i], NULL, thread_func, (void *)data) != 0) {
            fprintf(stderr, "Error: Failed to create thread %d\n", i);
            free(data);
            exit(1);
        }
    }
}

// Helper: Join all threads
void join_threads(pthread_t threads[]) {
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error: Failed to join thread %d\n", i);
            exit(1);
        }
    }
}

/*
 * Main: Spawns 30 threads total (10 per operation) to compute matrix operations in parallel.
 * 
 * Why parallel execution of all operations:
 * - Demonstrates true concurrency (all 30 threads can run simultaneously)
 * - More efficient on multi-core systems
 * - Shows independence of operations (no dependencies between sum/diff/product)
 */
int main() {
    srand(time(0));  // Do Not Remove. Just ignore and continue below.
    
    // 1. Fill the matrices (matA and matB) with random values.
    fillMatrix(matA);
    fillMatrix(matB);
    
    // 2. Print the initial matrices.
    printf("========================================\n");
    printf("           INITIAL MATRICES\n");
    printf("========================================\n\n");
    
    printf("Matrix A:\n");
    printMatrix(matA);
    
    printf("Matrix B:\n");
    printMatrix(matB);
    
    // 3. Create pthread_t objects for our threads.
    pthread_t sum_threads[NUM_THREADS];
    pthread_t diff_threads[NUM_THREADS];
    pthread_t product_threads[NUM_THREADS];
    
    int total_cells = MAX * MAX;
    
    // 4. Create threads for each operation (30 threads total).
    // All operations run in parallel for efficiency.
    create_threads(sum_threads, computeSum, total_cells);
    create_threads(diff_threads, computeDiff, total_cells);
    create_threads(product_threads, computeProduct, total_cells);
    
    // 5. Wait for all threads to finish.
    join_threads(sum_threads);
    join_threads(diff_threads);
    join_threads(product_threads);
    
    // 6. Print the results.
    printf("========================================\n");
    printf("         COMPUTATION RESULTS\n");
    printf("========================================\n\n");
    
    printf("Sum (A + B):\n");
    printMatrix(matSumResult);
    
    printf("Difference (A - B):\n");
    printMatrix(matDiffResult);
    
    printf("Product (A × B):\n");
    printMatrix(matProductResult);
    
    printf("========================================\n");
    printf("All computations completed successfully.\n");
    printf("Total threads used: %d (%d per operation)\n", NUM_THREADS * 3, NUM_THREADS);
    printf("========================================\n");
    
    return 0;
}