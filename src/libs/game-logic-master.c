#include "game-logic-master.h"
#include "structures.h"
#include "return-codes.h"
#include "random.h"

#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <time.h>
#include <math.h>


typedef struct {
    int x;
    int y;
} T2D_move;

T2D_move posible_moves[8] = { 
    {0, -1},
    {1, -1}, 
    {1, 0}, 
    {1, 1}, 
    {0, 1}, 
    {-1, 1}, 
    {-1, 0}, 
    {-1, -1}, 
};

#define POS_X(game_state, player) ((game_state)->players[(player)].x)
#define POS_Y(game_state, player) ((game_state)->players[(player)].y)
#define BOARD_AT(game_state, x, y) ((game_state)->board[(y) * game_state->width + (x)])
#define BOARD_AT_PLAYER(game_state, player) (BOARD_AT(game_state, POS_X(game_state, player), POS_Y(game_state, player)))

int valid_possition(Tgame_state * game_state, int x, int y) {
    return (x >= 0 && x < game_state->width && y >= 0 && y < game_state->height && 
            BOARD_AT(game_state, x, y) > 0);
}

static void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}

static void set_inicial_players_position(Tgame_state * game_state) {

    double h = (game_state->height - 1) / 2.0;
    double k = (game_state->width - 1) / 2.0;

    double a = h * 0.8;
    double b = k * 0.8;

    for (int i = 0; i < game_state->amount_players; i++) {

        double theta = 2.0 * M_PI * i / game_state->amount_players;

        double xf = k + b * sin(theta);
        double yf = h + a * cos(theta);

        game_state->players[i].x = (int) round(xf);
        game_state->players[i].y = (int) round(yf);
    }
}

static void set_initial_players_state(Tgame_state * game_state, char * players[]) {
    for (int i = 0; i < game_state->amount_players; i++) {
        char name[150];
        strcpy(name, players[i]);
        strcpy(game_state->players[i].name_player, basename(name));
        game_state->players[i].points = 0;
        game_state->players[i].amount_invalid_movements = 0;
        game_state->players[i].amount_valid_movements = 0;
        game_state->players[i].pid = 0;
        game_state->players[i].cant_move = 0;
        BOARD_AT_PLAYER(game_state, i) = i * (-1);
    }
}

static size_t timeout = 0;

static time_t timeouts[MAX_NUM_PLAYERS] = {0};

static void set_initial_timeouts(size_t timeout_value) {
    timeout = timeout_value;
    time_t current_time = time(NULL);
    for (size_t i = 0; i < MAX_NUM_PLAYERS; i++) {
        timeouts[i] = current_time;
    }
}

void set_initial_game_state(Tgame_state * game_state, Tparameters params) {
    game_state->width = params.width;
    game_state->height = params.height;
    game_state->can_end = false;
    game_state->amount_players = params.amount_players;
    srand(params.seed);
    fill_board(game_state->width, game_state->height, game_state->board);
    set_inicial_players_position(game_state);
    set_initial_players_state(game_state, params.players);
    set_initial_timeouts(params.timeout);
}

int has_next_move(Tgame_state * game_state, int player) {
    int x = POS_X(game_state, player);
    int y = POS_Y(game_state, player);
    for (int i = y - 1; i <= y + 1; i++) {
        for (int j = x - 1; j <= x + 1; j++) {
            if (!(x == j && y == i) && valid_possition(game_state, j, i)) {
                if (BOARD_AT(game_state, j, i) > 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void compute_next_move(Tgame_state * game_state, Tplayer_move move, bool * valid_move) {
    T2D_move next_move = posible_moves[move.move % 8];
    if (!valid_possition(game_state, 
        POS_X(game_state, move.player) + next_move.x, 
        POS_Y(game_state, move.player) + next_move.y)) {
        game_state->players[move.player].amount_invalid_movements++;
        *valid_move = false;
    } else {
        game_state->players[move.player].amount_valid_movements++;
        game_state->players[move.player].x += next_move.x;
        game_state->players[move.player].y += next_move.y;
        game_state->players[move.player].points += BOARD_AT_PLAYER(game_state, move.player);
        BOARD_AT_PLAYER(game_state, move.player) = move.player * (-1);
        *valid_move = true;
    }
}

void verify_players_cant_move(Tgame_state * game_state) {
    for (int i = 0; i < game_state->amount_players; i++) {
        game_state->players[i].cant_move = !has_next_move(game_state, i);
    }
}

static int get_max_fd(pipe_array pipes, size_t amount_players) {
    int max_fd = ERROR;
    for (int i = 0; i < amount_players; i++) {
        if (pipes[i][0] > max_fd) {
            max_fd = pipes[i][0];
        }
    }
    return max_fd;
}

static int check_pipes_open(pipe_array pipes, size_t amount_players) {
    for (int i = 0; i < amount_players; i++) {
        if (pipes[i][0] != CLOSED_PIPE) {
            return true;
        }
    }
    return false;
}

int get_player_move(pipe_array pipes, size_t amount_players, Tplayer_move *move) {
    static int player_number = 0;
    int max_fd = get_max_fd(pipes, amount_players);
    fd_set read_fds;
    FD_ZERO(&read_fds);

    for (int i = 0; i < amount_players; i++) {
        if (pipes[i][0] != CLOSED_PIPE) {
            FD_SET(pipes[i][0], &read_fds);
        }
    }

    if (!check_pipes_open(pipes, amount_players)) {
        printf("No pipes open. Ending game.\n");
        return ERROR;
    }

    struct timeval tv = {.tv_sec = 10, .tv_usec = 0};

    int act = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

    if (act < 0) {
        perror("Error: select failed");
        return ERROR;
    }
    if (act == 0) {
        printf("Timeout reached. Ending game.\n");
        return ERROR;
    }

    int i = 0;
    while (i++ < amount_players) {
        if (pipes[player_number][0] != CLOSED_PIPE && FD_ISSET(pipes[player_number][0], &read_fds)) {
            char buffer;
            int bytes_read = read(pipes[player_number][0], &buffer, 1);
            if (bytes_read == ERROR) {
                perror("Error: read failed");
                return ERROR;
            }
            if (bytes_read > 0) {
                move->player = player_number;
                move->move = buffer;
                player_number++;
                player_number %= amount_players;
                return SUCCESS;
            }
        }
        player_number++;
        player_number %= amount_players;
    }
    return ERROR;
}

void timeout_update(size_t player_number) {
    timeouts[player_number] = time(NULL);
}

void check_players_timeout(Tgame_state * game_state/* , int pipe[2] */) {
    time_t current_time = time(NULL);
    for (size_t i = 0; i < game_state->amount_players; i++) {
        if (current_time - timeouts[i] > timeout) {
            game_state->players[i].cant_move = true;
        }
    }
}

void delay(size_t delay) {
    usleep(delay * 1000);
}
