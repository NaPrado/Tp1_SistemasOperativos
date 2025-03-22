#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>
#include <unistd.h>


int main(int argc, char const *argv[]){

    int height, width;
    height=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    randomize();
    /*bool can_end=false;
    sem_t * game_state_mutex= &(game_sync->game_state_mutex);
    sem_t * master_mutex= &(game_sync->master_mutex);
    sem_t * player_read_count_mutex= &(game_sync->player_read_count_mutex); */
    while (!game_state->can_end){
        /* //entry section
        sem_wait(master_mutex);
        sem_wait(player_read_count_mutex);
        game_sync->player_reading_status++; 
        if (game_sync->player_reading_status == 1) {
            sem_wait(&game_state_mutex);
        }
        sem_post(player_read_count_mutex);
        sem_post(master_mutex);

        //critical zone
        

        //exit Section
        sem_wait(player_read_count_mutex);
        game_sync->player_reading_status--;
        if (game_sync->player_reading_status == 0) {
            sem_post(&game_state_mutex);
        }
        sem_post(player_read_count_mutex);  */
        putchar(randInt(0,7));
        //sem_post(&master_mutex);
    }

    return 0;
}