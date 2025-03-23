#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>


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
    int player;
    pid_t pid=getpid();
    for (size_t i = 0; i < game_state->cant_players; i++){
        if ((game_state->players[i].pid)==pid){
            player=i;  
        }
    }

    int file = open("debug.txt", O_CREAT | O_RDWR | O_TRUNC);

    
    while (!game_state->players[player].can_move){

        // char aux = game_state->players[player].can_move;
        
        char buf[] = {'0'};
        write(file, buf, 1);

        
        putchar(randInt(0,7));


        // entry section

        sem_wait(master_mutex);

        sem_wait(player_read_count_mutex);      // espero modificar variable
        game_sync->player_reading_status++;     // modifico variable

        if (game_sync->player_reading_status == 1) {    // si soy el primero
            sem_wait(game_state_mutex);                // espero a que no haya writer
        }
        sem_post(player_read_count_mutex);      // dejo modificar variable
        sem_post(master_mutex);     // dejo entrar

        //critical zone



        //exit Section
        sem_wait(player_read_count_mutex);      // espero modificar variable
        game_sync->player_reading_status--;     // modifico variable
        if (game_sync->player_reading_status == 0) {    // si soy el ultimo
            sem_post(game_state_mutex);                // dejo que haya writer
        }
        sem_post(player_read_count_mutex);      // dejo modificar variable

        
    }

    close(file);

    return 0;
}