#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include "../include/structures.h"
//void * create_SHM(const char * name,size_t size);
game_status * get_game_state(size_t size);
semaphores_status * get_game_sync();
#endif