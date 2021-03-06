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
#define PERM S_IRUSR|S_IWUSR
#define KEY_NUM 1000

typedef struct msgbuf msgbuf;

struct msgbuf
{
    long mtype;
    char mtext[BUFFER_SIZE+ 1 ];
};
int isEnd = 0;
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
    sem_init(&end, 0, 0);      // 工作
    sem_init(&mutex, 0, 1);     // 空闲

    key = KEY_NUM;

    // create message queue: key
    if((msgid = msgget(key, PERM|IPC_CREAT)) == -1)
    {
        fprintf(stderr, "Create Message Queue Error %s \n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void * readProcess(void *arg)
{
    msgbuf msg;
    // init msg
    msg.mtype = 1;
    while(1)
    {
        sem_wait(&mutex);

        // receive message from message queue
        msgrcv(msgid, &msg, sizeof(msgbuf), 1, 0);

        // detect for end
        if(strcmp(msg.mtext, "end") == 0)
        {
	    printf("R: [message received] end\n");
            msg.mtype = 2;
            strncpy(msg.mtext, "over", BUFFER_SIZE);
            msgsnd(msgid, &msg, sizeof(msgbuf), 0);
	    printf("R: [message sent] over\n");
            sem_post(&empty);
	    sem_post(&mutex);
	    break;
        }

        // print message
        printf("R: [message received] %s\n\n", msg.mtext);

        sem_post(&mutex);
    }
    printf("R: Exit.\n");
    exit(EXIT_SUCCESS);
}

void * writeProcess(void * arg)
{
    char input[50];
    msgbuf msg;
    msg.mtype = 1;

    while(!isEnd)
    {
        // semaphore
        sem_wait(&mutex);

        printf("W: Please input the message you want to send:\n");
        scanf("%s", input);
        
        if(strcmp(input, "exit") == 0)
        {
            strncpy(msg.mtext, "end", BUFFER_SIZE);
            msgsnd(msgid, &msg, sizeof(msgbuf), 0);
	    printf("W: [message sent] end\n");
	    isEnd = 1;
	    sem_post(&full);
	    sem_post(&mutex);
            break;
        }
        strncpy(msg.mtext, input, BUFFER_SIZE);
        msgsnd(msgid, &msg, sizeof(msgbuf), 0);

        printf("W: [message sent] %s\n", msg.mtext);

        sem_post(&mutex);
    }
    printf("W: Enter.");
    while(isEnd){
    sem_wait(&mutex);
    printf("W: Wait to receive \"over\"\n");
    // clear node
    memset(&msg, '\0', sizeof(msgbuf));
    // block, waiting for message with type 2
    msgrcv(msgid, &msg, sizeof(msgbuf), 2, 0);
    printf("W: [message received2] %s\n", msg.mtext);
    sem_post(&empty);
    sem_post(&mutex);
    break;
}
    // remove message queue
    if (msgctl(msgid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "Remove message queue error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

int main()
{
    init();

    pthread_create(&write_pid, NULL, writeProcess, NULL);
    pthread_create(&read_pid, NULL, readProcess, NULL);
    // waiting for the thread end
    pthread_join(write_pid, NULL);
    pthread_join(read_pid, NULL);

    printf("Main end...");
}
