#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include "structures.h"
// Crean SHM y lo mapea (para master)
void * create_shmem(const char * name, size_t size, mode_t mode);

Tgame_state * create_game_state(size_t size);

Tgame_sync * create_game_sync();

// Abren SHM ya abiertos (para player y view)
Tgame_state * get_game_state(size_t size);

Tgame_sync * get_game_sync();

// Unmap y cierra
void munmap_game_state(Tgame_state * ptr, size_t size);

void munmap_game_sync(Tgame_sync * ptr);
/* Only Unmaps and closes */
void free_game_sync(Tgame_sync * ptr);
void free_game_state(Tgame_state * ptr,size_t size);

//
void exit_error(Tgame_state * game_ptr, Tgame_sync * sync_ptr, size_t size_game_state);
#endif