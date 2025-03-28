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
void set_params(int argc, char const *argv[], int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]);

// chequea los parametros, si no son validos los setea a los valores por defecto
// devuelve EXIT_FAILURE si no hay jugadores, EXIT_SUCCESS si todo ok
/*
 * check_params: chequea los parametros
 * num_params: array donde se guardan los parametros
 * view: puntero a la vista
 * players: array de punteros a los jugadores
 * 
*/
int check_params(int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]);
void fill_board(int width, int height, int * board);

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
    ///////////////////////////////////////////////////////////////////////////

    //SHM
    game_status * game = (game_status *) create_shmem("/game_state", sizeof(game_status) + (sizeof(int) * (num_params[1] * num_params[0])), 0644);
    semaphores_status * sync = (semaphores_status *) create_shmem("/game_sync", sizeof(semaphores_status), 0666);

    game->width = num_params[0];
    game->height = num_params[1];
    int delay = num_params[2];
    int timeout = num_params[4];
    srand(num_params[3]);

    game->cant_players = 0;
    while (players[game->cant_players] != NULL) {
        game->cant_players++;
    }

    printf("\033[H\033[J");
    printf("width: %d\n", game->width);
    printf("height: %d\n", game->height);
    printf("delay: %d\n", delay);
    printf("timeout: %d\n", timeout);
    printf("seed: %d\n", num_params[3]);
    printf("view: %s\n", view == NULL ? "-" : view);
    printf("num_players: %d\n", game->cant_players);
    for (int i = 0; i < game->cant_players; i++) {
        printf("  %s\n", players[i]);
    }

    for (int j = 0; j < game->cant_players; j++) {
        strcpy(game->players[j].name_player, players[j]);
        game->players[j].points = 0;
        game->players[j].cant_invalid_movements = 0;
        game->players[j].cant_valid_movements = 0;
        game->players[j].x = rand() % game->width;
        game->players[j].y = rand() % game->height;
        game->players[j].pid = 0;
        game->players[j].can_move = true;
    }

    game->can_end = false;

    sem_init(&sync->show_needed, 1, 0);
    sem_init(&sync->show_done, 1, 0);
    sem_init(&sync->master_mutex, 1, 1);
    sem_init(&sync->game_state_mutex, 1, 1);
    sem_init(&sync->player_read_count_mutex, 1, 1);
    sync->player_reading_status = 0;

    fill_board(game->width, game->height, game->board);
    //////////////////////////////////////////////////////////////////////////////////

    // Abrir pipes
    int fd[game->cant_players][2];
    for (int i = 0; i < game->cant_players; i++) {
        if (pipe(fd[i]) == -1) {
            perror("Error: pipe failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    /////////////////////////////////////////////////////////////////
    
    //execve

    int pid = 0;

    for (int i = 0; i < game->cant_players; i++) {
        if ((pid = fork()) < 0) {
            perror("Error: fork failed.\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // hijo
            printf("Entering player %d\n", i);
            close(fd[i][0]);
            dup2(fd[i][1], STDOUT_FILENO);
            close(fd[i][1]);

            char * new_argv[] = {NULL, NULL, NULL, NULL};
            char arg0[20] = {0};
            snprintf(arg0, 19, "%s", game->players[i].name_player);
            char arg1[10] = {0};
            snprintf(arg1, 9, "%d", game->width);
            char arg2[10] = {0};
            snprintf(arg2, 9, "%d", game->height);
            new_argv[0] = arg0;
            new_argv[1] = arg1;
            new_argv[2] = arg2;

            execve(players[i], new_argv, NULL);
            perror("Error: execve failed");
            exit(EXIT_FAILURE);
        } else { // padre
            printf("Created player %d with pid %d\n", i, pid);

            close(fd[i][1]);

            game->players[i].pid = pid;
            
        }
    }

    fd_set read_fds;
    int max_fd = 0;
    for (int i = 0; i < game->cant_players; i++) {
        if (fd[i][1] > max_fd) {
            max_fd = fd[i][0];
        }
    }

    char buff[20];

    if (read(fd[0][0], buff, 20) == -1) {
        perror("Error: read failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("Read from player %d: %s", 0, buff);

    
    for (int i = 0; i < game->cant_players; i++) {
        wait(NULL);
    }
    
    return 0;
}
















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

void fill_board(int width, int height, int * board) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = randInt(1, 9);
        }
    }
}
