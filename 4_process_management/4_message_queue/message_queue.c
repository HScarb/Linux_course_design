#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define TRUE 1

#define BUFFER_SIZE 255
#define RERM S_IRUSR|S_IWUSR
#define KEY_NUM 1000

typedef struct msgbuf msgbuf;

struct msgbuf
{
    long mtype;
    char mtext[BUFFER_SIZE+ 1 ];
}

// semaphore and mutex
sem_t full;
sem_t empty;
sem_t mutex;

// pthread
pthread_t write_pid;
pthread_t read_pid;

// key
key_t key;

// message
int msgid;

struct msgbuf msg;

void init()
{
    // init semaphore
    sem_init(&full, 0, 0);
    
}