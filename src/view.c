#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "../include/shm.h"
#include "../include/structures.h"

// Vector de códigos de color
const char *colores[] = {
    "\033[40;97m",  // Fondo negro, letra blanca
    "\033[41;97m",  // Fondo rojo, letra blanca
    "\033[42;30m",  // Fondo verde, letra negra
    "\033[43;30m",  // Fondo amarillo, letra negra
    "\033[44;97m",  // Fondo azul, letra blanca
    "\033[45;97m",  // Fondo magenta, letra blanca
    "\033[46;30m",  // Fondo cian, letra negra
    "\033[47;30m",  // Fondo blanco, letra negra
    "\033[100;97m"  // Fondo gris claro, letra blanca
};
const char *colorescabeza[] = {
    "\033[48;5;235;97m",  // Cabeza con fondo gris más oscuro y letra blanca
    "\033[48;5;196;97m",  // Cabeza con fondo rojo y letra blanca
    "\033[48;5;28;97m",   // Cabeza con fondo verde oscuro y letra blanca
    "\033[48;5;130;97m",  // Cabeza con fondo marrón oscuro y letra blanca
    "\033[48;5;32;97m",   // Cabeza con fondo azul y letra blanca
    "\033[48;5;125;97m",  // Cabeza con fondo magenta oscuro y letra blanca
    "\033[48;5;51;97m",   // Cabeza con fondo cian oscuro y letra blanca
    "\033[48;5;15;97m",   // Cabeza con fondo blanco y letra negra
    "\033[48;5;237;97m"   // Cabeza con fondo gris oscuro y letra blanca
};


const char *colores_reset = "\033[0;0m"; // Restablecer colores

void print_view(int * board, size_t height, size_t width, game_status * game_state) {
    int flag = 0;
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            for (size_t k = 0; k < game_state->cant_players; k++) {
                if(board[j + i * width] == -k){
                    if (game_state->players[k].x == j && game_state->players[k].y == i) {
                        printf("%s|   |", colorescabeza[k]);
                        flag = 1;
                    } else {
                        printf("%s|   |", colores[k]);
                        flag = 1;
                    }
                } 
            }
            if (board[j + i * width] < 0 && !flag) {
                printf("|%d |", board[j + i * width]);
            } else if (!flag) {
                printf("| %d |", board[j + i * width]);
            }
            flag = 0;
            printf("%s", colores_reset);
        }
        printf("\n");
    }
    
}

void clear_screen() {
    const char *clear = "\033[2J\033[H"; // Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda
    write(STDOUT_FILENO, clear, strlen(clear));
}
static void print_player_stats(player_status* player_state){
    printf("name:%s\tpoints:%d\tvalidM:%d\tinvalidM:%d\tcoords:(%d,%d)%s\n",player_state->name_player,player_state->points,player_state->cant_valid_movements,player_state->cant_invalid_movements,player_state->x,player_state->y,colores_reset);
}


void print_stats(game_status* game_state){
    for (size_t i = 0; i < game_state->cant_players; i++){
        printf("%s",colores[i]);
        print_player_stats(&(game_state->players)[i]);
    }
}



int main(int argc, char const *argv[]) {
    
    int height, width;
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    semaphores_status * game_sync = get_game_sync();
    //chequear el size
    game_status * game_state = get_game_state(sizeof(game_status) + (sizeof(int) * (height * width)));
    sem_t * show_done= &(game_sync->show_done);
    sem_t * show_needed= &(game_sync->show_needed);

    while (!game_state->can_end) {
        sem_wait(show_needed);
        clear_screen();
        print_view(game_state->board, height, width, game_state);
        print_stats(game_state);
        sem_post(show_done);
    }
    return 0;
}
