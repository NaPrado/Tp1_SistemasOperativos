#ifndef GAME_LOGIC_MASTER_H
#define GAME_LOGIC_MASTER_H

#include "structures.h"
#include <stddef.h>

typedef struct {
    int player;
    int move;
} Tplayer_move;

#define INVALID -1

void set_initial_game_state(Tgame_state * game_state, Tparameters params);

int valid_possition(Tgame_state * game_state, int x, int y);

int get_player_move(pipe_array pipes, size_t amount_players, Tplayer_move * move);

int has_next_move(Tgame_state * game_state, int player);

void compute_next_move(Tgame_state * game_state, Tplayer_move move, bool * valid_move);

void verify_players_cant_move(Tgame_state * game_state);

void timeout_update(size_t player_number);

void check_players_timeout(Tgame_state * game_state);

void delay(size_t delay);

#endif
