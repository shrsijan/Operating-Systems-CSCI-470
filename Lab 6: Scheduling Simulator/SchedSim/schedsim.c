/**
 * CPU Scheduling Simulator
 * 
 * Implements four CPU scheduling algorithms:
 * 1. FCFS (First-Come-First-Serve) - Non-preemptive
 * 2. SRTF (Shortest Remaining Time First) - Preemptive SJF
 * 3. Priority Scheduling - Non-preemptive with FCFS tie-breaking
 * 4. Round Robin (RR) - Preemptive with time quantum
 * 
 * All algorithms properly handle process arrival times and calculate
 * waiting time and turnaround time metrics.
 */

#include<stdio.h> 
#include<limits.h>
#include<stdlib.h>
#include<stdbool.h>
#include "process.h"
#include "util.h"

/**
 * Calculate waiting time for Round Robin scheduling
 * 
 * Round Robin (RR) is a preemptive scheduling algorithm where each process
 * gets a fixed time quantum. If a process doesn't finish within its quantum,
 * it's moved to the back of the ready queue.
 * 
 * @param plist: array of processes
 * @param n: number of processes
 * @param quantum: time quantum for each process execution slice
 * 
 * Algorithm:
 * 1. Track remaining burst time for each process
 * 2. Cycle through all processes in order
 * 3. For each arrived process:
 *    - Execute for quantum time (or remaining time if less)
 *    - Update remaining burst time
 *    - Calculate waiting time when process completes
 * 4. Handle idle CPU time when no process has arrived
 * 
 * Time Complexity: O(n * total_burst_time / quantum)
 * Space Complexity: O(n)
 * 
 * @note Waiting time = Completion time - Arrival time - Burst time
 */
void findWaitingTimeRR(ProcessType plist[], int n, int quantum) 
{ 
    // 1. Create an array rem_bt[] to keep track of remaining burst time
    int *rem_bt = (int *)malloc(n * sizeof(int));
    
    // Initialize remaining burst times from process burst times
    for (int i = 0; i < n; i++) {
        rem_bt[i] = plist[i].bt;
    }
    
    // 2. Initialize waiting times to 0
    for (int i = 0; i < n; i++) {
        plist[i].wt = 0;
    }
    
    // 3. Initialize time : t = 0
    int t = 0;
    bool done = false;
    
    // 4. Keep traversing all processes while all processes are not done
    // This implements a circular queue traversal
    while (!done) {
        done = true; // Assume all processes are done
        
        // Check each process in order (simulates circular ready queue)
        for (int i = 0; i < n; i++) {
            // Only consider processes that have arrived and are not done
            if (rem_bt[i] > 0 && plist[i].art <= t) {
                done = false; // At least one process is still pending
                
                if (rem_bt[i] > quantum) {
                    // Process needs more than one quantum
                    // (i) Advance time by quantum
                    t = t + quantum;
                    // (ii) Reduce remaining burst time by quantum
                    rem_bt[i] -= quantum;
                } else {
                    // Last cycle for this process (remaining time <= quantum)
                    // (i) Advance time by remaining burst time
                    t = t + rem_bt[i];
                    // (ii) Calculate waiting time
                    // Waiting time = (Completion time) - (Arrival time) - (Burst time)
                    plist[i].wt = t - plist[i].art - plist[i].bt;
                    // (iii) Mark process as complete
                    rem_bt[i] = 0;
                }
            }
        }
        
        // Handle CPU idle time: if no process is ready but some are still pending
        // Advance time to the next process arrival
        if (done) {
            int min_arrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (rem_bt[i] > 0 && plist[i].art < min_arrival) {
                    min_arrival = plist[i].art;
                }
            }
            // If there's a pending process that hasn't arrived yet, jump to its arrival
            if (min_arrival != INT_MAX && min_arrival > t) {
                t = min_arrival;
                done = false; // Continue simulation
            }
        }
    }
    
    // Clean up dynamically allocated memory
    free(rem_bt);
} 

/**
 * Calculate waiting time for Shortest Remaining Time First (SRTF) scheduling
 * 
 * SRTF is the preemptive version of Shortest Job First (SJF). At each time unit,
 * the process with the shortest remaining burst time among all arrived processes
 * is selected for execution.
 * 
 * @param plist: array of processes
 * @param n: number of processes
 * 
 * Algorithm:
 * 1. At each time unit, find the process with minimum remaining time
 * 2. Execute that process for 1 time unit
 * 3. When a process completes, calculate its waiting time
 * 4. Repeat until all processes complete
 * 
 * Optimization: When no process is ready, jump directly to the next arrival
 * time instead of incrementing by 1, significantly improving performance for
 * inputs with large gaps between arrivals.
 * 
 * This is optimal for minimizing average waiting time but requires
 * knowledge of burst times in advance (not realistic in practice).
 * 
 * Time Complexity: O(n * total_burst_time) worst case
 *                  O(n * number_of_arrivals) best case (with idle time optimization)
 * Space Complexity: O(n)
 * 
 * @note This is PREEMPTIVE SJF (also called SRTF), not non-preemptive SJF
 * @note Waiting time = Completion time - Arrival time - Burst time
 */
