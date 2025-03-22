#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "../include/shm.h"
#include "../include/structures.h"

// Vector de códigos de color
const char *colores[] = {
    "\033[0;30m",  // Negro
    "\033[0;31m",  // Rojo
    "\033[0;32m",  // Verde
    "\033[0;33m",  // Amarillo
    "\033[0;34m",  // Azul
    "\033[0;35m",  // Magenta
    "\033[0;36m",  // Cian
    "\033[0;37m",  // Blanco
    "\033[0;90m"   // Gris claro
};

const char *colores_reset = "\033[0m"; // Restablecer colores

void print_view(int * board, size_t height, size_t width, game_status* game_state) {
    for (size_t i = 0; i < width; i++){
        for (size_t j = 0; j < height; j++){
            for (size_t k = 0; k < game_state->cant_players; k++){
                 if(game_state->players[k].x==i && game_state->players[k].y==j){
                    printf("%s",colores[k]);
                } 
            }
            printf("%d,%s",board[i+j*width],colores_reset);

        }
        printf("\n");
    }
    
}

void clear_screen() {
    const char *clear = "\033[2J\033[H"; // Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda
    write(STDOUT_FILENO, clear, strlen(clear));
}
static void print_player_stats(player_status* player_state){
    printf("name:%s\tpoints:%d\tvalidM:%d\tinvalidM:%d\tcoords:(%d,%d)\n",player_state->name_player,player_state->points,player_state->cant_valid_movements,player_state->cant_invalid_movements,player_state->x,player_state->y);
}

void print_stats(game_status* game_state){
    for (size_t i = 0; i < game_state->cant_players; i++){
        print_player_stats(&(game_state->players)[i]);
    }
    
}


int main(int argc, char const *argv[]) {
    
    int height, width;
    height = atoi(argv[1]);
    width = atoi(argv[2]);
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    sem_t * show_done= &(game_sync->show_done);
    sem_t * show_needed= &(game_sync->show_needed);
    while (!game_state->can_end) {
        sem_wait(show_needed);
        clear_screen();
        print_view(game_state->board, height, width,game_state);
        print_stats(game_state);
        sem_post(show_done);
    }
    return 0;
}
