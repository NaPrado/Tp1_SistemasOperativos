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
    semaphores_status * game_sync = getOpenSHM("/game_sync", sizeof(semaphores_status));
    //chequear el size
    game_status * game_state = getOpenSHM("/game_state", sizeof(game_status) + (sizeof(int) * (heigth * width)));
    while (!game_state->can_end) {
        sem_t show_needed=game_sync->show_needed;
        sem_wait(&(show_needed)); 
        clear_screen();
        printView(game_state->board, heigth, width);
        sem_t show_done=game_sync->show_done;
        sem_wait(&(show_done));
    }
    
    

    return 0;
}
