#ifndef GAME_LOGIC_MASTER_H
#define GAME_LOGIC_MASTER_H

#include "structures.h"
#include <stddef.h>

typedef struct {
    int player;
    int move;
} Tplayer_move;

int valid_possition(game_status * game_state, int x, int y);

void fill_board(int width, int height, int * board);

void set_initial_players_state(game_status * game_state, char * players[]);

int get_player_move(int fd[][2], size_t amount_players, Tplayer_move *move, int* player_number);

int has_next_move(game_status * game_state, int player);

void compute_next_move(game_status * game_state, Tplayer_move move);

void verify_players_cant_move(game_status * game_state);

#endif
