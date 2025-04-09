#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "utils.h"
#include "shm.h"
#include "structures.h"
#include "random.h"

#define OPT_STRING "w:h:d:s:v:t:p:"

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

extern char *optarg;
extern int optind, opterr, optopt;

int valid_possition(game_status * game_state, int x, int y) {
    return (x >= 0 && x < game_state->width && y >= 0 && y < game_state->height && 
            BOARD_AT(game_state, x, y) > 0);
}

int set_params(int argc, char * const argv[], Tparameters * params) {
    int op;
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return ERROR;
    }
    while ((op = getopt(argc, argv, OPT_STRING)) != ERROR) {
        switch (op) {
            case 'w':
                if (atoi(optarg) >= 10) {
                    params->width = atoi(optarg);
                }
                break;
            case 'h':
                if (atoi(optarg) >= 10) {
                    params->height= atoi(optarg);
                }
                break;
            case 'd':
                if (atoi(optarg) > 0) {
                    params->delay = atoi(optarg);
                }
                break;
            case 's':
                if (atoi(optarg) > 0) {
                    params->seed = atoi(optarg);
                }
                break;
            case 'v':
                params->view = optarg;
                break;
            case 't':
                if (atoi(optarg) > 0) {
                    params->timeout = atoi(optarg);
                }
                break;
            case 'p': {
                int idx = optind - 1;
                params->amount_players = 0;
                while (argv[idx] != NULL && argv[idx][0] != '-') {
                    if (params->amount_players >= MAX_NUM_PLAYERS) {
                        fprintf(stderr, "Error: At most 9 players can be specified using -p");
                        return ERROR;
                    }
                    params->players[params->amount_players++] = argv[idx++];
                }
                break;
            }
            default:
                return ERROR;
        }
    }
    if (params->players[0] == NULL) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return ERROR;
    }
    return SUCCESS;
}

void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}

void print_inicial_state(Tparameters params) {
    printf("\033[H\033[J");
    printf("width: %zu\n", params.width);
    printf("height: %zu\n", params.height);
    printf("delay: %zu\n", params.delay);
    printf("timeout: %zu\n", params.timeout);
    printf("seed: %zu\n", params.seed);
    printf("view: %s\n", params.view == NULL ? "-" : params.view);
    printf("num_players: %zu\n", params.amount_players);
    for (int i = 0; i < params.amount_players; i++) {
        printf("  %s\n", params.players[i]);
    }
}

void set_initial_players_state(game_status * game_state, char * players[MAX_NUM_PLAYERS]) {
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

void set_init_semaphores(semaphores_status * game_sync) {
    sem_init(&game_sync->show_needed, 1, 0);
    sem_init(&game_sync->show_done, 1, 0);
    sem_init(&game_sync->master_mutex, 1, 1);
    sem_init(&game_sync->game_state_mutex, 1, 1);
    sem_init(&game_sync->player_read_count_mutex, 1, 1);
    game_sync->player_reading_status = 0;
}

int set_pipes(int fd[MAX_NUM_PLAYERS][2], int num_players) {
    for (int i = 0; i < num_players; i++) {
        if (pipe(fd[i]) == -1) {
            return ERROR;
        }
    }
    return SUCCESS;
}

int set_players_processes(Tparameters* params,game_status* game_state, int fd[][2]) {
    int pid = 0;
    for (int i = 0; i < params->amount_players; i++) {
        if ((pid = fork()) < 0) {
            perror("Error: player fork failed");
            return ERROR;
        } else if (pid == 0) { // hijo
            if (close(fd[i][0]) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }
            if (dup2(fd[i][1], STDOUT_FILENO) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }
            if (close(fd[i][1]) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }

            char * new_argv[] = {NULL, NULL, NULL, NULL};
            char arg0[20] = {0};
            snprintf(arg0, 19, "%s", params->players[i]);
            char arg1[10] = {0};
            snprintf(arg1, 9, "%ld", params->width);
            char arg2[10] = {0};
            snprintf(arg2, 9, "%ld", params->height);
            new_argv[0] = arg0;
            new_argv[1] = arg1;
            new_argv[2] = arg2;
            new_argv[3] = NULL;

            execve(params->players[i], new_argv, NULL);
            return ERROR;
        } else { // padre
            if (close(fd[i][1]) == -1) {
                return ERROR;
            }
            game_state->players[i].pid = pid;
        }
    }
    return SUCCESS;
}

int set_view_process(const char * view_name, size_t width, size_t height) {
    if (view_name == NULL) {
        return ERROR;
    }
    int pid = fork();
    if (pid < 0) {
        perror("Error: view fork failed");
        return ERROR;
    }
    if (pid == 0) { // hijo
        char * new_argv[] = {NULL, NULL, NULL, NULL};
        char arg0[20] = {0};
        snprintf(arg0, 19, "%s", view_name);
        char arg1[10] = {0};
        snprintf(arg1, 9, "%zu", width);
        char arg2[10] = {0};
        snprintf(arg2, 9, "%zu", height);
        new_argv[0] = arg0;
        new_argv[1] = arg1;
        new_argv[2] = arg2;
        new_argv[3] = NULL;

        execve(view_name, new_argv, NULL);
        return ERROR;
    }
    return SUCCESS;
}

int get_max_fd(int fd[][2], size_t amount_players) {
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


void exit_error(game_status * game_ptr, semaphores_status * sync_ptr, size_t size_game_state) {
    free_game_sync(sync_ptr);
	free_game_state(game_ptr, size_game_state);
    exit(EXIT_FAILURE);
}
