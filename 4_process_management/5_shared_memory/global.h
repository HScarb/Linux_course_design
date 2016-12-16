#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>

#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

static const char * MUTEX_NAME = "mutex_shm";
static const char * FULL_NAME = "full_shm";

// constant define
#define SHM_SIZE 1024
#define KEY_NUM 1000

int getShmId(key_t key);

/*
*   create mutex and semaphore
*   init those value
*/
void semInit();

void semDestroy();

void P(sem_t *sem);

void V(sem_t *sem);

#endif