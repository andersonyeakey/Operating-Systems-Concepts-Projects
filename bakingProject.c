#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>

volatile pthread_t ramsiedThread;
volatile sig_atomic_t ramsiedRecipe;
volatile sig_atomic_t bakersReady;

void lockSem(int semID) {
        struct sembuf semBuf = {0, -1, 0};
        if (semop(semID, &semBuf, 1) == -1) {
                perror("Semaphore lock failed");
                exit(1);
        }
}

void unlockSem(int semID) {
        struct sembuf semBuf = {0, 1, 0};
        if (semop(semID, &semBuf, 1) == -1) {
                perror("Semaphore unlock failed");
                exit(1);
        }
}

/*char[] getIngredients(int recipe, int location) {
    const char* recipeIngredients[5][2][6] = {
        {//Cookies
            {"Flour", "Sugar", NULL, NULL, NULL, NULL},
            {"Milk", "Butter", NULL, NULL, NULL, NULL}
        },
        {//Pancakes
            {"Flour", "Sugar", "Baking Soda", "Salt", NULL, NULL},
            {"Milk", "Butter", "Egg(s)", NULL, NULL, NULL}
        },
        {//Pizza
            {"Yeast", "Sugar", "Salt"},
            NULL
        },
        {//Pretzels
            {"Flour", "Sugar", "Salt", "Yeast", "Baking Soda", NULL},
            {"Egg", NULL, NULL, NULL, NULL, NULL}
        },
        {//Cinnamon Rolls
            {"Flour", "Sugar", "Salt", "Cinnamon", NULL, NULL},
            {"Eggs", "Butter", NULL, NULL, NULL, NULL}
        }
    };

    return recipeIngredients[recipe][location];
} FIX */

void ramsey() {
        ramsiedThread = pthread_self();
        ramsiedRecipe = rand() % 6;
}

void makeRecipe(int* semIDs, int recipe) {
        /* Takes semID list and recipe name to identify instructions. */
        char recipes[5][16] = {"cookies", "pancakes", "pizza", "pretzels", "cinnamon rolls"};

        lockSem(semIDs[1]);
        /*char[] ingredients = getIngredients(recipe, 0); FIX */
        printf("Chef [%lu] has retrieved alsdjflkasdjf for %s.\n", pthread_self(), recipes[recipe]);
        if(recipe != 2) { lockSem(semIDs[2]); }
        lockSem(semIDs[0]);
        lockSem(semIDs[3]);
        lockSem(semIDs[4]);
        lockSem(semIDs[5]);

        if (pthread_self() == ramsiedThread && recipe == ramsiedRecipe) {
                printf("\033[31mBaker [%lu] got Ramsied while making %s!\n\033[0m", pthread_self(), recipes[recipe]);
                unlockSem(semIDs[0]);
                unlockSem(semIDs[1]);
                if (recipe != 2) { unlockSem(semIDs[2]); }
                unlockSem(semIDs[3]);
                unlockSem(semIDs[4]);
                unlockSem(semIDs[5]);
                lockSem(semIDs[0]);
                lockSem(semIDs[1]);
                if (recipe != 2) { lockSem(semIDs[2]); }
                lockSem(semIDs[3]);
                lockSem(semIDs[4]);
                lockSem(semIDs[5]);
                printf("Chef [%lu] has retrieved lkafnklnfdgnjkdsl for %s.\n", pthread_self(), recipes[recipe]);
        }

        unlockSem(semIDs[0]);
        unlockSem(semIDs[1]);
        if (recipe != 2) { unlockSem(semIDs[2]); }
        unlockSem(semIDs[3]);
        unlockSem(semIDs[4]);
        unlockSem(semIDs[5]);
}

/* Baker Thread Function */
void* baker(void* arg) {
        signal(SIGUSR1, ramsey);
        sleep(0.1);
        __sync_fetch_and_add(&bakersReady, 1);
        sleep(0.1);

        void** data = (void**)arg;
        int* semIDs = malloc(6*sizeof(int));
        for (int i = 0; i < 6; i++) {
                semIDs[i] = ((int*)data[0])[i];
        }
        char** pantryIngredients = ((char***)data[1])[0];
        char** fridgeIngredients = ((char***)data[1])[1];

        srand(time(NULL));
        int firstRecipe = rand() % 5;
        for(int i = firstRecipe; i < 5; i++) {
                makeRecipe(semIDs, i);
        }
        for(int i = 0; i < firstRecipe; i++) {
                makeRecipe(semIDs, i);
        }

        printf("Baker [%lu] is done!!!\n", pthread_self());

        return(0);
}


