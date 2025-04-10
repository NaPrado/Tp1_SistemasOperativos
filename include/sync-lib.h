#ifndef SEMAPHORES_MASTER_H
#define SEMAPHORES_MASTER_H

#include "structures.h"

void set_init_semaphores(semaphores_status * game_sync);

void set_master_writing(semaphores_status * game_sync);

void unset_master_writing(semaphores_status * game_sync);

void set_player_reading(semaphores_status * game_sync);

void unset_player_reading(semaphores_status * game_sync);

void destroy_semaphores(semaphores_status * game_sync);

#endif
