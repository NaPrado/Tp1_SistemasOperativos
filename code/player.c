#include "../include/shm.h"
#include "../include/structures.h"
#include <stdio.h>



int main(int argc, char const *argv[]){

    int heigth, width;
    heigth=atoi(argv[1]);
    width=atoi(argv[2]);
    
    semaphoresStatus* semStatus=createSHM("/game_sync",sizeof(semaphoresStatus),O_RDWR | O_CREAT,PROT_WRITE | PROT_READ);
    //chequear el size
    gameStatus* gStatus=createSHM("/game_state",sizeof(gameStatus),O_RDONLY,PROT_WRITE);

    

    return 0;
}