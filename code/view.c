#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "../include/shm.h"
#include "../include/structures.h"


void printView(int * board,size_t height,size_t width){
    for (size_t i = 0; i < width; i++){
        for (size_t j = 0; j < height; j++){
            printf("%d",board[i*j]);
        }
        printf("\n");
    }
    
}

void clear_screen() {
    const char *clear = "\033[2J\033[H"; // Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda
    write(STDOUT_FILENO, clear, strlen(clear));
}


int main(int argc, char const *argv[]){
    
    int heigth, width;
    heigth=atoi(argv[1]);
    width=atoi(argv[2]);
    semaphoresStatus* semStatus=createSHM("/game_sync",sizeof(semaphoresStatus));
    //chequear el size
    gameStatus* gStatus=createSHM("/game_state",sizeof(gameStatus)+(sizeof(int)*(heigth*width)));
    printf("heigth %d,%d",heigth,gStatus->heigth);
    printf("width %d,%d",width,gStatus->width);
    while (1){
        sem_wait(&(semStatus->show_needed));
        printView(gStatus->board,heigth,width);
        sem_post(&(semStatus->show_needed));
    }
    
    

    return 0;
}
