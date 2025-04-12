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

    Tparameters params = get_default_params();

    if (set_params(argc, (char * const *) argv, &params) == ERROR) {
        return EXIT_FAILURE;
    }

    print_inicial_state(params);

    //SHM
    Tgame_state * game_state = create_game_state(GAME_STATUS_SIZE(game_state, params.width, params.height));
    Tgame_sync * game_sync = create_game_sync();

    set_initial_game_state(game_state, params);

    set_init_semaphores(game_sync);

    // Abrir pipes
    pipe_array pipes;
    if (set_pipes(pipes, game_state->amount_players) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    // Crear procesos de jugadores
    if (set_players_processes(&params, game_state, pipes) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    if (set_view_process(params.view, game_state->width, game_state->height) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    int player=0;
    // loop principal
    while (!game_state->can_end) {

        // print view
        usleep(params.delay * 1000);
        sem_post(&game_sync->show_needed);
        sem_wait(&game_sync->show_done);

        check_players_timeout(game_state);

        // leer movimiento
        Tplayer_move move = {.player = -1, .move = -1};
        if (get_player_move(pipes, game_state->amount_players, &move, &player) == ERROR) {
            game_state->can_end = true;
            break;
        }

        sem_wait(&game_sync->master_mutex);
        sem_wait(&game_sync->game_state_mutex);
        sem_post(&game_sync->master_mutex);


        // zona critica (writer)
        // calcular siguiente estado de juego
        bool valid_move = false;
        compute_next_move(game_state, move, &valid_move);
        verify_players_cant_move(game_state);
        
        sem_post(&game_sync->game_state_mutex);

        // si el movimiento fue valido, checkeamos que no se halla pasado el timeout
        if (valid_move) {
            timeout_update(move.player);
        }

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
