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

/* 
 * setea los parametros de la partida
 * @param argc: cantidad de argumentos
 * @param argv: argumentos
 * @param num_params: array donde se guardan los parametros
 * @param num_params[0]: ancho del tablero
 * @param num_params[1]: alto del tablero
 * @param num_params[2]: delay entre movimientos
 * @param num_params[3]: seed para el random
 * @param num_params[4]: timeout para el juego
 * @param view: puntero a la vista
 * @param players: array de punteros a los jugadores
 * @param view: vista del juego
 * @param players: array de jugadores
 * 
*/
void set_params(int argc, char const *argv[], int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]) {
    int i = 1;
    while (i+1 < argc) {
        if (strcmp(argv[i], FLAG_WIDTH) == 0) {
            num_params[0] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_HEIGHT) == 0) {
            num_params[1] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_DELAY) == 0) {
            num_params[2] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_SEED) == 0) {
            num_params[3] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_VIEW) == 0) {
            *view = (char *) argv[i+1];
            i += 2;
        } else if (strcmp(argv[i], FLAG_TIMEOUT) == 0) {
            num_params[4] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_PLAYER) == 0) {
            int j = 0;
            i++;
            while (i < argc && strcmp(argv[i], FLAG_DELAY) != 0 && strcmp(argv[i], FLAG_HEIGHT) != 0 && strcmp(argv[i], FLAG_WIDTH) != 0 && strcmp(argv[i], FLAG_SEED) != 0 && strcmp(argv[i], FLAG_VIEW) != 0 && strcmp(argv[i], FLAG_TIMEOUT) != 0 && strcmp(argv[i], FLAG_PLAYER) != 0) {
                players[j++] = (char *) argv[i++];
            }
        }
    }
}

/* 
 * Chequea los parametros, si no son validos los setea a los valores por defecto.
 * @param num_params: array donde se guardan los parametros
 * @param num_params[0]: ancho del tablero
 * @param num_params[1]: alto del tablero
 * @param num_params[2]: delay entre movimientos
 * @param num_params[3]: seed para el random
 * @param num_params[4]: timeout para el juego
 * @param view: vista del juego
 * @param players: array de jugadores
 * @return: EXIT_FAILURE si no hay jugadores, EXIT_SUCCESS si todo ok
 * 
*/
int check_params(int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]) {
    if (num_params[0] <= DEF_WIDTH) {
        num_params[0] = DEF_WIDTH;
    }
    if (num_params[1] <= DEF_HEIGHT) {
        num_params[1] = DEF_HEIGHT;
    }
    if (num_params[2] <= 0) {
        num_params[2] = DEF_DELAY;
    }
    if (num_params[3] <= 0) {
        num_params[3] = time(NULL);
    }
    if (num_params[4] <= 0) {
        num_params[4] = DEF_TIMEOUT;
    }
    if (players[0] == NULL) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/* 
 * Llena el tablero con valores aleatorios entre 1 y 9.
 * @param width: ancho del tablero
 * @param height: alto del tablero
 * @param board: puntero al tablero
 * 
*/
void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}

void print_inicial_state(game_status * game_state, int delay, int timeout, int seed, char * view) {
    printf("\033[H\033[J");
    printf("width: %d\n", game_state->width);
    printf("height: %d\n", game_state->height);
    printf("delay: %d\n", delay);
    printf("timeout: %d\n", timeout);
    printf("seed: %d\n", seed);
    printf("view: %s\n", view == NULL ? "-" : view);
    printf("num_players: %d\n", game_state->amount_players);
    for (int i = 0; i < game_state->amount_players; i++) {
        printf("  %s\n", game_state->players[i].name_player);
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

int main(int argc, char const *argv[]) {
    //params
    int num_params[5] = {0};
    char * view = NULL;
    char * players[MAX_NUM_PLAYERS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    set_params(argc, argv, num_params, &view, players);

    if (check_params(num_params, &view, players) == EXIT_FAILURE) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    //SHM
    game_status * game_state = (game_status *) create_shmem("/game_state", sizeof(game_status) + (sizeof(int) * (num_params[1] * num_params[0])), 0644);
    semaphores_status * game_sync = (semaphores_status *) create_shmem("/game_sync", sizeof(semaphores_status), 0666);

    game_state->width = num_params[0];
    game_state->height = num_params[1];
    int delay = num_params[2];
    int timeout = num_params[4];
    srand(num_params[3]);
    game_state->cant_end = false;

    game_state->amount_players = 0;
    while (players[game_state->amount_players] != NULL) {
        game_state->amount_players++;
    }

    set_initial_players_state(game_state, players);

    print_inicial_state(game_state, delay, timeout, num_params[3], view);

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

    fd_set read_fds;
    int max_fd = 0;
    for (int i = 0; i < game_state->amount_players; i++) {
        if (fd[i][0] > max_fd) {
            max_fd = fd[i][0];
        }
    }

    struct timeval tv = {.tv_sec = timeout, .tv_usec = 0};


    // loop principal
    while (!game_state->cant_end) {
        FD_ZERO(&read_fds);
        for (int i = 0; i < game_state->amount_players; i++) {
            FD_SET(fd[i][0], &read_fds);
        }

        int act = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

        if (act < 0) {
            perror("Error: select failed");
            exit(EXIT_FAILURE);
        }
        if (act == 0) {
            printf("Timeout reached. Ending game.\n");
            game_state->cant_end = true;
            break;
        }
        for (int i = 0; i < game_state->amount_players; i++) {
            if (FD_ISSET(fd[i][0], &read_fds)) {
                char buffer[100];
                int bytes_read = read(fd[i][0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Player %d: %s\n", i, buffer);
                } else {
                    perror("Error: read failed");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    

    
    for (int i = 0; i < game_state->amount_players; i++) {
        wait(NULL);
    }
    
    return 0;
}
