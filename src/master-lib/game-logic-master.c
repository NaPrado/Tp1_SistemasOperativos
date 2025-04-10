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

int valid_possition(game_status * game_state, int x, int y) {
    return (x >= 0 && x < game_state->width && y >= 0 && y < game_state->height && 
            BOARD_AT(game_state, x, y) > 0);
}

void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}

void set_initial_players_state(game_status * game_state, char * players[]) {
    for (int i = 0; i < game_state->amount_players; i++) {
        char name[150];
        strcpy(name, players[i]);
        strcpy(game_state->players[i].name_player, basename(name));
        game_state->players[i].points = 0;
        game_state->players[i].amount_invalid_movements = 0;
        game_state->players[i].amount_valid_movements = 0;
        game_state->players[i].x = rand() % game_state->width;
        game_state->players[i].y = rand() % game_state->height;
        game_state->players[i].pid = 0;
        game_state->players[i].cant_move = 0;
        BOARD_AT_PLAYER(game_state, i) = i * (-1);
    }
}

int has_next_move(game_status * game_state, int player) {
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

void compute_next_move(game_status * game_state, Tplayer_move move) {
    T2D_move next_move = posible_moves[move.move % 8];
    if (!valid_possition(game_state, 
        POS_X(game_state, move.player) + next_move.x, 
        POS_Y(game_state, move.player) + next_move.y)) {
        game_state->players[move.player].amount_invalid_movements++;
    } else {
        game_state->players[move.player].amount_valid_movements++;
        game_state->players[move.player].x += next_move.x;
        game_state->players[move.player].y += next_move.y;
        game_state->players[move.player].points += BOARD_AT_PLAYER(game_state, move.player);
        BOARD_AT_PLAYER(game_state, move.player) = move.player * (-1);
    }
}

void verify_players_cant_move(game_status * game_state) {
    for (int i = 0; i < game_state->amount_players; i++) {
        game_state->players[i].cant_move = !has_next_move(game_state, i);
    }
}

static int get_max_fd(int fd[][2], size_t amount_players) {
    int max_fd = ERROR;
    for (int i = 0; i < amount_players; i++) {
        if (fd[i][0] > max_fd) {
            max_fd = fd[i][0];
        }
    }
    return max_fd;
}

int get_player_move(int fd[][2], size_t amount_players, Tplayer_move *move, int* player_number) {
    int max_fd = get_max_fd(fd, amount_players);
    fd_set read_fds;
    FD_ZERO(&read_fds);

    for (int i = 0; i < amount_players; i++) {
        if (fd[i][0] != -1) {
            FD_SET(fd[i][0], &read_fds);
        }
    }

    // if (!check_pipes_open(fd, amount_players)) {
    //     printf("No pipes open. Ending game.\n");
    //     return ERROR;
    // }

    struct timeval tv = {.tv_sec = 3, .tv_usec = 0};

    int act = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

    if (act < 0) {
        perror("Error: select failed");
        return ERROR;
    }
    if (act == 0) {
        printf("Timeout reached. Ending game.\n");
        return ERROR;
    }

    for (; *player_number < amount_players; (*player_number)++,(*player_number)%=amount_players) {
        if (fd[(*player_number)][0] != -1 && FD_ISSET(fd[(*player_number)][0], &read_fds)) {
            char buffer;
            int bytes_read = read(fd[(*player_number)][0], &buffer, 1);
            if (bytes_read == -1) {
                perror("Error: read failed");
                return ERROR;
            }
            if (bytes_read == 0) {
                // Pipe cerrado
                printf("Player %d disconnected.\n", (*player_number));
                move->player = -1;
                move->move = -1;
                close(fd[(*player_number)][0]);
                fd[(*player_number)][0] = -1;
                return SUCCESS; // que un jugador se haya desconectado no es un error
            } else {
                printf("Player %d read %d\n", (*player_number), buffer);
                move->player = (*player_number);
                move->move = buffer;
                (*player_number)++;
                (*player_number)%=amount_players;
                return SUCCESS;
            }
        }
    }
    return ERROR;
}
