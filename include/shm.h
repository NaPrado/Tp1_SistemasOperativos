#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include "structures.h"
// Crean SHM y lo mapea (para master)
void * create_shmem(const char * name, size_t size, mode_t mode);

game_status * create_game_state(size_t size);

semaphores_status * create_game_sync();

// Abren SHM ya abiertos (para player y view)
game_status * get_game_state(size_t size);

semaphores_status * get_game_sync();

// Unmap y cierra
void munmap_game_state(game_status * ptr, size_t size);

void munmap_game_sync(semaphores_status * ptr);
/* Only Unmaps and closes */
void free_game_sync(semaphores_status * ptr);
void free_game_state(game_status * ptr,size_t size);

//
void exit_error(game_status * game_ptr, semaphores_status * sync_ptr, size_t size_game_state);
#endif