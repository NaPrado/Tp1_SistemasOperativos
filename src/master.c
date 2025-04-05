#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <time.h>
#include <libgen.h>

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

#define POS_X(game_state, player) ((game_state)->players[(player)].x)
#define POS_Y(game_state, player) ((game_state)->players[(player)].y)
#define BOARD_AT(game_state, x, y) ((game_state)->board[(y) * game_state->width + (x)])
#define BOARD_AT_PLAYER(game_state, player) (BOARD_AT(game_state, POS_X(game_state, player), POS_Y(game_state, player)))

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

void set_params(int argc, char * const argv[], Tparameters * params) {
    int op;
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
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
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
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
        strcpy(game_state->players[i].name_player,basename(name));
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
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int set_players_processes(Tparameters* params,game_status* game_state, int fd[][2]) {
    int pid = 0;
    for (int i = 0; i < params->amount_players; i++) {
        if ((pid = fork()) < 0) {
            perror("Error: player fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // hijo
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
            exit(EXIT_FAILURE);
        } else { // padre
            if (close(fd[i][1]) == -1) {
                exit(EXIT_FAILURE);
            }
            game_state->players[i].pid = pid;
        }
    }
    return 0;
}

int set_view_process(const char * view_name, size_t width, size_t height) {
    if (view_name == NULL) {
        return -1;
    }
    int pid = fork();
    if (pid < 0) {
        perror("Error: view fork failed");
        return -1;
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
    return -1;
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

typedef struct {
    int queue[2 * MAX_NUM_PLAYERS];
    int start;
    int end;
} RoundRobinQueue;

void round_robin_init(RoundRobinQueue *round_robin_queue) {
    round_robin_queue->start = 0;
    round_robin_queue->end = 0;
    for (int i = 0; i < 2 * MAX_NUM_PLAYERS; i++) {
        round_robin_queue->queue[i] = -1;
    }
}

void round_robin_enqueue(RoundRobinQueue *round_robin_queue, int player) {
    round_robin_queue->queue[round_robin_queue->end] = player;
    round_robin_queue->end = (round_robin_queue->end + 1) % (2 * MAX_NUM_PLAYERS);
}

int round_robin_dequeue(RoundRobinQueue *round_robin_queue) {
    if (round_robin_queue->queue[round_robin_queue->start] == -1 || round_robin_queue->start == round_robin_queue->end) {
        return -1; // Cola vacía
    }
    int player = round_robin_queue->queue[round_robin_queue->start];
    round_robin_queue->queue[round_robin_queue->start] = -1;
    round_robin_queue->start = (round_robin_queue->start + 1) % (2 * MAX_NUM_PLAYERS);
    return player;
}


int get_player_move(int fd[][2], size_t amount_players, Tplayer_move *move, RoundRobinQueue *round_robin_queue) {
    int max_fd = get_max_fd(fd, amount_players);
    fd_set read_fds;
    FD_ZERO(&read_fds);

    for (int i = 0; i < amount_players; i++) {
        if (fd[i][0] != -1) {
            FD_SET(fd[i][0], &read_fds);
        }
    }

    struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
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
        if (fd[i][0] != -1 && FD_ISSET(fd[i][0], &read_fds)) {
            round_robin_enqueue(round_robin_queue, i);

            char buffer;
            int bytes_read = read(fd[i][0], &buffer, 1);
            if (bytes_read == -1) {
                perror("Error: read failed");
                exit(EXIT_FAILURE);
            }
            if (bytes_read == 0) {
                // Pipe cerrado
                printf("Player %d disconnected.\n", i);
                move->player = -1;
                move->move = -1;
                close(fd[i][0]);
                fd[i][0] = -1;
            } else {
                printf("Player %d read %c\n", i, buffer+'0');
                move->player = i;
                move->move = buffer;
                return 1;
            }
        }
    }

    // Si no hubo lectura válida, intentar avanzar en la cola
    int next_player = round_robin_dequeue(round_robin_queue);
    if (next_player == -1) {
        return 0;
    }

    move->player = next_player;
    move->move = -1; // O lo que tenga sentido en este contexto
    return 1;
}

int has_next_move(game_status * game_state, int player) {
    int x = POS_X(game_state, player);
    int y = POS_Y(game_state, player);
    for (int i = y - 1; i <= y + 1; i++) {
        for (int j = x - 1; j <= x + 1; j++) {
            if (!(x == j && y == i) && valid_possition(game_state, j, i)) {
                if (BOARD_AT(game_state, j, i) > 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
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

int main(int argc, char const *argv[]) {

    printf("\033[H\033[J\n");

    //params
    Tparameters params = {
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

    fill_board(game_state->width, game_state->height, game_state->board);

    set_initial_players_state(game_state, params.players);

    set_init_semaphores(game_sync);

    // Abrir pipes
    int fd[game_state->amount_players][2];
    if (set_pipes(fd, game_state->amount_players) == EXIT_FAILURE) {
        perror("Error: pipe failed");
        exit(EXIT_FAILURE);
    }

    // Crear procesos de jugadores
    if (set_players_processes(&params, game_state, fd) == EXIT_FAILURE) {
        perror("Error: set_players_processes failed");
        exit(EXIT_FAILURE);
    }

    if (set_view_process(params.view, game_state->width, game_state->height) == EXIT_FAILURE) {
        perror("Error: set_view_process failed");
        exit(EXIT_FAILURE);
    }
    struct timespec ts = {0, params.delay*1000000};
    
    // loop principal
    while (!game_state->cant_end) {

        // print view

        // getchar();
        sem_post(&game_sync->show_needed);
        sem_wait(&game_sync->show_done);
        nanosleep(&ts, NULL);

        // leer movimiento
        Tplayer_move move = {.player = -1, .move = -1};
        RoundRobinQueue round_robin_queue;
        round_robin_init(&round_robin_queue);
        while (move.move == -1 || move.player == -1) {
            if (get_player_move(fd, game_state->amount_players, &move,&round_robin_queue) == 0) {
                game_state->cant_end = true;
                break;
            }
        }

        sem_wait(&game_sync->master_mutex);
        sem_wait(&game_sync->game_state_mutex);
        sem_post(&game_sync->master_mutex);


        // zona critica (writer)
        // calcular siguiente estado de juego
        compute_next_move(game_state, move);
        verify_players_cant_move(game_state);
        

        sem_post(&game_sync->game_state_mutex);
        // printf("Player %d moved to (%d)\n", move.player, move.move);
    }
    
    
    
    for (int i = 0; i < game_state->amount_players; i++) {
        wait(NULL);
    }
    free_shmem("/game_state",game_state,sizeof(game_status) + (sizeof(int) * (params.width * params.width)));
    free_shmem("/game_sync",game_sync,sizeof(semaphores_status));
    return 0;
}
