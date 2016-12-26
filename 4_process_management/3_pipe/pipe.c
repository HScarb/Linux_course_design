#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#define MAXSIZE 100
#define READ_LEN 50
#define WRITE_LEN 70

void initStr(char * str, int len)
{
    int i;
    char string[MAXSIZE] = "";

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
        initStr(str, len);
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
    printf("Read: %s\n", buffer);
}

void writeProcess(int  fd[2], int len)
{
	pipeWrite(fd,len);

	printf("Write process: Done\n");
}

void readProcess(int fd[2], int len)
{
	printf("Read process: Time 1\n");
	pipeRead(fd, len);
}

int main()
{
    int fd[2];
    int i;
    pid_t pid;

    if(pipe(fd) < 0)        	// create pipe
    {
        fprintf(stderr, "Create pipe error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

        if((pid = fork()) < 0)
        {
            fprintf(stderr, "Fork error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            writeProcess(fd, 10);
        }
        else
        {
            pid = fork();
	    if (pid == 0)
		writeProcess(fd, 20);
	    else
	    {
		pid = fork();
		if (pid == 0)
			writeProcess(fd, 30);
		else
		{
			wait(NULL);
			wait(NULL);
			wait(NULL);
			pipeRead(fd, 40);
			pipeRead(fd, 30);
		}
	    }
        }
}
