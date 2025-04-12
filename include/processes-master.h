#ifndef PROCESSES_MASTER_H
#define PROCESSES_MASTER_H

#include "structures.h"

int set_pipes(pipe_array pipes, int num_players);

int set_players_processes(Tparameters * params, Tgame_state * game_state, pipe_array pipes);

int set_view_process(const char * view_name, size_t width, size_t height);


#endif
