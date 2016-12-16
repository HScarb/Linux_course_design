#include "global.h"

int getShmId(key_t key)
{
    int shmid;
    shmid = shmget(key, SHM_SIZE, IPC_CREAT|0666);
    if(shmid < 0)
    {
        perror("Receiver: Shmget error.\n");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

/*
* create mutex + semaphore
* init those value
*/
void semInit()
{
    if((sem_open(MUTEX_NAME, O_CREAT, 0644, 1)) < 0)
    {
        perror("sem_open error.\n");
        exit(EXIT_FAILURE);
    }
    if((sem_open(FULL_NAME, O_CREAT, 0644, 0)) < 0)
    {
        perror("sem_open error.\n");
        exit(EXIT_FAILURE);
    }
    printf("Sem init finish.\n");
}

/*
*   close and unlink semaphore
*/
void semDestroy()
{
    sem_t * mutexPtr = sem_open(MUTEX_NAME, O_CREAT);
    sem_t * fullPtr = sem_open(FULL_NAME, O_CREAT);

    // destroy mutex
    sem_close(mutexPtr);                // int sem_close(sem_t * sem)
    sem_unlink(MUTEX_NAME);             // int sem_unlink(const char * name)

    // destroy full
    sem_close(fullPtr);
    sem_unlink(FULL_NAME);
}

void P(sem_t * semPtr)
{
    sem_wait(semPtr);
}

void V(sem_t * semPtr)
{
    sem_post(semPtr);
}