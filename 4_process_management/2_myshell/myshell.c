#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>

#define MAX_CMD_LEN 20
#define CMD_COLLECTION_LEN 4

// command index
#define INVALID_COMMAND -1
#define EXIT 0
#define CMD_1 1
#define CMD_2 2
#define CMD_3 3

// bool
#define TRUE 1

char * cmdStr [CMD_COLLECTION_LEN] = {"exit", "cmd1", "cmd2", "cmd3"};		// 命令字符串

/**
*	把输入进来的命令字符串转换成命令号	
*/
int getCmdIndex(char *cmd)
{
	int i;
	for (i = 0; i < CMD_COLLECTION_LEN; i++)
	{
		if(strcmp(cmd, cmdStr[i]) == 0)
		{
			return i;		// 返回:命令号
		}
	}
	return INVALID_COMMAND;	// 返回:无效命令
}

void myFork(int cmdIndex)
{
	pid_t pid;
	pid = fork();		// 创建子进程
	if (pid < 0)		// 如果创建出错
	{
		printf("*Fork Error\n");
		exit(1);
	}
	else if (pid == 0)	// 成功创建子进程
	{
		int execl_status = -1;
		printf("-Child is running.\n");

		switch(cmdIndex)
		{
		case CMD_1:
			execl_status = execl("./cmd1", "cmd1", NULL);
			break;
		case CMD_2:
			execl_status = execl("./cmd2", "cmd2", NULL);
			break;
		case CMD_3:
			execl_status = execl("./cmd3", "cmd3", NULL);
			break;
		default:
			printf("*Invalid command.\n");
			break;
		}

		if (execl_status < 0)
		{
			printf("*Fork error.\n");
			exit(1);
		}

		printf("-Child is OK!\n");
		exit(1);
	}
	else
		return;
}

/**
*	根据命令号分配子进程执行命令
*/
void runCMD(int cmdIndex)
{
	switch(cmdIndex)
	{
	case INVALID_COMMAND:
		printf("*Command not found.\n");
		break;
	case EXIT:
		exit(0);
		break;
	default:
		myFork(cmdIndex);
		break;
	}
}

int main()
{
	pid_t pid;
	char cmdStr[MAX_CMD_LEN];

	int cmdIndex;
	while(1)
	{
		printf("\n*Please Input your command:\n");
		scanf("%s", cmdStr);
		cmdIndex = getCmdIndex(cmdStr);
		runCMD(cmdIndex);

		wait(0);
		printf("*Wating for next command...\n");
	}
}
