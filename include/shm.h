#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include "structures.h"
void * create_shmem(const char * name, size_t size, mode_t mode);
game_status * get_game_state(size_t size);
semaphores_status * get_game_sync();
void free_shmem(char* name_shm,void * ptr,size_t size);
#endif