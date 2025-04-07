#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include "structures.h"
/* Creates a SHM */
void * create_shmem(const char * name, size_t size, mode_t mode);
game_status * create_game_state(size_t size);
semaphores_status * create_game_sync();
/* Gets a SHM that's already open */
game_status * get_game_state(size_t size);
semaphores_status * get_game_sync();
/* Only Unmaps */
void munmap_game_state(game_status * ptr, size_t size);
void munmap_game_sync(semaphores_status * ptr);
/* Only Unmaps and closes */
void free_shmem(char* name_shm,void * ptr,size_t size);
void free_game_sync(semaphores_status * ptr);
void free_game_state(game_status * ptr,size_t size);
void free_game_state_and_sync(semaphores_status * sync_ptr,game_status * ptr,size_t size_game_state);
#endif