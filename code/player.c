#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>


int main(int argc, char const *argv[]){

    int heigth, width;
    heigth=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphores_status* game_sync=getOpenSHM("/game_sync",sizeof(semaphores_status));
    //chequear el size
    game_status* game_state=getOpenSHM("/game_state",sizeof(game_status) + sizeof(int)*(heigth*width-1));
    randomize();
    while (!game_state->can_end){
        putchar(randInt(0,8));
    }

    return 0;
}