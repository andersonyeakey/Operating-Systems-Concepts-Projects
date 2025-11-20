#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/shm.h>

// The ring consists of the parent process (parent_pid),
// and the k child processes; all pids are stored in
// the pids[] array. So, the ring goes parent_pid ->
// pids[1] -> pids[2] -> ... -> pids[k - 1], where pid[i]
// is the actual process number and i is the node index
// in the array.

void sigHandler(int);
void exitHandler(int);
int numProcesses;
pid_t childPid;
int processNum; //The index of pids array which contains this child processes ID
int ShmId;
pid_t *pids;
int (*pipes)[][2];

struct buffer {
        char message[100];
        int destination;
};

int main() {
        printf("Give a value for k child processes: ");
        scanf("%d", &numProcesses);

        ShmId = shmget(IPC_PRIVATE, sizeof(pid_t) * numProcesses, IPC_CREAT|0666);
        if(ShmId < 0){
                perror("Unable to create shared memory\n");
                exit(1);
        }

        pids = shmat(ShmId, 0, 0);
        if(pids == (void*) -1){
                perror("Unable to attack shared memory");
                exit(1);
        }

        pids[0] = getpid();
        processNum = 0;
        pipes = malloc(numProcesses * sizeof(int[2]));
        for (int i = 0; i < numProcesses; i++) {
                if (pipe((*pipes)[i]) < 0) {
                        perror("Failed pipe creation.\n");
                        exit(1);
                }
        }

        // Initialize child processes and store their pids
        for (int i = 1; i < numProcesses; i++) {
                if ((childPid = fork()) < 0) {
                        fprintf(stderr, "Fork failure: %s", strerror(errno));
                        exit(1);
                } else if (childPid > 0) {
                        pids[i] = childPid;
                } else {
                        processNum = i;
                        break;
                }
        }

        if (childPid != 0) { // Parent Process
                sigHandler(SIGUSR1); // prompts the first time
                signal(SIGUSR1, sigHandler);
                signal(SIGINT, exitHandler);
                pause();
        } else { // Child Processes
                signal(SIGUSR2, sigHandler);
                signal(SIGINT, exitHandler);
                pause();
        }

        return 0;
}

void sigHandler(int sigNum) {   // parent process should only get a signal at the start and when
                                //the cycle has completed, so this prompts the message for the parent
        char message[100];
        int destination;
        struct buffer buf;

        if (sigNum == 10) { // parent process looks for SIGUSR1
                printf("Give a string to send: ");
                scanf("%100s[^\n]", message);
                printf("Give what process you want to send it to (%d or less): ", numProcesses);
                scanf("%d", &destination);

                printf("Parent process writing %s to child 1!\n", message);
                write((*pipes)[0][1], message, 100*sizeof(char));
                kill(pids[1], SIGUSR2);
        } else if (sigNum == 12) { // non-parents look for SIGUSR2
                if (processNum < numProcesses-1) {
                        printf("Child %d reading... ", processNum);
                        read((*pipes)[processNum-1][0], message, 100*sizeof(char));
                        sleep(1);
                        printf("Child %d writing %s to child %d!\n", processNum, message, processNum+1);
                        write((*pipes)[processNum][1], message, 100*sizeof(char));
                        kill(pids[processNum+1], SIGUSR2);
                } else {
                        printf("Child %d reading... ", processNum);
                        read((*pipes)[processNum-1][0], message, 100*sizeof(char));
                        sleep(1);
                        printf("Child %d writing %s to parent process!\n", processNum, message);
                        write((*pipes)[processNum][1], message, 100*sizeof(char));
                        kill(pids[0], SIGUSR1);
                }
        }
}

void exitHandler(int sigNum) {
        if (childPid != 0) {
                for (int i = 0; i < numProcesses; i++) {
                        close((*pipes)[i][0]);
                        close((*pipes)[i][1]);
                }
                free(pipes);
                kill(pids[1], SIGINT);
                if(shmdt(pids) < 0) {
                        perror("Unable to detach\n");
                        exit(1);
                }
                if(shmctl(ShmId, IPC_RMID, 0) < 0) {
                        perror("Unable to deallocate\n");
                        exit(1);
                }
        } else {
                if (processNum < numProcesses-1) {
                        kill(pids[processNum+1], SIGINT);
                }
                if(shmdt(pids) < 0) {
                        perror("Unable to detach\n");
                        exit(1);
                }
        }
        exit(0);
}
