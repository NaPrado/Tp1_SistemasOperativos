#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>


int main(int argc, char const *argv[]){

    int heigth, width;
    heigth=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphores_status* game_sync=get_open_SHM("/game_sync",sizeof(semaphores_status));
    //chequear el size
    game_status* game_state=get_open_SHM("/game_state",sizeof(game_status) + sizeof(int)*(heigth*width-1));
    randomize();
    while (!game_state->can_end){
        putchar(randInt(0,8));
    }

    return 0;
}