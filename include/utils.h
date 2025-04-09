#ifndef UTILS_H
#define UTILS_H

#include "structures.h"

#define MAX_NUM_PLAYERS 9

#define ERROR -1
#define SUCCESS 0

#define POS_X(game_state, player) ((game_state)->players[(player)].x)
#define POS_Y(game_state, player) ((game_state)->players[(player)].y)
#define BOARD_AT(game_state, x, y) ((game_state)->board[(y) * game_state->width + (x)])
#define BOARD_AT_PLAYER(game_state, player) (BOARD_AT(game_state, POS_X(game_state, player), POS_Y(game_state, player)))
#define GAME_SIZE(game_state, width, height) sizeof(*game_state) + (sizeof(int) * (width * height))


typedef struct {
    size_t width; // ancho del tablero
    size_t height; // alto del tablero
    size_t delay;
    size_t timeout;
    size_t seed;
    char * view;
    char * players[MAX_NUM_PLAYERS];
    size_t amount_players;
} Tparameters;

typedef struct {
    int player;
    int move;
} Tplayer_move;



int valid_possition(game_status * game_state, int x, int y);

int set_params(int argc, char * const argv[], Tparameters * params);

void fill_board(int width, int height, int * board);

void print_inicial_state(Tparameters params);

void set_initial_players_state(game_status * game_state, char * players[MAX_NUM_PLAYERS]);

void set_init_semaphores(semaphores_status * game_sync);

int set_pipes(int fd[MAX_NUM_PLAYERS][2], int num_players);

int set_players_processes(Tparameters* params,game_status* game_state, int fd[][2]);

int set_view_process(const char * view_name, size_t width, size_t height);

int get_max_fd(int fd[][2], size_t amount_players);

int get_player_move(int fd[][2], size_t amount_players, Tplayer_move *move, int* player_number);

int has_next_move(game_status * game_state, int player);

void compute_next_move(game_status * game_state, Tplayer_move move);

void verify_players_cant_move(game_status * game_state);

void exit_error(game_status * game_ptr, semaphores_status * sync_ptr, size_t size_game_state);

#endif
