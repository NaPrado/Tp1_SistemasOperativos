#ifndef PROCESSES_MASTER_H
#define PROCESSES_MASTER_H

#include "structures.h"

#define WAIT_CHILDS(num) for (int i = 0; i < (num); i++) wait(NULL)

int set_pipes(pipe_array pipes, int num_players);

int set_players_processes(Tparameters * params, Tgame_state * game_state, pipe_array pipes);

int set_view_process(const char * view_name, size_t width, size_t height);

void close_pending_pipes(pipe_array pipes, size_t amount_players);

#endif
