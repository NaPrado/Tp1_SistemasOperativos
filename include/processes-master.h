#ifndef PROCESSES_MASTER_H
#define PROCESSES_MASTER_H

#include "structures.h"

int set_pipes(int fd[][2], int num_players);

int set_players_processes(Tparameters * params, game_status * game_state, int fd[][2]);

int set_view_process(const char * view_name, size_t width, size_t height);


#endif
