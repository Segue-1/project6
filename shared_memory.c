#include <sys/stat.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "shared_memory.h"

int shared_mem_getter() {
    int shmemid;

    shmemid = shmget(IPC_PRIVATE, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);

    if (shmemid == -1) {
        perror("shmget");
        exit(1);
    }
    
    return shmemid;
}

void* shared_mem_attacher(int shmemid, unsigned int readonly) {
    void* p;
    int shmflag;

    if (readonly) {
        shmflag = SHM_RDONLY;
    }
    else {
        shmflag = 0;
    }

    p = (void*)shmat(shmemid, 0, shmflag);

    if (!p) {
        perror("shmat");
        exit(1);
    }

    return p;

}

void shared_mem_cleaner(int shmemid, void* p) {
    shared_mem_detach(p);
    shared_mem_deallocator(shmemid);
}

void shared_mem_detach(void* p) {
    if (shmdt(p) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void shared_mem_deallocator(int shmemid) {
    if (shmctl(shmemid, IPC_RMID, 0) == 1) {
        perror("shmctl");
        exit(1);
    }
}