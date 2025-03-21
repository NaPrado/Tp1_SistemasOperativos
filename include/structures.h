#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <stdbool.h>
#include <sys/wait.h>
#include <semaphore.h>
typedef struct { 
    char name_player[16]; // Nombre del jugador 
    unsigned int points; // Puntaje 
    unsigned int cant_invalid_movements; // Cantidad de solicitudes de movimientos inválidas realizadas 
    unsigned int cant_valid_movements; // Cantidad de solicitudes de movimientos válidas realizadas 
    unsigned short x, y; // Coordenadas x e y en el tablero 
    pid_t pid; // Identificador de proceso 
    bool can_move; // Indica si el jugador tiene movimientos válidos disponibles 
  } player_status;

typedef struct { 
  unsigned short width; // Ancho del tablero 
  unsigned short heigth; // Alto del tablero 
  unsigned int cant_players; // Cantidad de jugadores 
  player_status players[9]; // Lista de jugadores  
  bool can_end; // Indica si el juego se ha terminado 
  int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1  
} game_status;

typedef struct { 
  sem_t show_needed;  // Indica a la vista que hay cambios por imprimir  
  sem_t show_done;  // Indica al máster que la vista terminó de imprimir  
  sem_t master_mutex;  // Evita inanición del máster al acceder al estado  
  sem_t game_state_mutex;  // Protege el estado del juego contra modificaciones concurrentes  
  sem_t player_read_count_mutex;  // Protege la variable 'player_reading_status'  
  unsigned int player_reading_status;  // Cantidad de jugadores leyendo el estado  
} semaphores_status;
#endif