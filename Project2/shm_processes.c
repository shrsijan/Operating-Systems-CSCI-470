#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

// Shared memory structure
typedef struct {
    int BankAccount;
    sem_t mutex;  // Semaphore for mutual exclusion
} SharedData;

void PoorStudent(SharedData *shared, int studentNum);
void DearOldDad(SharedData *shared);
void LovableMom(SharedData *shared);

int main(int argc, char *argv[])
{
    int ShmID;
    SharedData *ShmPTR;
    pid_t pid;
    int numParents, numStudents;
    int i;

    // Disable output buffering
    setbuf(stdout, NULL);

    // Parse command line arguments
    if (argc != 3) {
        printf("Use: %s <num_parents 1 or 2> <num_students>\n", argv[0]);
        printf("  1 parent  = Dear Old Dad only\n");
        printf("  2 parents = Dear Old Dad + Lovable Mom\n");
        exit(1);
    }

    numParents = atoi(argv[1]);
    numStudents = atoi(argv[2]);

    if (numParents < 1 || numParents > 2) {
        printf("Number of parents must be 1 or 2\n");
        exit(1);
    }

    if (numStudents < 1) {
        printf("Number of students must be at least 1\n");
        exit(1);
    }

    // Create shared memory
    ShmID = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }

    // Attach shared memory
    ShmPTR = (SharedData *) shmat(ShmID, NULL, 0);
    if (ShmPTR == (void *) -1) {
        printf("*** shmat error ***\n");
        exit(1);
    }

    // Initialize shared data
    ShmPTR->BankAccount = 0;
    
    // Initialize semaphore (shared between processes, initial value = 1)
    if (sem_init(&ShmPTR->mutex, 1, 1) < 0) {
        printf("*** sem_init error ***\n");
        exit(1);
    }

    printf("Bank Account initialized to $%d\n", ShmPTR->BankAccount);
    printf("Starting with %d parent(s) and %d student(s)\n", numParents, numStudents);

    // Seed random number generator
    srand(time(NULL));

    // Fork child processes (Poor Students)
    for (i = 0; i < numStudents; i++) {
        pid = fork();
        if (pid < 0) {
            printf("*** fork error (student %d) ***\n", i + 1);
            exit(1);
        }
        else if (pid == 0) {
            // Child process - Poor Student
            srand(time(NULL) ^ getpid());  // Re-seed for child
            PoorStudent(ShmPTR, i + 1);
            exit(0);
        }
    }

    // Fork Lovable Mom if numParents == 2
    if (numParents == 2) {
        pid = fork();
        if (pid < 0) {
            printf("*** fork error (mom) ***\n");
            exit(1);
        }
        else if (pid == 0) {
            // Child process - Lovable Mom
            srand(time(NULL) ^ getpid());
            LovableMom(ShmPTR);
            exit(0);
        }
    }

    // Parent process - Dear Old Dad (original process)
    DearOldDad(ShmPTR);

    // Cleanup (this won't normally be reached due to infinite loops)
    // Wait for all children
    while (wait(NULL) > 0);
    
    sem_destroy(&ShmPTR->mutex);
    shmdt((void *) ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);
    
    return 0;
}

void DearOldDad(SharedData *shared)
{
    int localBalance;
    int randomNum;
    int amount;

    while (1) {
        // Sleep random amount between 0-5 seconds
        sleep(rand() % 6);

        printf("Dear Old Dad: Attempting to Check Balance\n");

        // Generate random number
        randomNum = rand();

        if (randomNum % 2 == 0) {
            // Even: Check if should deposit
            sem_wait(&shared->mutex);  // DOWN - enter critical section
            
            localBalance = shared->BankAccount;

            if (localBalance < 100) {
                // Try to deposit money
                amount = rand() % 101;  // Random amount 0-100

                if (amount % 2 == 0) {
                    // Even: Actually deposit
                    localBalance += amount;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                    shared->BankAccount = localBalance;
                } else {
                    // Odd: No money to give
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }

            sem_post(&shared->mutex);  // UP - leave critical section
        } else {
            // Odd: Just check balance
            sem_wait(&shared->mutex);
            localBalance = shared->BankAccount;
            sem_post(&shared->mutex);
            
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
    }
}

void LovableMom(SharedData *shared)
{
    int localBalance;
    int amount;

    while (1) {
        // Sleep random amount between 0-10 seconds
        sleep(rand() % 11);

        printf("Loveable Mom: Attempting to Check Balance\n");

        sem_wait(&shared->mutex);  // DOWN - enter critical section

        localBalance = shared->BankAccount;

        if (localBalance <= 100) {
            // Always deposit when balance <= 100
            amount = rand() % 126;  // Random amount 0-125

            localBalance += amount;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
            shared->BankAccount = localBalance;
        }

        sem_post(&shared->mutex);  // UP - leave critical section
    }
}

void PoorStudent(SharedData *shared, int studentNum)
{
    int localBalance;
    int randomNum;
    int need;

    while (1) {
        // Sleep random amount between 0-5 seconds
        sleep(rand() % 6);

        printf("Poor Student %d: Attempting to Check Balance\n", studentNum);

        // Generate random number
        randomNum = rand();

        if (randomNum % 2 == 0) {
            // Even: Attempt to withdraw
            sem_wait(&shared->mutex);  // DOWN - enter critical section

            localBalance = shared->BankAccount;

            // Generate need between 0-50
            need = rand() % 51;
            printf("Poor Student %d needs $%d\n", studentNum, need);

            if (need <= localBalance) {
                // Can withdraw
                localBalance -= need;
                printf("Poor Student %d: Withdraws $%d / Balance = $%d\n", studentNum, need, localBalance);
            } else {
                // Not enough cash
                printf("Poor Student %d: Not Enough Cash ($%d)\n", studentNum, localBalance);
            }

            shared->BankAccount = localBalance;

            sem_post(&shared->mutex);  // UP - leave critical section
        } else {
            // Odd: Just check balance
            sem_wait(&shared->mutex);
            localBalance = shared->BankAccount;
            sem_post(&shared->mutex);
            
            printf("Poor Student %d: Last Checking Balance = $%d\n", studentNum, localBalance);
        }
    }
}