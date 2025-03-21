#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "../include/shm.h"
#include "../include/structures.h"


void printView(int * board, size_t height, size_t width) {
    for (size_t i = 0; i < width; i++){
        for (size_t j = 0; j < height; j++){
            printf("%d",board[i+j*height]);
        }
        printf("\n");
    }
    
}

void clear_screen() {
    const char *clear = "\033[2J\033[H"; // Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda
    write(STDOUT_FILENO, clear, strlen(clear));
}


int main(int argc, char const *argv[]) {
    
    int heigth, width;
    heigth = atoi(argv[1]);
    width = atoi(argv[2]);
    semaphoresStatus * semStatus = getOpenSHM("/game_sync", sizeof(semaphoresStatus));
    //chequear el size
    gameStatus * gStatus = getOpenSHM("/game_state", sizeof(gameStatus) + (sizeof(int) * (heigth * width)));
    while (!gStatus->can_end) {
        sem_t show_needed=semStatus->show_needed;
        sem_wait(&(show_needed)); 
        clear_screen();
        printView(gStatus->board, heigth, width);
        sem_t show_done=semStatus->show_done;
        sem_wait(&(show_done));
    }
    
    

    return 0;
}
