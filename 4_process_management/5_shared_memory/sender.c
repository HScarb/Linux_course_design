// sender.c
#include "global.h"

// key
key_t key;

// shared memory 
int shmid;
char *shmptr;
char input[SHM_SIZE];

// semaphore
sem_t * full;
sem_t * mutex;

void init()
{
    key = KEY_NUM;                      // init key
    shmid = getShmId(key);              // init shared memory
    shmptr = shmat(shmid, NULL, 0);     // attach segment to vitural space
    // semaphore init
    full = sem_open(FULL_NAME, O_CREAT);
    mutex = sem_open(MUTEX_NAME, O_CREAT);
}

void saveMessage()
{
    P(mutex);
    strcpy(shmptr, input);
    V(mutex);

    V(full);
}

int main()
{
    init();

    printf("Sender: Please input a message:\n");
    // waiting for user inputting message
    scanf("%s", input);

    saveMessage();

    printf("Sender: Process end.\n");
    return 0;
}
