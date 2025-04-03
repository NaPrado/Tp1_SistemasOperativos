#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "shm.h"
#include "random.h"

enum params_default {
    DEF_WIDTH = 10, 
    DEF_HEIGHT = 10, 
    DEF_DELAY = 200, 
    DEF_TIMEOUT = 10
};

// flags de los parametros
#define FLAG_WIDTH "-w"
#define FLAG_HEIGHT "-h"
#define FLAG_DELAY "-d"
#define FLAG_SEED "-s"
#define FLAG_VIEW "-v"
#define FLAG_TIMEOUT "-t"
#define FLAG_PLAYER "-p"

#define MAX_NUM_PLAYERS 9

typedef struct {
    size_t width; // ancho del tablero
    size_t height; // alto del tablero
    size_t delay;
    size_t timeout;
    size_t seed;
    char * view;
    char * players[MAX_NUM_PLAYERS];
    size_t amount_players;
} parameters;

extern char *optarg;
extern int optind, opterr, optopt;

void set_params(int argc, char * const argv[], parameters * params) {
    int op;
    if (argc < 2) {
        perror("Error: At least one player must be specified using -p.");
        exit(EXIT_FAILURE);
    }
    while ((op = getopt(argc, argv, "w:h:d:s:v:t:p:")) != -1) {
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
                        exit(EXIT_FAILURE);
                    }
                    params->players[params->amount_players++] = argv[idx++];
                }
                break;
            }
            default:
                exit(EXIT_FAILURE);
        }
    }
    printf("End of options\n");
    if (params->players[0] == NULL) {
        perror("Error: At least one player must be specified using -p.");
        exit(EXIT_FAILURE);
    }
}

void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}

void print_inicial_state(parameters params) {
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
    for (int j = 0; j < game_state->amount_players; j++) {
        strcpy(game_state->players[j].name_player, players[j]);
        game_state->players[j].points = 0;
        game_state->players[j].amount_invalid_movements = 0;
        game_state->players[j].amount_valid_movements = 0;
        game_state->players[j].x = rand() % game_state->width;
        game_state->players[j].y = rand() % game_state->height;
        game_state->players[j].pid = 0;
        game_state->players[j].cant_move = true;
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
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int set_players_processes(game_status * game_state, int fd[][2]) {
    int pid = 0;
    for (int i = 0; i < game_state->amount_players; i++) {
        if ((pid = fork()) < 0) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // hijo
            printf("Entering player %d\n", i);
            if (close(fd[i][0]) == -1) {
                exit(EXIT_FAILURE);
            }
            if (dup2(fd[i][1], STDOUT_FILENO) == -1) {
                exit(EXIT_FAILURE);
            }
            if (close(fd[i][1]) == -1) {
                exit(EXIT_FAILURE);
            }

            char * new_argv[] = {NULL, NULL, NULL, NULL};
            char arg0[20] = {0};
            snprintf(arg0, 19, "%s", game_state->players[i].name_player);
            char arg1[10] = {0};
            snprintf(arg1, 9, "%d", game_state->width);
            char arg2[10] = {0};
            snprintf(arg2, 9, "%d", game_state->height);
            new_argv[0] = arg0;
            new_argv[1] = arg1;
            new_argv[2] = arg2;
            new_argv[3] = NULL;

            execve(game_state->players[i].name_player, new_argv, NULL);
            exit(EXIT_FAILURE);
        } else { // padre
            printf("Created player %d with pid %d\n", i, pid);
            if (close(fd[i][1]) == -1) {
                exit(EXIT_FAILURE);
            }
            game_state->players[i].pid = pid;
        }
    }
    return 0;
}

int get_max_fd(int fd[][2], size_t amount_players) {
    int max_fd = -1;
    for (int i = 0; i < amount_players; i++) {
        if (fd[i][0] > max_fd) {
            max_fd = fd[i][0];
        }
    }
    return max_fd;
}

int get_player_move(int fd[][2], size_t amount_players, int * player_number, int * move) {
    int max_fd = get_max_fd(fd, amount_players);
    fd_set read_fds;
    FD_ZERO(&read_fds);
    for (int i = 0; i < amount_players; i++) {
        if (fd[i][0] != -1) {
            FD_SET(fd[i][0], &read_fds);
        }
    }
    struct timeval tv = {.tv_sec = 10, .tv_usec = 0};
    int act = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    if (act < 0) {
        perror("Error: select failed");
        exit(EXIT_FAILURE);
    }
    if (act == 0) {
        printf("Timeout reached. Ending game.\n");
        return 0;
    }
    for (int i = 0; i < amount_players; i++) {
        if (FD_ISSET(fd[i][0], &read_fds)) {
            char buffer;
            int bytes_read = read(fd[i][0], &buffer, 1);
            if (bytes_read == -1) {
                perror("Error: read failed");
                exit(EXIT_FAILURE);
            }
            if (bytes_read == 0) {
                printf("Player %d has closed the pipe.\n", i);
                close(fd[i][0]);
                fd[i][0] = -1; // Mark as closed
            } else {
                printf("Player %d: %c\n", i, buffer);
            }
        }
    }
    return 1;
}

int main(int argc, char const *argv[]) {
    //params
    parameters params = {
        .width = DEF_WIDTH,
        .height = DEF_HEIGHT,
        .delay = DEF_DELAY,
        .timeout = DEF_TIMEOUT,
        .seed = time(NULL),
        .view = NULL,
        .players = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
        .amount_players = 0
    };

    set_params(argc, (char * const *) argv, &params);

    print_inicial_state(params);

    //SHM
    game_status * game_state = (game_status *) create_shmem("/game_state", sizeof(game_status) + (sizeof(int) * (params.width * params.width)), 0644);
    semaphores_status * game_sync = (semaphores_status *) create_shmem("/game_sync", sizeof(semaphores_status), 0666);

    game_state->width = params.width;
    game_state->height = params.height;
    srand(params.seed);
    game_state->cant_end = false;   
    game_state->amount_players = params.amount_players;

    set_initial_players_state(game_state, params.players);

    fill_board(game_state->width, game_state->height, game_state->board);

    set_init_semaphores(game_sync);

    // Abrir pipes
    int fd[game_state->amount_players][2];
    if (set_pipes(fd, game_state->amount_players) == EXIT_FAILURE) {
        perror("Error: pipe failed");
        exit(EXIT_FAILURE);
    }

    // Crear procesos de jugadores
    if (set_players_processes(game_state, fd) == EXIT_FAILURE) {
        perror("Error: set_players_processes failed");
        exit(EXIT_FAILURE);
    }
    // loop principal
    while (!game_state->cant_end) {

        int player_number = -1, move = -1;
        if (get_player_move(fd, game_state->amount_players, &player_number, &move) == 0) {
            game_state->cant_end = true;
            break;
        } 
        // leer jugada y calcular siguiente estado de juego
        
    }
    

    
    for (int i = 0; i < game_state->amount_players; i++) {
        wait(NULL);
    }
    
    return 0;
}