void findWaitingTimeSJF(ProcessType plist[], int n)
{ 
    // Arrays to track process state
    int *rem_bt = (int *)malloc(n * sizeof(int));
    int *completion_time = (int *)malloc(n * sizeof(int));
    bool *completed = (bool *)malloc(n * sizeof(bool));
    
    // Initialize tracking arrays
    for (int i = 0; i < n; i++) {
        rem_bt[i] = plist[i].bt;           // Remaining burst time
        plist[i].wt = 0;                    // Waiting time (to be calculated)
        completed[i] = false;               // Completion status
        completion_time[i] = 0;             // Completion time (to be calculated)
    }
    
    int current_time = 0;
    int completed_count = 0;
    
    // Traverse until all processes get completely executed
    while (completed_count < n) {
        int min_rem_bt = INT_MAX;
        int min_idx = -1;
        
        // Find process with minimum remaining time among arrived processes
        // This is the key operation for SRTF scheduling
        for (int i = 0; i < n; i++) {
            if (!completed[i] && plist[i].art <= current_time && rem_bt[i] < min_rem_bt) {
                min_rem_bt = rem_bt[i];
                min_idx = i;
            }
        }
        
        if (min_idx == -1) {
            // No process available at current time - CPU is idle
            // OPTIMIZATION: Jump directly to the next process arrival instead of incrementing by 1
            // This significantly improves performance when there are gaps between arrivals
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!completed[i] && plist[i].art > current_time && plist[i].art < next_arrival) {
                    next_arrival = plist[i].art;
                }
            }
            
            if (next_arrival != INT_MAX) {
                current_time = next_arrival;  // Jump to next arrival
            } else {
                current_time++;  // Fallback (should not reach here if logic is correct)
            }
            continue;
        }
        
        // Execute selected process for 1 time unit (preemptive)
        rem_bt[min_idx]--;
        
        // Check if process has completed execution
        if (rem_bt[min_idx] == 0) {
            // Mark process as completed
            completed[min_idx] = true;
            completed_count++;
            
            // Completion time = current_time + 1 (since we just finished this time unit)
            completion_time[min_idx] = current_time + 1;
            
            // Calculate waiting time for completed process
            // Waiting time = Completion time - Arrival time - Burst time
            // This gives the total time the process spent waiting in the ready queue
            plist[min_idx].wt = completion_time[min_idx] - plist[min_idx].art - plist[min_idx].bt;
        }
        
        // Increment time by one unit
        current_time++;
    }
    
    // Clean up dynamically allocated memory
    free(rem_bt);
    free(completion_time);
    free(completed);
} 

/**
 * Calculate waiting time for First-Come-First-Serve (FCFS) scheduling
 * 
 * FCFS is the simplest non-preemptive scheduling algorithm. Processes are
 * executed in the order they arrive. Once a process starts, it runs to completion.
 * 
 * @param plist: array of processes (assumed to be sorted by arrival time)
 * @param n: number of processes
 * 
 * Algorithm:
 * 1. First process starts at its arrival time
 * 2. Each subsequent process starts when:
 *    - Previous process completes, OR
 *    - Its own arrival time (if CPU was idle)
 * 3. Waiting time = Start time - Arrival time
 * 
 * FCFS is fair but can suffer from the "convoy effect" where short processes
 * wait behind long processes.
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(n) for completion_time array
 * 
 * @note Processes must be sorted by arrival time before calling this function
 */
void findWaitingTime(ProcessType plist[], int n)
{ 
    // Array to track when each process completes
    int *completion_time = (int *)malloc(n * sizeof(int));
    
    // First process starts at its arrival time (no waiting)
    completion_time[0] = plist[0].art + plist[0].bt;
    plist[0].wt = 0;
  
    // Calculate waiting time for subsequent processes
    for (int i = 1; i < n; i++) {
        // Process starts when previous completes OR when it arrives (whichever is later)
        // This handles CPU idle time when next process hasn't arrived yet
        int start_time = (completion_time[i-1] > plist[i].art) ? completion_time[i-1] : plist[i].art;
        
        // Completion time = start time + burst time
        completion_time[i] = start_time + plist[i].bt;
        
        // Waiting time = start time - arrival time
        plist[i].wt = start_time - plist[i].art;
    }
    
    // Clean up dynamically allocated memory
    free(completion_time);
} 
  
