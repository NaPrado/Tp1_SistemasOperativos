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
        exit(EXIT_FAILURE);
    }

    print_inicial_state(params);

    //SHM
    Tgame_state * game_state = create_game_state(GAME_STATUS_SIZE(game_state, params.width, params.height));
    Tgame_sync * game_sync = create_game_sync();

    set_initial_game_state(game_state, params);

    set_init_semaphores(game_sync);

    // Abrir pipes
    pipe_array pipes;

    pid_t view_pid = set_view_process(params.view, game_state->width, game_state->height);
    if (view_pid == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    if (set_players_processes(&params, game_state, pipes) == ERROR) {
        exit_error(game_state, game_sync, GAME_STATUS_SIZE(game_state, params.width, params.height));
    }

    // loop principal
    while (!game_state->can_end) {

        // print view
        delay(params.delay);
        master_signal_print(game_sync);

        check_players_timeout(game_state);

        // leer movimiento
        Tplayer_move move = {.player = INVALID, .move = INVALID};
        if (get_player_move(pipes, game_state->amount_players, &move) == ERROR) {
            game_state->can_end = true;
            break;
        }

        set_master_writing(game_sync);

        // zona critica (writer)
        // calcular siguiente estado de juego
        bool valid_move = false;
        compute_next_move(game_state, move, &valid_move);
        verify_players_cant_move(game_state);
        
        unset_master_writing(game_sync);

        // si el movimiento fue valido, checkeamos que no se halla pasado el timeout
        if (valid_move) {
            timeout_update(move.player);
        }

    }

    close_pending_pipes(pipes, game_state->amount_players);

    // ultimo post para que la vista termine
    master_signal_print(game_sync);
    
    wait_processes(game_state, view_pid);

    destroy_semaphores(game_sync);

    free_game_sync(game_sync);
	free_game_state(game_state, GAME_STATUS_SIZE(game_state, params.width, params.height));

    return 0;
}