int main() {
        /* Allocate Shared Baker Resources */
        int pantryShmID = shmget(IPC_PRIVATE, 6*sizeof(char*), IPC_CREAT | 0666);
        int fridgeShmID = shmget(IPC_PRIVATE, 3*sizeof(char*), IPC_CREAT | 0666);
        if (pantryShmID == -1 || fridgeShmID == -1) {
                perror("Unable to allocate shared memory");
                exit(1);
        }
        /* Attach Shared Baker Resources */
        char** pantryIngredients = shmat(pantryShmID, 0, 0);
        char** fridgeIngredients = shmat(fridgeShmID, 0, 0);
        if (pantryIngredients == (void*) -1 || fridgeIngredients == (void*) -1) {
                perror("Unable to attach shared memory");
                exit(1);
        }
        pantryIngredients[0] = "Flour";
        pantryIngredients[1] = "Sugar";
        pantryIngredients[2] = "Yeast";
        pantryIngredients[3] = "Baking Soda";
        pantryIngredients[4] = "Salt";
        pantryIngredients[5] = "Cinnamon";
        fridgeIngredients[0] = "Egg(s)";
        fridgeIngredients[1] = "Milk";
        fridgeIngredients[2] = "Butter";

        /* Initiate Semaphore IDs */
        int mixerSemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        int pantrySemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        int fridgeSemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        int bowlSemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        int spoonSemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        int ovenSemID = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        if (mixerSemID == -1 ||
                pantrySemID == -1 ||
                fridgeSemID == -1 ||
                bowlSemID == -1 ||
                spoonSemID == -1 ||
                ovenSemID == -1)
        {
                perror("Unable to allocate semaphore locks");
                exit(1);
        }

        /* Initiate Semaphore Values */
        if (semctl(mixerSemID, 0, SETVAL, 2) == -1 ||
                semctl(pantrySemID, 0, SETVAL, 1) == -1 ||
                semctl(fridgeSemID, 0, SETVAL, 2) == -1 ||
                semctl(bowlSemID, 0, SETVAL, 3) == -1 ||
                semctl(spoonSemID, 0, SETVAL, 5) == -1 ||
                semctl(ovenSemID, 0, SETVAL, 1) == -1)
        {
                perror("Unable to initiate semaphore locks");
                exit(1);
        }

        /* Request Number of Bakers from Stdin */
        int numBakers;
        printf("How many bakers will there be? -> ");
        scanf("%d", &numBakers);

        /* Initiate Threads in ID Array */
        pthread_t *bakerList = (pthread_t *)malloc(numBakers * sizeof(pthread_t));
        int semList[6] = {mixerSemID, pantrySemID, fridgeSemID, bowlSemID, spoonSemID, ovenSemID};
        char **shmList[2] = {pantryIngredients, fridgeIngredients};
        void *semAndShmList[2] = {semList, shmList};

        /* Select Random Thread ID to Ramsey */
        srand(time(0));
        int ramseyThread = rand() % numBakers;

        /* Initialize Threads */
        for (int i = 0; i < numBakers; i++) {
                pthread_create(&bakerList[i], NULL, baker, &semAndShmList);
        }

        /* Send Ramsey Signal */
        while (bakersReady < numBakers) {}
        if (pthread_kill(bakerList[ramseyThread], SIGUSR1) != 0) {
                perror("Unable to set Ramsey signal");
                exit(1);
        }

        /* Join All Threads */
        int joinReturnValue;
        for (int i = 0; i < numBakers; i++) {
                pthread_join(bakerList[i], (void **) &joinReturnValue);
        }

        /* Deallocate Semaphore Locks */
        if (semctl(mixerSemID, 0, IPC_RMID) == -1 ||
                semctl(pantrySemID, 0, IPC_RMID) == -1 ||
                semctl(fridgeSemID, 0, IPC_RMID) == -1 ||
                semctl(bowlSemID, 0, IPC_RMID) == -1 ||
                semctl(spoonSemID, 0, IPC_RMID) == -1 ||
                semctl(ovenSemID, 0, IPC_RMID) == -1)
        {
                perror("Unable to deallocate semaphore locks");
                exit(1);
        }

        /* Detach Shared Baker Resources */
        if (shmdt(pantryIngredients) == -1 || shmdt(fridgeIngredients) == -1) {
                perror("Unable to detach shared memory");
                exit(1);
        }

        /* Deallocate Shared Baker Resources */
        if (shmctl(pantryShmID, IPC_RMID, 0) == -1 ||
                shmctl(fridgeShmID, IPC_RMID, 0) == -1)
        {
                perror("Unable to deallocated shared memory");
                exit(1);
        }

        return(0);
}
