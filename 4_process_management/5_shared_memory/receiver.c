// receiver.c
#include "global.h"

// key
key_t key;

// shared memory
int shmid;
char *shmptr;
char result[SHM_SIZE];

// semaphore
sem_t * full;
sem_t * mutex;

void init()
{
    key = KEY_NUM;
    shmid = getShmId(key);              // init shared memory
    shmptr = shmat(shmid, NULL, 0);     // attach segment to virtual space
    // semaphore init
    full = sem_open(FULL_NAME, O_CREAT);
    mutex = sem_open(MUTEX_NAME, O_CREAT);
}

void readMessage()
{
    P(full);
    P(mutex);
    strcpy(result, shmptr);
    V(mutex);
}

int main(int argc, char const * argv[])
{
    init();
    // waiting for user inputting message
    readMessage();

    printf("Receiver: Message is %s\n", result);
    semDestroy();
    printf("Receiver: Process end.\n");
    return 0;
}