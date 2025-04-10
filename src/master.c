#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "return-codes.h"
#include "structures.h"
#include "game-logic-master.h"
#include "parameters-master.h"
#include "processes-master.h"
#include "sync-lib.h"
#include "shm.h"

int main(int argc, char const *argv[]) {

    printf("\033[H\033[J\n");

    Tparameters params = get_default_params();

    if (set_params(argc, (char * const *) argv, &params) == ERROR) {
        return EXIT_FAILURE;
    }

    print_inicial_state(params);

    //SHM
    game_status * game_state = create_game_state(GAME_STATUS_SIZE(game_state, params.width, params.height));
    semaphores_status * game_sync = create_game_sync();

    game_state->width = params.width;
    game_state->height = params.height;
    srand(params.seed);
    game_state->can_end = false;   
    game_state->amount_players = params.amount_players;

    fill_board(game_state->width, game_state->height, game_state->board);

    set_initial_players_state(game_state, params.players);

    set_init_semaphores(game_sync);

    // Abrir pipes
    int fd[game_state->amount_players][2];
    if (set_pipes(fd, game_state->amount_players) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    // Crear procesos de jugadores
    if (set_players_processes(&params, game_state, fd) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    if (set_view_process(params.view, game_state->width, game_state->height) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }
    
    int player=0;
    // loop principal
    while (!game_state->can_end) {

        // print view
        usleep(params.delay* 1000);
        sem_post(&game_sync->show_needed);
        sem_wait(&game_sync->show_done);

        // leer movimiento
        Tplayer_move move = {.player = -1, .move = -1};
        while (move.move == -1 || move.player == -1) {
            if (get_player_move(fd, game_state->amount_players, &move, &player) == ERROR) {
                game_state->can_end = true;
                break;
            }
        }
        // if (!game_state->can_end) {
        //     break;
        // }

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
    // ultimo post para que la vista termine
    sem_post(&game_sync->show_needed);
    
    for (int i = 0; i < game_state->amount_players + 1; i++) { // +1 para el view
        wait(NULL);
    }

	free_game_state(game_state, GAME_STATUS_SIZE(game_state, params.width, params.height));
    free_game_sync(game_sync);

    return 0;
}
