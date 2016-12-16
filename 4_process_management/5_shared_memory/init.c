// init.c
#include "common.h"

int main(int argc, const char * argv[])
{
    key_t key;
    int semid;
    int shmid;

    // create key
    key = KEY_NUM;

    semInit();

    getShmId(key);

    printf("End of initialize.\n");
}