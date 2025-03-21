#include "../include/shm.h"
#include "../include/structures.h"
#include "../include/random.h"
#include <stdio.h>


int main(int argc, char const *argv[]){

    int heigth, width;
    heigth=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphoresStatus* semStatus=getOpenSHM("/game_sync",sizeof(semaphoresStatus));
    //chequear el size
    gameStatus* gStatus=getOpenSHM("/game_state",sizeof(gameStatus) + sizeof(int)*(heigth*width-1));
    randomize();
    while (!gStatus->can_end){
        putchar(randInt(0,8));
    }

    return 0;
}