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

int main(int argc, char const *argv[]){

    int height, width;
    height=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    randomize();
    sem_t * game_state_mutex= &(game_sync->game_state_mutex);
    sem_t * master_mutex= &(game_sync->master_mutex);
    sem_t * player_read_count_mutex= &(game_sync->player_read_count_mutex);
    int player_number = 0;
    pid_t pid=getpid();
    for (size_t i = 0; i < game_state->cant_players; i++){
        if ((game_state->players[i].pid)==pid){
            player_number=i;  
        }
    }
    
    sem_init(master_mutex, 1, 1);
    sem_init(game_state_mutex, 1, 1);
    sem_init(player_read_count_mutex, 1, 1);

    // FILE * file = fopen("debug.txt", "w+");

    // if (file == NULL) {
    //     exit(1);
    // }

    // putchar(randInt(0, 7));

    struct timespec time = {.tv_sec = 0, .tv_nsec = 100};
    nanosleep(&time, NULL);
    
    while (!game_state->players[player_number].can_move){

        // seccion de entrada

        

        // sem_wait(master_mutex); // espero en la cola

        sem_wait(player_read_count_mutex); // espero a modificar variable
        game_sync->player_reading_status++;
        if (game_sync->player_reading_status == 1) {
            sem_wait(game_state_mutex); // espero a que writer libere
        }

        // sem_post(master_mutex); // dejo al siguiente en la cola
        sem_post(player_read_count_mutex); // dejo modificar variable

        // seccion critica de lectura
        // putchar(randInt(0,7));
        putchar(player_number);


        // seccion de salida
        sem_wait(player_read_count_mutex); // espero a modificar variable
        game_sync->player_reading_status--;
        if (game_sync->player_reading_status == 0) {
            sem_post(game_state_mutex); // dejo al writer
        }
        sem_post(player_read_count_mutex); // dejo modificar variable

        
    }

    // fclose(file);

    return 0;
}