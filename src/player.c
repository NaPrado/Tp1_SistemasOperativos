#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
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
static int directions[3][3]={
    {7,0,1},
    {6,8,2},
    {5,4,3}
};

int get_next_move(game_status * game_state, int player_number){
    int x=game_state->players[player_number].x;
    int y=game_state->players[player_number].y;
    int last_points=-10;
    int last_i=0;
    int last_j=0;
    int points;
    for (int i = y-1 ; i < y+2 ; i++){
        for (int j = x-1 ; j < x+2 ; j++){
            if ((x!=j || y!=i) && j<game_state->width && i<game_state->height && j>=0 && i>=0){
                points=game_state->board[j+i*game_state->width];
                if ((points>=0||points==-10) && last_points<points){
                    last_points=points;
                    last_i=i;
                    last_j=j;
                }
            }
        }
    }
    return directions[last_i-y+1][last_j-x+1];
}


int main(int argc, char const *argv[]){

    int height, width;
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    
    sem_t * game_state_mutex= &(game_sync->game_state_mutex);
    sem_t * master_mutex= &(game_sync->master_mutex);
    sem_t * player_read_count_mutex= &(game_sync->player_read_count_mutex);
    int player_number = 0;
    pid_t pid=getpid();
    for (size_t i = 0; i < game_state->amount_players; i++){
        if ((game_state->players[i].pid)==pid){
            player_number=i;  
        }
    }
    srand(time(NULL) + player_number);
    /* 
    while(1){
        recibir_movimiento(...);
        wait(writer);
        wait(mutex);
        post(writer);
        ejecutar_movimiento(...);
        post(mutex);
    }
    while(1){
        wait(writer);
        post(writer);

        wait(readers_count_mutex);
        if(readers++=0){
            wait(mutex);
        }
        post(readers_count_mutex);

        GET_BOARD(...);

        wait(readers_count_mutex);

    }
    */
    int next_dir;
    while (!game_state->players[player_number].cant_move) {
        // seccion de entrada
        sem_wait(master_mutex);
        sem_post(master_mutex);

        sem_wait(player_read_count_mutex); // espero a modificar variable
        if (game_sync->player_reading_status++ == 0) sem_wait(game_state_mutex); // espero a que writer libere
        sem_post(player_read_count_mutex); // dejo modificar variable

        // seccion critica de lectura
        next_dir=get_next_move(game_state,player_number);

        // seccion de salida
        sem_wait(player_read_count_mutex); // espero a modificar variable
        if (game_sync->player_reading_status-- == 1) sem_post(game_state_mutex); // dejo al writer
        sem_post(player_read_count_mutex); // dejo modificar variable
        putchar(next_dir);
        //usleep(500);
    }

    return 0;
}