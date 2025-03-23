#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>

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

int main(int argc, char const *argv[]){

    int height, width;
    height=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    randomize();
    bool can_end=false;
    sem_t * game_state_mutex= &(game_sync->game_state_mutex);
    sem_t * master_mutex= &(game_sync->master_mutex);
    sem_t * player_read_count_mutex= &(game_sync->player_read_count_mutex);
    int player_number;
    pid_t pid=getpid();
    for (size_t i = 0; i < game_state->cant_players; i++){
        if ((game_state->players[i].pid)==pid){
            player_number=i;  
        }
    }

    FILE * file = fopen("debug.txt", "w+");

    if (file == NULL) {
        exit(1);
    }

    int cant_move = 0;
    
    while (!cant_move){
        
        putchar(randInt(0,7));

         // entry section

        fprintf("%d ", cant_move);

        sem_wait(master_mutex);
        sem_wait(player_read_count_mutex);      // espero modificar variable
        game_sync->player_reading_status++;     // modifico variable

        if (game_sync->player_reading_status == 1) {    // si soy el primero
            sem_wait(game_state_mutex);                // espero a que no haya writer
        }
        sem_post(player_read_count_mutex);      // dejo modificar variable
        sem_post(master_mutex);     // dejo entrar

        //critical zone

        cant_move = !can_player_move(width, height, game_state->board, game_state->players[player_number].x, game_state->players[player_number].y);

        if (cant_move) {
            break;
        }

        //exit Section
        sem_wait(player_read_count_mutex);      // espero modificar variable
        game_sync->player_reading_status--;     // modifico variable
        if (game_sync->player_reading_status == 0) {    // si soy el ultimo
            sem_post(game_state_mutex);                // dejo que haya writer
        }
        sem_post(player_read_count_mutex);      // dejo modificar variable 

        
    }

    fclose(file);

    return 0;
}