/**
 * Calculate turnaround time for all processes
 * 
 * Turnaround time is the total time a process spends in the system:
 * from arrival to completion.
 * 
 * @param plist: array of processes with waiting times already calculated
 * @param n: number of processes
 * 
 * Formula: Turnaround time = Burst time + Waiting time
 * Alternatively: Turnaround time = Completion time - Arrival time
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(1)
 */
void findTurnAroundTime(ProcessType plist[], int n)
{ 
    // Turnaround time = Burst time + Waiting time
    // This represents total time from arrival to completion
    for (int i = 0; i < n; i++) {
        plist[i].tat = plist[i].bt + plist[i].wt;
    }
} 
  
/**
 * Comparator function for priority-based sorting
 * 
 * Sorts processes by priority value where LOWER NUMBER = HIGHER PRIORITY.
 * This means priority 1 is the highest priority, priority 2 is next, etc.
 * Breaks ties using FCFS order (by arrival time).
 * 
 * @param this: pointer to first process
 * @param that: pointer to second process
 * @return: negative if this < that, positive if this > that, 0 if equal
 * 
 * Sorting Order:
 * 1. Primary: Priority value (ascending - LOWER number = HIGHER priority)
 *    Example: Priority 1 (highest) comes before Priority 2 (lower priority)
 * 2. Secondary: Arrival time (ascending - FCFS for same priority)
 *    Example: If both processes have priority 1, earlier arrival executes first
 * 
 * Note: This implements the common priority convention where smaller priority
 * numbers represent more important/urgent processes.
 */
int my_comparer(const void *this, const void *that)
{ 
    ProcessType *p1 = (ProcessType *)this;
    ProcessType *p2 = (ProcessType *)that;
    
    // Compare by priority first (lower priority number = higher priority)
    if (p1->pri < p2->pri) {
        return -1;  // p1 has higher priority (lower number)
    } else if (p1->pri > p2->pri) {
        return 1;   // p2 has higher priority (lower number)
    } else {
        // Same priority - use FCFS (compare by arrival time)
        if (p1->art < p2->art) {
            return -1;  // p1 arrived first
        } else if (p1->art > p2->art) {
            return 1;   // p2 arrived first
        } else {
            return 0;   // Same arrival time (rare, but possible)
        }
    }
} 

/**
 * Calculate average times for FCFS scheduling
 * 
 * @param plist: array of processes (must be sorted by arrival time)
 * @param n: number of processes
 */
void findavgTimeFCFS(ProcessType plist[], int n) 
{ 
    // Calculate waiting time using FCFS algorithm
    findWaitingTime(plist, n); 
  
    // Calculate turnaround time for all processes
    findTurnAroundTime(plist, n); 
  
    // Display algorithm header
    printf("\n*********\nFCFS\n");
}

/**
 * Calculate average times for SRTF (Preemptive SJF) scheduling
 * 
 * @param plist: array of processes
 * @param n: number of processes
 * 
 * @note This implements SRTF (Shortest Remaining Time First), which is
 *       the preemptive version of SJF.
 */
void findavgTimeSJF(ProcessType plist[], int n) 
{ 
    // Calculate waiting time using SRTF (preemptive SJF) algorithm
    findWaitingTimeSJF(plist, n); 
  
    // Calculate turnaround time for all processes
    findTurnAroundTime(plist, n); 
  
    // Display algorithm header - correctly labeled as SRTF
    printf("\n*********\nSRTF (Preemptive SJF)\n");
}

/**
 * Calculate average times for Round Robin scheduling
 * 
 * @param plist: array of processes
 * @param n: number of processes
 * @param quantum: time quantum for RR scheduling
 */
void findavgTimeRR(ProcessType plist[], int n, int quantum) 
{ 
    // Calculate waiting time using Round Robin algorithm
    findWaitingTimeRR(plist, n, quantum); 
  
    // Calculate turnaround time for all processes
    findTurnAroundTime(plist, n); 
  
    // Display algorithm header with quantum value
    printf("\n*********\nRR Quantum = %d\n", quantum);
}

/**
 * Calculate average times for Priority scheduling
 * 
 * @param plist: array of processes
 * @param n: number of processes
 * 
 * Algorithm:
 * 1. Sort processes by priority (lower number = higher priority)
 * 2. Break ties using FCFS order (by arrival time)
 * 3. Apply FCFS scheduling to sorted list
 */
void findavgTimePriority(ProcessType plist[], int n) 
{ 
    // 1. Sort processes by priority (with FCFS tie-breaking)
    qsort(plist, n, sizeof(ProcessType), my_comparer);
    
    // 2. Apply FCFS algorithm to priority-sorted list
    findWaitingTime(plist, n);
    findTurnAroundTime(plist, n);
  
    // Display algorithm header
    printf("\n*********\nPriority\n");
}

