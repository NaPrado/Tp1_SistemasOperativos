#include "shm.h"
#include "structures.h"
#include "return-codes.h"
#include "sync-lib.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int can_player_move(int width, int height, int board[][width], int x, int y){
    int ret = 0;

    for (int i = -1; i <= 1 && ret == 0; i++) {
        for (int j = -1; j <= 1 && ret == 0; j++) {

            if (!(i == 0 && j == 0) && (x + i >= 0 && y + j >= 0) && (x + i < width && y + j < height)) {
                ret += board[y + j][x + i] > 0;
            }

        }
    }
    
    return ret;
    
}
static int directions[3][3] = {
    {7, 0, 1},
    {6, 8, 2},
    {5, 4, 3}
};

int get_next_move(Tgame_state * game_state, int player_number){
    int x = game_state->players[player_number].x;
    int y = game_state->players[player_number].y;
    int last_points = -10;
    int last_i = 0;
    int last_j = 0;
    int points;
    for (int i = y - 1; i < y + 2; i++){
        for (int j = x - 1; j < x + 2; j++){
            if ((x != j || y != i) && j < game_state->width && i < game_state->height && j >= 0 && i >= 0) {
                points = game_state->board[j + i * game_state->width];
                if ((points >= 0 || points == -10) && last_points < points){
                    last_points = points;
                    last_i = i;
                    last_j = j;
                }
            }
        }
    }
    return directions[last_i - y + 1][last_j - x + 1];
}


int main(int argc, char const *argv[]){

    int height, width;
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    
    Tgame_sync * game_sync = get_game_sync();
    //chequear el size
    Tgame_state * game_state = get_game_state(GAME_STATUS_SIZE(game_state, width, height));


#ifndef _POSIX_VERSION
    if (init_systemv_semaphores() == ERROR) {
        perror("Error: set_player_semaphore failed");
        return EXIT_FAILURE;
    }
#endif
    
    int player_number = 0;
    pid_t pid = getpid();
    for (size_t i = 0; i < game_state->amount_players; i++){
        if ((game_state->players[i].pid) == pid){
            player_number = i;  
        }
    }
    srand(time(NULL) + player_number);
    
    int next_dir;
    int last_amount_moves = -1;
    bool do_move = false;
    while (!game_state->players[player_number].cant_move) {
        // seccion de entrada
        set_player_reading(game_sync);

        // seccion critica de lectura

        int amount_moves = (game_state->players[player_number].amount_valid_movements) + (game_state->players[player_number].amount_invalid_movements);
        if (amount_moves != last_amount_moves){    
            last_amount_moves = amount_moves;
            next_dir = get_next_move(game_state,player_number);
            do_move = true;
        }

        // seccion de salida
        unset_player_reading(game_sync);
        
        if (do_move){
            do_move=false;
            write(STDOUT_FILENO, &next_dir, 1);
        }
    }

    munmap_game_state(game_state, GAME_STATUS_SIZE(game_state, width, height));
    munmap_game_sync(game_sync);

    return SUCCESS;
}