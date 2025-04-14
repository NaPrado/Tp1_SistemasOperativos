#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

#include "shm.h"
#include "structures.h"
#include "sync-lib.h"
#include "return-codes.h"

#define RESET_COLOR "\033[0;0m"

#define BG_GOLD   "\x1b[48;5;178m"
#define FG_BLACK  "\x1b[30m"
#define FG_BLUE "\x1b[38;5;21m"

#define ES_CABEZA(x, y, j, i) ((x) == (j) && (y) == (i))

#define BOARD_AT(board, w, i, j) ((board)[(i) * (w) + (j)])

#define PLAY_SOUND(game_state) check_if_invalid((game_state)->players, (game_state)->amount_players) ? "\a" : ""

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

void print_horizontal_border(int width) {
    printf("%s%s", BG_GOLD, FG_BLACK);
    for (size_t i = 0; i < width + 2; i++) {
        printf("  *  ");
    }
    printf("%s\n", RESET_COLOR);    
}

void print_chomp_champs_logo(int width) {
    print_horizontal_border(width);
    printf("%s%s  *  %s", BG_GOLD, FG_BLACK, FG_BLUE);
    for (int i = 0; i < width-1; i++) {
        if ((width / 2) - 1 == i) {
            printf("%sCHOMPCHAMPS", (width % 2 != 0) ? " " : "");
        } else {
            printf("     ");
        }
    }
    printf("%s%s%s*  \n", BG_GOLD, FG_BLACK, (width % 2 != 0) ? "" : " ");
    print_horizontal_border(width);
}

void print_view(int * board, size_t height, size_t width, Tgame_state * game_state) {

    print_chomp_champs_logo(width);

    for (size_t i = 0; i < height; i++) {
        printf("%s%s  *  %s", BG_GOLD, FG_BLACK, RESET_COLOR);
        for (size_t j = 0; j < width; j++) {

            if (BOARD_AT(board, width, i, j) <= 0) {
                size_t k = (-1) * BOARD_AT(board, width, i, j);
                printf("%s     ", ES_CABEZA(game_state->players[k].x, game_state->players[k].y, j, i) ? colorescabeza[k] : colores[k]);
            } else {
                printf("| %d |", BOARD_AT(board, width, i, j));
            }

            printf("%s", RESET_COLOR);
        }
        printf("%s%s  *  %s\n", BG_GOLD, FG_BLACK, RESET_COLOR);
    }
    print_horizontal_border(width);
}

void clear_screen() {
    const char * clear = "\033[2J\033[H"; // Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda
    write(STDOUT_FILENO, clear, strlen(clear));
}
static void print_player_stats(Tplayer_state* player_state){
    printf("name:%s\tpoints:%d\tvalidM:%d\tinvalidM:%d\tcoords:(%d,%d)%s\n", player_state->name_player, player_state->points, player_state->amount_valid_movements, player_state->amount_invalid_movements, player_state->x, player_state->y, RESET_COLOR);
}


void print_stats(Tgame_state * game_state){
    for (size_t i = 0; i < game_state->amount_players; i++){
        printf("%s",colores[i]);
        print_player_stats(&(game_state->players)[i]);
    }
}
//esto es para hacer ruido si es invalida la pos
int invalid[9] = {0};
int check_if_invalid(Tplayer_state players[9], int amount_players){
    int ret=0;
    for (size_t i = 0; i < amount_players; i++) {
        if (players[i].amount_invalid_movements != invalid[i]) {
            ret = 1;
        }
        invalid[i] = players[i].amount_invalid_movements;
    }    
    return ret;
}

int main(int argc, char const *argv[]) {
    
    int height, width;
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    Tgame_sync * game_sync = get_game_sync();
    //chequear el size
    Tgame_state * game_state = get_game_state(GAME_STATUS_SIZE(game_state, width, height));

#ifndef _POSIX_VERSION

    if (init_systemv_semaphores() == ERROR) {
        perror("Error: set_player_semaphore failed");
        return EXIT_FAILURE;
    }

#endif

    while (!game_state->can_end) {
        
        wait_view(game_sync);
        
        clear_screen();
        
        print_view(game_state->board, height, width, game_state);
        
        printf("%s", PLAY_SOUND(game_state));
        
        print_stats(game_state);
        
        signal_view(game_sync);
    }

    signal_view(game_sync);

    munmap_game_state(game_state, GAME_STATUS_SIZE(game_state, width, height));
    munmap_game_sync(game_sync);

    return 0;
}