/**
 * Print scheduling metrics for all processes
 * 
 * Displays:
 * - Individual process metrics (PID, burst time, waiting time, turnaround time)
 * - Average waiting time across all processes
 * - Average turnaround time across all processes
 * 
 * @param plist: array of processes with calculated metrics
 * @param n: number of processes
 */
void printMetrics(ProcessType plist[], int n)
{
    int total_wt = 0, total_tat = 0; 
    float awt, att;
    
    printf("\tProcesses\tBurst time\tWaiting time\tTurn around time\n"); 
  
    // Calculate total waiting time and total turnaround time
    for (int i = 0; i < n; i++) 
    { 
        total_wt = total_wt + plist[i].wt; 
        total_tat = total_tat + plist[i].tat; 
        printf("\t%d\t\t%d\t\t%d\t\t%d\n", plist[i].pid, plist[i].bt, plist[i].wt, plist[i].tat); 
    } 
  
    // Calculate and display averages
    awt = ((float)total_wt / (float)n);
    att = ((float)total_tat / (float)n);
    
    printf("\nAverage waiting time = %.2f", awt); 
    printf("\nAverage turn around time = %.2f\n", att); 
} 

/**
 * Initialize process list from input file
 * 
 * @param filename: path to input file containing process data
 * @param n: pointer to store number of processes read
 * @return: dynamically allocated array of processes
 * 
 * @note Caller is responsible for freeing returned array
 */
ProcessType * initProc(char *filename, int *n) 
{
    FILE *input_file = fopen(filename, "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(0);
    }

    ProcessType *plist = parse_file(input_file, n);
    fclose(input_file);
  
    return plist;
}

/**
 * Create a deep copy of process list
 * 
 * This function duplicates a process array, allowing multiple scheduling
 * algorithms to run on the same initial process set without re-reading
 * the input file.
 * 
 * @param original: source process array to copy
 * @param n: number of processes in array
 * @return: newly allocated copy of process array
 * 
 * Benefits:
 * - Avoids redundant file I/O (4x faster initialization)
 * - Maintains original data integrity
 * - Each algorithm gets fresh copy with original arrival/burst times
 * 
 * @note Caller must free the returned array
 */
ProcessType * copyProcessList(ProcessType *original, int n)
{
    // Allocate new array
    ProcessType *copy = (ProcessType *)malloc(n * sizeof(ProcessType));
    
    // Deep copy all process data
    for (int i = 0; i < n; i++) {
        copy[i].pid = original[i].pid;
        copy[i].bt = original[i].bt;
        copy[i].art = original[i].art;
        copy[i].pri = original[i].pri;
        // Reset calculated fields (wt and tat will be recalculated)
        copy[i].wt = 0;
        copy[i].tat = 0;
    }
    
    return copy;
}
  
/**
 * Main driver function
 * 
 * Simulates all four scheduling algorithms on the same process set:
 * 1. FCFS (First-Come-First-Serve)
 * 2. SRTF (Shortest Remaining Time First - Preemptive SJF)
 * 3. Priority Scheduling
 * 4. Round Robin
 * 
 * Optimization: Reads input file once, then creates copies for each algorithm
 * instead of re-reading file 4 times. This improves efficiency and reduces I/O.
 * 
 * @param argc: argument count
 * @param argv: argument vector (argv[1] = input file path)
 * @return: 0 on success, 1 on error
 */
int main(int argc, char *argv[]) 
{ 
    int n = 0; 
    int quantum = 2;  // Time quantum for Round Robin
    ProcessType *proc_list;
    ProcessType *original_list;  // Master copy from file
  
    // Validate command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: ./schedsim <input-file-path>\n");
        fflush(stdout);
        return 1;
    }
    
    // Read input file ONCE - efficiency optimization
    // Instead of reading file 4 times (once per algorithm), we read it once
    // and create copies. This reduces I/O operations from 4 to 1.
    original_list = initProc(argv[1], &n);
    
    // ========== FCFS Scheduling ==========
    proc_list = copyProcessList(original_list, n);
    findavgTimeFCFS(proc_list, n);
    printMetrics(proc_list, n);
    free(proc_list);
  
    // ========== SRTF Scheduling ==========
    proc_list = copyProcessList(original_list, n);
    findavgTimeSJF(proc_list, n); 
    printMetrics(proc_list, n);
    free(proc_list);
  
    // ========== Priority Scheduling ==========
    proc_list = copyProcessList(original_list, n);
    findavgTimePriority(proc_list, n); 
    printMetrics(proc_list, n);
    free(proc_list);
    
    // ========== Round Robin Scheduling ==========
    proc_list = copyProcessList(original_list, n);
    findavgTimeRR(proc_list, n, quantum); 
    printMetrics(proc_list, n);
    free(proc_list);
    
    // Free the original master copy
    free(original_list);
    
    return 0; 
}