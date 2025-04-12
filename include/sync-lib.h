#ifndef SEMAPHORES_MASTER_H
#define SEMAPHORES_MASTER_H

#include "structures.h"

void set_init_semaphores(Tgame_sync * game_sync);

void set_master_writing(Tgame_sync * game_sync);

void master_signal_print(Tgame_sync * game_sync);

void unset_master_writing(Tgame_sync * game_sync);

void set_player_reading(Tgame_sync * game_sync);

void unset_player_reading(Tgame_sync * game_sync);

void wait_view(Tgame_sync * game_sync);

void signal_view(Tgame_sync * game_sync);

void destroy_semaphores(Tgame_sync * game_sync);

#endif
