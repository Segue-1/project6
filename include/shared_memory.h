#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>
#include "clock.h"

int shared_mem_getter();
void* shared_mem_attacher(int shmemid, unsigned int readonly);
void shared_mem_cleaner(int shmemid, void* p);
void shared_mem_detach(void* p);
void shared_mem_deallocator(int shmemid);

#endif
