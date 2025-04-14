#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <stdbool.h>
#include <sys/wait.h>
#include <semaphore.h>

#define MAX_NUM_PLAYERS 9
#define MAX_PLAYER_LENGTH 16

typedef int pipe_array[MAX_NUM_PLAYERS][2];
#define CLOSED_PIPE -1

#define GAME_STATUS_SIZE(game_state, width, height) (sizeof(*game_state) + (sizeof(int) * (width * height)))

typedef struct { 
    char name_player[MAX_PLAYER_LENGTH]; // Nombre del jugador 
    unsigned int points; // Puntaje 
    unsigned int amount_invalid_movements; // Cantidad de solicitudes de movimientos inválidas realizadas 
    unsigned int amount_valid_movements; // Cantidad de solicitudes de movimientos válidas realizadas 
    unsigned short x, y; // Coordenadas x e y en el tablero 
    pid_t pid; // Identificador de proceso 
    bool cant_move; // Indica si el jugador no tiene movimientos válidos disponibles 
  } Tplayer_state;

typedef struct { 
  unsigned short width; // Ancho del tablero 
  unsigned short height; // Alto del tablero 
  unsigned int amount_players; // Cantidad de jugadores 
  Tplayer_state players[MAX_NUM_PLAYERS]; // Lista de jugadores  
  bool can_end; // Indica si el juego no se ha terminado 
  int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1  
} Tgame_state;

typedef struct { 
  sem_t show_needed;  // Indica a la vista que hay cambios por imprimir  
  sem_t show_done;  // Indica al máster que la vista terminó de imprimir  
  sem_t master_mutex;  // Evita inanición del máster al acceder al estado  
  sem_t game_state_mutex;  // Protege el estado del juego contra modificaciones concurrentes  
  sem_t player_read_count_mutex;  // Protege la variable 'player_reading_status'  
  unsigned int player_reading_status;  // Cantidad de jugadores leyendo el estado  
} Tgame_sync;

typedef struct {
    size_t width; // ancho del tablero
    size_t height; // alto del tablero
    size_t delay;
    size_t timeout;
    size_t seed;
    char * view;
    char * players[MAX_NUM_PLAYERS];
    size_t amount_players;
} Tparameters;

#endif