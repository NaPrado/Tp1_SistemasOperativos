#ifndef SEMAPHORES_MASTER_H
#define SEMAPHORES_MASTER_H

#include <unistd.h>

#include "structures.h"

int set_init_semaphores(Tgame_sync * game_sync);

int set_master_writing(Tgame_sync * game_sync);

int master_signal_print(Tgame_sync * game_sync);

int unset_master_writing(Tgame_sync * game_sync);

int set_player_reading(Tgame_sync * game_sync);

int unset_player_reading(Tgame_sync * game_sync);

int wait_view(Tgame_sync * game_sync);

int signal_view(Tgame_sync * game_sync);

int destroy_semaphores(Tgame_sync * game_sync);

#ifndef _POSIX_VERSION

int init_systemv_semaphores(void);

#endif

#endif
