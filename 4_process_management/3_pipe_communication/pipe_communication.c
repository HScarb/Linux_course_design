#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#define MAXSIZE 100
#define READ_LEN 50
#define WRITE_LEN 70

void initStr(char * str, int len)
{
    int i;
    char string[MAXSIZE];

    for (i = 0; i < len; i++)
    {
        strcat(string, "*");
    }
    strcpy(str, "\0");
    strcat(str, string);
}

void pipeWrite(int fd[2], int len)
{
    int nWrited;
    char str[MAXSIZE];

    if (len < MAXSIZE)
    {
        initStr(str, WRITE_LEN);
    }
    else
    {
        printf("Pipe write error: len is to long.\n");
        exit(1);
    }

    printf("Write process: Inputting %d characters...\n", len);

    close(fd[0]);
    nWrited = write(fd[1], str, len);
    printf("Write process: Wrote %d characters.\n", len);
}

void pipeRead(int fd[2], int len)
{
    char buffer[MAXSIZE] = "\0";
    int nRead = 0;

    printf("Read process: Reading %d characters...\n", len);
    close(fd[1]);
    nRead = read(fd[0], buffer, len);
    printf("Read process: Read %d characters.\n", nRead);
}

void WriteProcess(int  fd[2])
{

	PipeWrite(fd,WRITE_LEN);

	printf("Write process: Sleep - 10 Seconds ...\n");
	sleep(10);
	printf("Write process: Wake up  then Why not rewrite 70 characters  ? \n");

	PipeWrite(fd,WRITE_LEN);
	printf("Write process : Done\n");
}

void readProcess(int fd[2])
{
    printf("Read process: Time 1.\n");
    pipeRead(fd, READ_LEN);

    printf("Read process: Time 2.\n");
    pipeRead(fd, READ_LEN);

    printf("Read process: Time 3.\n");\
    pipeRead(fd, READ_LEN);
}

int main()
{
    int fd[2];
    pid_t pid;

    if(pipe(fd) < 0)        // check pipe
    {
        fprintf(stderr, "Create pipe error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if((pid = fork()) < 0)      // check fork
    {
        fprintf(stderr, "Fork error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid == 0)           // child interprocess 1 ==? write
    {
        writeProcess(fd);
    }
    else                    // father interprocess
    {
        // create a new child interprocess: read 50
        if((pid = for()) < 0)
        {
            fprintf(stderr, "Fork error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if(pid == 0)        // child interprocess 2 => read
        {
            readProcess(fd);
        }
        else                // father interprocess
        {
            wait(0);
            wait(0);
            printf("End of program.\m");
            return 0;
        }
    }
}